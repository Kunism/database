
#include "qe.h"
#include <iostream>
#include <cmath>

//Condition::Condition() {
//
//}
//
//Condition::~Condition() {
//    if(this->bRhsIsAttr == false) {
//        delete[] this->rhsValue.data;
//    }
//}
//
//Condition Condition::operator= (const Condition &rCondition) {
//
//    this->lhsAttr = rCondition.lhsAttr;
//    this->op = rCondition.op;
//    this->bRhsIsAttr = rCondition.bRhsIsAttr;
//    this->rhsAttr = rCondition.rhsAttr;
//
//    //this->rhsValue.type = rCondition.rhsValue.type;
//
//    if(rCondition.bRhsIsAttr == false) {
//        this->rhsValue.type = rCondition.rhsValue.type;
//        this->rhsValue.data = new char [PAGE_SIZE];
//        memset(this->rhsValue.data, 0, PAGE_SIZE);
//
//        uint32_t conditionDataLen = sizeof(uint32_t);
//        if(rCondition.rhsValue.type == TypeVarChar) {
//            uint32_t strLen = 0;
//            memcpy(&strLen, rCondition.rhsValue.data, sizeof(uint32_t));
//            conditionDataLen += strLen;
//        }
//        memcpy(this->rhsValue.data, rCondition.rhsValue.data, conditionDataLen);
//
////        if (rCondition.rhsValue.type == TypeVarChar) {
////            uint32_t strLen = 0;
////            memcpy(&strLen, rCondition.rhsValue.data, sizeof(uint32_t));
////
////            this->rhsValue.data = new char[strLen + sizeof(uint32_t)];
////            memcpy(this->rhsValue.data, rCondition.rhsValue.data, strLen + sizeof(uint32_t));
////        } else {
////            this->rhsValue.data = new char[sizeof(uint32_t)];
////            memcpy(this->rhsValue.data, rCondition.rhsValue.data, sizeof(uint32_t));
////        }
//    }
//    return *this;
//}

// given attrs and attrName return returnAttr. 
RC Iterator::searchAttribute(const std::vector<Attribute> &attrs, const std::string &attrName, Attribute &returnAttr) {
    for(auto attr : attrs) {
        if( attr.name == attrName) {
            returnAttr = attr;
            return 0;
        }
    }
    return -1;
}

RC Iterator::mergeTwoTuple(const std::vector<Attribute> &leftAttribute, const char *leftTuple,
                           const std::vector<Attribute> &rightAttrbute, const char *rightTuple, void *mergedTuple) {


    uint32_t leftNullIndicatorSize = ceil(leftAttribute.size() / 8.0);
    uint32_t rightNullIndicatorSize = ceil(rightAttrbute.size() / 8.0);
    uint32_t totalNullIndicatorSize = ceil((leftAttribute.size() + rightAttrbute.size()) / 8.0);

    char* nullIndicator = new char [totalNullIndicatorSize];
    memset(nullIndicator, 0, totalNullIndicatorSize);

    Record leftRecord(leftAttribute, leftTuple, {0, 0});
    Record rightRecord(rightAttrbute, rightTuple, {0, 0});

    for(int i = 0; i < leftAttribute.size(); i++) {

        uint8_t nullIndicatorBuffer = 0;
        uint8_t byteOffset = i / CHAR_BIT;
        uint8_t bitOffset = i % CHAR_BIT;
        memcpy(&nullIndicatorBuffer, nullIndicator + byteOffset, sizeof(uint8_t));

        if(leftRecord.isNull(i)) {
            nullIndicatorBuffer = nullIndicatorBuffer >> (CHAR_BIT - bitOffset - 1);
            nullIndicatorBuffer = nullIndicatorBuffer | 1;
            nullIndicatorBuffer = nullIndicatorBuffer << (CHAR_BIT - bitOffset - 1);
            memcpy(nullIndicator + byteOffset, &nullIndicatorBuffer, sizeof(uint8_t));
        }
        else {
            nullIndicatorBuffer = nullIndicatorBuffer >> (CHAR_BIT - bitOffset);
            nullIndicatorBuffer = nullIndicatorBuffer << (CHAR_BIT - bitOffset);
            memcpy(nullIndicator + byteOffset, &nullIndicatorBuffer, sizeof(uint8_t));
        }
    }

    for(int i = 0; i < rightAttrbute.size(); i++) {
        uint8_t nullIndicatorBuffer = 0;
        uint8_t byteOffset = (leftAttribute.size() + i) / CHAR_BIT;
        uint8_t bitOffset = (leftAttribute.size() + i) % CHAR_BIT;
        memcpy(&nullIndicatorBuffer, nullIndicator + byteOffset, sizeof(uint8_t));

        if(leftRecord.isNull(i)) {
            nullIndicatorBuffer = nullIndicatorBuffer >> (CHAR_BIT - bitOffset - 1);
            nullIndicatorBuffer = nullIndicatorBuffer | 1;
            nullIndicatorBuffer = nullIndicatorBuffer << (CHAR_BIT - bitOffset - 1);
            memcpy(nullIndicator + byteOffset, &nullIndicatorBuffer, sizeof(uint8_t));
        }
        else {
            nullIndicatorBuffer = nullIndicatorBuffer >> (CHAR_BIT - bitOffset);
            nullIndicatorBuffer = nullIndicatorBuffer << (CHAR_BIT - bitOffset);
            memcpy(nullIndicator + byteOffset, &nullIndicatorBuffer, sizeof(uint8_t));
        }
    }

    uint8_t tmp = *(uint8_t*)nullIndicator;
    //std::cerr << "Merged nullindicator = " << (unsigned)tmp << std::endl;

    uint32_t mergedTupleSize = totalNullIndicatorSize + leftRecord.getDataSize() + rightRecord.getDataSize();

    uint32_t offset = 0;
    memset(mergedTuple, 0, mergedTupleSize);

    memcpy((char*)mergedTuple + offset, nullIndicator, totalNullIndicatorSize);
    offset += totalNullIndicatorSize;

    memcpy((char*)mergedTuple + offset, leftTuple + leftNullIndicatorSize, leftRecord.getDataSize());
    offset += leftRecord.getDataSize();

    memcpy((char*)mergedTuple + offset, rightTuple + rightNullIndicatorSize, rightRecord.getDataSize());
    offset += rightRecord.getDataSize();

    return 0;
}

Filter::Filter(Iterator *input, const Condition &condition) {
    this->m_input = input;
    this->m_condition = condition;

//    std::cerr << "condition=" << condition.rhsValue.data << "###m_condition=" << m_condition.rhsValue.data << std::endl;
//    std::cerr << *(int*)condition.rhsValue.data << "###" << *(int*)m_condition.rhsValue.data << std::endl;
    //std::cerr << condition.rhsValue.
}

RC Filter::getNextTuple(void *data) {
    while(true) {
        if(m_input->getNextTuple(data) == QE_EOF) {
            return QE_EOF;
        }
        std::vector<Attribute> attributes;
        m_input->getAttributes(attributes);

        m_attrType = Utility::getAttributeByName(m_condition.lhsAttr, attributes).type;
//        for(auto it = attributes.begin(); it != attributes.end(); it++) {
//            if(it->name == m_condition.lhsAttr) {
//                m_attrType = it->type;
//            }
//        }


        uint32_t attrDataMaxLen = getAttributeMaxLength(attributes, m_condition.lhsAttr);

        char* attrData = new char [attrDataMaxLen];
        memset(attrData, 0, attrDataMaxLen);

        if(readAttribute(attributes, m_condition.lhsAttr, (const void*)data, attrData, m_attrType) == -1) {
            delete[] attrData;
            return QE_EOF;
        }

        if(compValue(attrData, (const char*)m_condition.rhsValue.data)) {
            delete[] attrData;
            return 0;
        }
    }
}

RC Filter::readAttribute(const std::vector<Attribute> &attrs, const std::string attrName, const void *tupleData, char* attrData, AttrType &attrType) {

    Record record(attrs, tupleData, {0, 0});
    record.getAttribute(attrName, attrs, attrData);
//    std::cerr << "Filter::getAttribute attrData=" << *( (int*) ((char*)attrData + 1)   ) << std::endl;

//    uint32_t attrOffset = sizeof(uint8_t);
//
//    for(int i = 0; i < attrs.size(); i++) {
//
//        uint32_t dataLength = sizeof(uint32_t);
//
//        if(attrs[i].type == TypeVarChar) {
//            uint32_t strLen = 0;
//            memcpy(&strLen, (const char*)tupleData + attrOffset, sizeof(uint32_t));
//
//            dataLength += strLen;
//        }
//
//        if(attrs[i].name == attrName) {
//            memcpy(attrData, (const char*)tupleData + attrOffset, dataLength);
//            std::cerr << "Filter::getAttribute attrData=" << *((int*)attrData) << std::endl;
//            return 0;
//        }
//
//        attrOffset += dataLength;
//    }
    return 0;
}

void Filter::getAttributes(std::vector<Attribute> &attrs) const {
    m_input->getAttributes(attrs);
}

uint32_t Filter::getAttributeMaxLength(std::vector<Attribute> &attrs, const std::string attrName) {
    uint32_t maxLength = sizeof(uint8_t);   //  nullIndicator

    Attribute attribute = Utility::getAttributeByName(attrName, attrs);
    if(attribute.type == TypeVarChar) {
        maxLength += attribute.length + sizeof(int);
    }
    else {
        maxLength += attribute.length;
    }
    return maxLength;

//    for(int i = 0; i < attrs.size(); i++) {
//        if(attrs[i].name == attrName) {
//            if(attrs[i].type == TypeVarChar) {
//                maxLength += attrs[i].length + sizeof(int);
//            }
//            else {
//                maxLength += attrs[i].length;
//            }
//        }
//    }
//    return maxLength;
}

bool Filter::compValue(const char *lData, const char *conditionData) {
    //std::cerr << *((int*)((const char*)lData + 1)) << "#####" << *(int*)conditionData << "Tail" << std::endl;

    uint8_t nullIndicator = 0x80;
    if(memcmp(&nullIndicator, lData, sizeof(uint8_t)) == 0) {
        return false;
    }

    // Record record(0);
    return Record::isMatch(m_attrType, lData + sizeof(uint8_t) , conditionData, m_condition.op);

//    unsigned offset = sizeof(uint8_t);
//
//    if(m_condition.rhsValue.type == TypeInt) {
//        int lValue = 0;
//        int condition = 0;
//
//        memcpy(&lValue, lData + offset, sizeof(int));
//        memcpy(&condition, conditionData, sizeof(int));
//
//        switch(m_condition.op) {
//            case EQ_OP: return lValue == condition;
//            case LT_OP: return lValue < condition;
//            case LE_OP: return lValue <= condition;
//            case GT_OP: return lValue > condition;
//            case GE_OP: return lValue >= condition;
//            case NE_OP: return lValue != condition;
//            default: return false;
//        }
//    }
//    else if(m_condition.rhsValue.type == TypeReal) {
//        float lValue = 0.0;
//        float condition = 0.0;
//
//        memcpy(&lValue, lData, sizeof(float));
//        memcpy(&condition, conditionData, sizeof(float));
//
//        switch(m_condition.op) {
//            case EQ_OP: return lValue == condition;
//            case LT_OP: return lValue < condition;
//            case LE_OP: return lValue <= condition;
//            case GT_OP: return lValue > condition;
//            case GE_OP: return lValue >= condition;
//            case NE_OP: return lValue != condition;
//            default: return false;
//        }
//    }
//    else if(m_condition.rhsValue.type == TypeVarChar) {
//        uint32_t lValueLength = 0;
//        uint32_t conditionLength = 0;
//
//        memcpy(&lValueLength, lData, sizeof(uint32_t));
//        memcpy(&conditionLength, conditionData, sizeof(uint32_t));
//
//        char* lValueBuffer = new char[lValueLength + 1];
//        char* conditionBuffer = new char[conditionLength + 1];
//
//        memset(lValueBuffer, 0, lValueLength + 1);
//        memset(conditionBuffer, 0, conditionLength + 1);
//
//        memcpy(lValueBuffer, lData + sizeof(uint32_t), lValueLength);
//        memcpy(conditionBuffer, conditionData + sizeof(uint32_t), conditionLength);
//
//        lValueBuffer[lValueLength] = '\0';
//        conditionBuffer[conditionLength] = '\0';
//
//        std::string record(lValueBuffer, lValueLength);
//        std::string condition(conditionBuffer, conditionLength);
//
//        delete[] lValueBuffer;
//        delete[] conditionBuffer;
//
//        switch(m_condition.op) {
//            case EQ_OP: return record == condition;
//            case LT_OP: return record < condition;
//            case LE_OP: return record <= condition;
//            case GT_OP: return record > condition;
//            case GE_OP: return record >= condition;
//            case NE_OP: return record != condition;
//            default: return false;
//        }
//    }
}


Project::Project(Iterator *input, const std::vector<std::string> &attrNames) {
    this->m_input = input;
    m_input->getAttributes(m_leftAttributes);

    for(int i = 0; i < attrNames.size(); i++) {
        m_projectedAttributes.push_back(Utility::getAttributeByName(attrNames[i], m_leftAttributes));
    }

}

RC Project::getNextTuple(void *data) {
    char* tuple = new char[PAGE_SIZE];
    memset(tuple, 0, PAGE_SIZE);

    if(m_input->getNextTuple(tuple) != 0) {
        delete[] tuple;
        return QE_EOF;
    }
    //std::cerr << "Project::getNextTuple Success" << std::endl;
    Record record(m_leftAttributes, tuple, {0, 0});
    uint32_t nullIndicatorSize = ceil(m_projectedAttributes.size() / 8.0);


    uint32_t projectedOffset = nullIndicatorSize;

    for(int i = 0; i < m_projectedAttributes.size(); i++) {
        uint8_t nullIndicatorBuffer = 0;
        uint8_t byteOffset = i / CHAR_BIT;
        uint8_t bitOffset = i % CHAR_BIT;

        memcpy(&nullIndicatorBuffer, (char*)data + byteOffset, sizeof(uint8_t));

        if(record.isNull(Utility::getAttrIndexByName(m_projectedAttributes[i].name, m_leftAttributes))) {
            nullIndicatorBuffer = nullIndicatorBuffer >> (CHAR_BIT - bitOffset - 1);
            nullIndicatorBuffer = nullIndicatorBuffer | 1;
            nullIndicatorBuffer = nullIndicatorBuffer << (CHAR_BIT - bitOffset - 1);
            memcpy((char*)data + byteOffset, &nullIndicatorBuffer, sizeof(uint8_t));
        }
        else {
            nullIndicatorBuffer = nullIndicatorBuffer >> (CHAR_BIT - bitOffset);
            nullIndicatorBuffer = nullIndicatorBuffer << (CHAR_BIT - bitOffset);
            memcpy((char*)data + byteOffset, &nullIndicatorBuffer, sizeof(uint8_t));

            uint32_t leftOffset = nullIndicatorSize;
            for(auto it = m_leftAttributes.begin(); it != m_leftAttributes.end(); it++) {

                uint32_t attrLength = sizeof(uint32_t);
                if(it->type == TypeVarChar) {
                    uint32_t strLen = 0;
                    memcpy(&strLen, tuple + leftOffset, sizeof(uint32_t));
                    attrLength += strLen;
                }

                if(it->name == m_projectedAttributes[i].name) {
                    memcpy((char*)data + projectedOffset, tuple + leftOffset, attrLength);
                    projectedOffset += attrLength;

                    uint8_t tmp = *(uint8_t*)data;
                    break;
                }
                leftOffset += attrLength;
            }
        }

    }
}

void Project::getAttributes(std::vector<Attribute> &attrs) const {
    attrs.clear();
    attrs = m_projectedAttributes;
}



INLJoin::INLJoin(Iterator *leftIn, IndexScan *rightIn, const Condition &condition) {
    this->m_leftInput = leftIn;
    this->m_rightInput = rightIn;
    this->m_condition = condition;
    this->m_isFirstScan = true;

    m_leftInput->getAttributes(m_leftAttribute);
    m_rightInput->getAttributes(m_rightAttribute);

    m_leftTupleData = new char [PAGE_SIZE];
    m_rightTupleData = new char [PAGE_SIZE];

    memset(m_leftTupleData, 0, PAGE_SIZE);
    memset(m_rightTupleData, 0, PAGE_SIZE);

}

INLJoin::~INLJoin() {

}

RC INLJoin::getNextTuple(void *data) {


    memset(m_rightTupleData, 0, PAGE_SIZE);
    //std::cerr << "Begin INLJoin::getNxtTuple" << std::endl;

    if(!m_isFirstScan && m_rightInput->getNextTuple(m_rightTupleData) != QE_EOF) {
        //std::cerr << "Got tuple from right" << std::endl;
        mergeTwoTuple(m_leftAttribute, m_leftTupleData, m_rightAttribute, m_rightTupleData, data);

        return 0;
    }

    //std::cerr << "First in" << std::endl;
    memset(m_leftTupleData, 0, PAGE_SIZE);
    do{
        // std::cerr << "Try to getNext from left" << std::endl;
        if(m_leftInput->getNextTuple(m_leftTupleData) == QE_EOF) {
            // std::cerr << "Left is over" << std::endl;
            return QE_EOF;
        }
        // std::cerr << "INLJoin::getNextTuple Success" << std::endl;
        char* leftAttrData = new char [PAGE_SIZE];
        memset(leftAttrData, 0, PAGE_SIZE);
        auto &rm = RelationManager::instance();
        // std::cerr << "Print Left tuple" << std::endl;
        rm.printTuple(m_leftAttribute,m_leftTupleData);
        Record leftTuple(m_leftAttribute, m_leftTupleData, {0, 0});
        leftTuple.getAttribute(m_condition.lhsAttr, m_leftAttribute, leftAttrData);

        //  return value of getAttribute contains nullIndicator
        //  , but underlying key constructor does not expect nullIndicator
        // std::cerr << "RIGHT SET IT -----------------------------------" << std::endl;
        // std::cerr << "LOW: " << ((int*)leftAttrData + sizeof(uint8_t))[0] << std::endl;
        m_rightInput->setIterator(leftAttrData + sizeof(uint8_t), leftAttrData + sizeof(uint8_t), true, true);
        // std::cerr << "RIGHT SET IT END-----------------------------------" << std::endl;


        delete[] leftAttrData;

    }while(m_rightInput->getNextTuple(m_rightTupleData) == QE_EOF);

    mergeTwoTuple(m_leftAttribute, m_leftTupleData, m_rightAttribute, m_rightTupleData, data);
    m_isFirstScan = false;

    return 0;
}

void INLJoin::getAttributes(std::vector<Attribute> &attrs) const {
    attrs.clear();

    for(int i = 0; i < m_leftAttribute.size(); i++) {
        attrs.push_back(m_leftAttribute[i]);
    }

    for(int i = 0; i < m_rightAttribute.size(); i++) {
        attrs.push_back(m_rightAttribute[i]);
    }
}


BNLJoin::BNLJoin(Iterator *leftIn, // Iterator of input R
            TableScan *rightIn,    // TableScan Iterator of input S
            const Condition &condition,  // Join condition
            const unsigned numPages) {    // # of pages that can be 

    finishFlag = false;
    m_leftInput = leftIn;
    m_rightInput = rightIn;
    m_condition = condition;
    m_numPages = numPages;

    m_leftInput->getAttributes(m_leftAttribute);
    m_rightInput->getAttributes(m_rightAttribute);
    
    m_leftInputData = new uint8_t [PAGE_SIZE];
    m_rightInputData = new uint8_t [PAGE_SIZE];

    m_hashTableSize = 0;
    m_nextIndicator = 0;
}

BNLJoin::~BNLJoin() {
    delete[] m_leftInputData;
    delete[] m_rightInputData;
}


void BNLJoin::getAttributes(std::vector<Attribute> &attrs) const {
    attrs.clear();

    for(auto &attr : m_leftAttribute) {
        attrs.push_back(attr);
    }

    for(auto &attr : m_rightAttribute) {
        attrs.push_back(attr);
    }
   

}

RC BNLJoin::getNextTuple(void *data) { 

    
    while(true) {
        if(recordBuffer.size() != 0) {
            auto lastRecordPair = recordBuffer.back();
            char* leftRecord = new char [lastRecordPair.first.recordSize];
            char* rightRecord = new char [lastRecordPair.second.recordSize];
            lastRecordPair.first.decode(leftRecord);
            lastRecordPair.second.decode(rightRecord);
            Iterator::mergeTwoTuple(m_leftAttribute, leftRecord, m_rightAttribute, rightRecord, data);
            recordBuffer.pop_back();
            return 0;
        }

        while(m_hashTableSize < m_numPages * PAGE_SIZE) {
              
            if(m_leftInput->getNextTuple(m_leftInputData) != QE_EOF) {
                Record nextRecord(m_leftAttribute, m_leftInputData, {0,0});

                uint8_t* attrData = new uint8_t [PAGE_SIZE];
                memset(attrData, 0, PAGE_SIZE);
                nextRecord.getAttribute(m_condition.lhsAttr, m_leftAttribute, attrData);
                uint8_t nullFlag = 0x80;
                if(memcmp(attrData, &nullFlag, sizeof(uint8_t)) != 0) {
                    Attribute targetAttr;
                    searchAttribute(m_leftAttribute, m_condition.lhsAttr, targetAttr);
                    Key key(attrData+1, {0,0}, targetAttr.type);    
                    m_hashtable[key].push_back(nextRecord);
                    m_hashTableSize += nextRecord.recordSize;
                }
                delete[] attrData;
            } 
            else if(m_hashTableSize == 0) {
                finishFlag = true;
                return QE_EOF;
            }
            else {
                break;
            }
        }

        while(m_rightInput->getNextTuple(m_rightInputData) != QE_EOF) {
            Record rightRecord(m_rightAttribute, m_rightInputData, {0,0});

            uint8_t* attrData = new uint8_t [PAGE_SIZE];
            memset(attrData, 0, PAGE_SIZE);
            rightRecord.getAttribute(m_condition.rhsAttr, m_rightAttribute, attrData);
            uint8_t nullFlag = 0x80;

            if(memcmp(attrData, &nullFlag, sizeof(uint8_t)) != 0) {
                Attribute targetAttr;
                searchAttribute(m_rightAttribute, m_condition.rhsAttr, targetAttr);

                Key key(attrData+1, {0,0}, targetAttr.type);
                if(m_hashtable.find(key) != m_hashtable.end()) {
                    for(auto r : m_hashtable[key]) {
                        recordBuffer.push_back({r, rightRecord});
                    }
                }
            }
        }

        m_hashtable.clear();
        m_hashTableSize = 0;
        m_rightInput->setIterator();
    }
    return QE_EOF;
}
        

Attribute Utility::getAttributeByName(std::string attrName, std::vector<Attribute> &attrs) {
    for(int i = 0; i < attrs.size(); i++) {
        if(attrs[i].name == attrName) {
            return attrs[i];
        }
    }
}

uint32_t Utility::getAttrIndexByName(std::string attrName, std::vector<Attribute> &attrs) {
    for(int i = 0; i < attrs.size(); i++) {
        if(attrs[i].name == attrName) {
            return i;
        }
    }
}
