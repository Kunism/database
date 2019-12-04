
#include "qe.h"
#include <iostream>

Condition Condition::operator= (const Condition &rCondition) {
    // std::cerr << __LINE__ << std::endl;
    this->lhsAttr = rCondition.lhsAttr;
    this->op = rCondition.op;
    this->bRhsIsAttr = rCondition.bRhsIsAttr;
    this->rhsAttr = rCondition.rhsAttr;

    this->rhsValue.type = rCondition.rhsValue.type;

    if(rCondition.bRhsIsAttr == false) {
        if (rCondition.rhsValue.type == TypeVarChar) {
            uint32_t strLen = 0;
            memcpy(&strLen, rCondition.rhsValue.data, sizeof(uint32_t));

            this->rhsValue.data = new char[strLen + sizeof(uint32_t)];
            memcpy(this->rhsValue.data, rCondition.rhsValue.data, strLen + sizeof(uint32_t));
        } else {
            this->rhsValue.data = new char[sizeof(uint32_t)];
            memcpy(this->rhsValue.data, rCondition.rhsValue.data, sizeof(uint32_t));
        }
    }
    return *this;
}

RC Iterator::mergeTwoTuple(const std::vector<Attribute> &leftAttribute, const char *leftTuple,
                           const std::vector<Attribute> &rightAttrbute, const char *rightTuple, void *mergedTuple) {

    Record leftRecord(leftAttribute, leftTuple, {0, 0});
    Record rightRecord(rightAttrbute, rightTuple, {0, 0});

    uint32_t offset = 0;
    memset(mergedTuple, 0, leftRecord.recordSize + rightRecord.recordSize);

    memcpy((char*)mergedTuple + offset, leftTuple, leftRecord.recordSize);
    offset += leftRecord.recordSize;

    memcpy((char*)mergedTuple + offset, rightTuple, rightRecord.recordSize);
    offset += rightRecord.recordSize;

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

        //std::cerr << "attrs size = " << attributes.size() << std::endl;

        uint32_t attrDataMaxLen = getAttributeMaxLength(attributes, m_condition.lhsAttr);

        //std::cerr << "attrDataMaxLen = " << attrDataMaxLen << std::endl;

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

    Record record(attrs, tupleData, ((TableScan*)m_input)->rid);
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
    for(int i = 0; i < attrs.size(); i++) {
        if(attrs[i].name == attrName) {
            if(attrs[i].type == TypeVarChar) {
                maxLength += attrs[i].length + sizeof(int);
            }
            else {
                maxLength += attrs[i].length;
            }
        }
    }
    return maxLength;
}

bool Filter::compValue(const char *lData, const char *conditionData) {
    //std::cerr << *((int*)((const char*)lData + 1)) << "#####" << *(int*)conditionData << "Tail" << std::endl;

    uint8_t nullIndicator = 0x80;
    if(memcmp(&nullIndicator, lData, sizeof(uint8_t)) == 0) {
        return false;
    }

    Record record(0);
    return record.isMatch(m_attrType, lData + sizeof(uint8_t) , conditionData, m_condition.op);
//    if(m_condition.rhsValue.type == TypeInt) {
//        int lValue = 0;
//        int condition = 0;
//
//        memcpy(&lValue, lData, sizeof(int));
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
//        memset(lValueBuffer, lValueLength + 1, 0);
//        memset(conditionBuffer, conditionLength + 1, 0);
//
//        memcpy(lValueBuffer, lData + sizeof(uint32_t), lValueLength);
//        memcpy(conditionBuffer, conditionData + sizeof(uint32_t), conditionLength);
//
//        lValueBuffer[lValueLength] = '\0';
//        conditionBuffer[conditionLength] = '\0';
//
//        std::string record(lValueBuffer);
//        std::string condition(conditionBuffer);
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

    for(int i = 0; i < m_leftAttributes.size(); i++) {
        for(auto it = m_leftAttributes.begin(); it != m_leftAttributes.end(); it++) {
            if(it->name == attrNames[i]) {
                m_projectedAttributes.push_back(*it);
                break;
            }
        }
    }

}

RC Project::getNextTuple(void *data) {
    char* tuple = new char[PAGE_SIZE];
    memset(tuple, 0, PAGE_SIZE);

    if(m_input->getNextTuple(tuple) != 0) {
        delete[] tuple;
        return QE_EOF;
    }

    uint32_t projectedOffset = 0;

    for(int i = 0; i < m_projectedAttributes.size(); i++) {

        uint32_t leftOffset = 0;
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
                break;
            }
            leftOffset += attrLength;
        }
    }
}

void Project::getAttributes(std::vector<Attribute> &attrs) const {
    attrs.clear();
    attrs = m_leftAttributes;
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
    if(!m_isFirstScan && m_rightInput->getNextTuple(m_rightTupleData) != QE_EOF) {

        mergeTwoTuple(m_leftAttribute, m_leftTupleData, m_rightAttribute, m_rightTupleData, data);

//        //RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
//        Record leftTuple(m_leftAttribute, m_leftTupleData, {0, 0});
//        Record rightTuple(m_rightAttribute, m_rightTupleData, {0, 0});
//
//        uint32_t offset = 0;
//        memset(data, 0, leftTuple.recordSize + rightTuple.recordSize);
//
//        memcpy((char*)data + offset, m_leftTupleData, leftTuple.recordSize);
//        offset += leftTuple.recordSize;
//
//        memcpy((char*)data + offset, m_rightTupleData, rightTuple.recordSize);
//        offset += rightTuple.recordSize;

        return 0;
    }


    memset(m_leftTupleData, 0, PAGE_SIZE);
    do{
        if(m_leftInput->getNextTuple(m_leftTupleData) == QE_EOF) {
            return QE_EOF;
        }

        char* leftAttrData = new char [PAGE_SIZE];
        memset(leftAttrData, 0, PAGE_SIZE);

        Record leftTuple(m_leftAttribute, m_leftTupleData, {0, 0});
        leftTuple.getAttribute(m_condition.lhsAttr, m_leftAttribute, leftAttrData);

        //  return value of getAttribute contains nullIndicator
        //  , but underlying key constructor does not expect nullIndicator
        m_rightInput->setIterator(leftAttrData + sizeof(uint8_t), leftAttrData + sizeof(uint8_t), true, true);

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
        // read hash table
        while(m_hashTableSize < m_numPages * PAGE_SIZE && 
              m_leftInput->getNextTuple(m_leftInputData) != QE_EOF ) {
            Record nextRecord(m_leftAttribute, m_leftInputData, {0,0});

            uint8_t* attrData = new uint8_t [PAGE_SIZE];
            memset(attrData, 0, PAGE_SIZE);
            nextRecord.getAttribute(m_condition.lhsAttr, m_leftAttribute, attrData);
            uint8_t nullFlag = 0x80;
            if(memcmp(attrData, &nullFlag, sizeof(uint8_t)) != 0) {
                Attribute targetAttr;
                for(auto attr : m_leftAttribute) {
                    if(attr.name == m_condition.lhsAttr) {
                        targetAttr = attr;
                    }
                }
                Key key(attrData+1, {0,0}, targetAttr.type);    
                m_hashtable[key].push_back(nextRecord);
                m_hashTableSize += nextRecord.recordSize;
            }
            delete[] attrData;
        }
        // what if left used up ?????????????????????//
        while(m_rightInput->getNextTuple(m_rightInputData) != QE_EOF) {

            // last iterator not used up ?????????
            Record rightRecord(m_rightAttribute, m_rightInputData, {0,0});

            uint8_t* attrData = new uint8_t [PAGE_SIZE];
            memset(attrData, 0, PAGE_SIZE);
            rightRecord.getAttribute(m_condition.rhsAttr, m_rightAttribute, attrData);
            uint8_t nullFlag = 0x80;
            if(memcmp(attrData, &nullFlag, sizeof(uint8_t)) != 0) {
                Attribute targetAttr;
                for(auto attr : m_rightAttribute) {
                    if(attr.name == m_condition.rhsAttr) {
                        targetAttr = attr;
                    }
                }

                Key key(attrData+1, {0,0}, targetAttr.type);
                if(m_hashtable.find(key) != m_hashtable.end()) {
                    if (m_nextIndicator < m_hashtable[key].size()) {
                        uint8_t* leftRecord = new uint8_t [  m_hashtable[key][m_nextIndicator].recordSize];
                        m_hashtable[key][m_nextIndicator].decode(leftRecord);
                        Iterator::mergeTwoTuple(m_leftAttribute, (char*)leftRecord, m_rightAttribute, (char*)m_rightInputData, data);
                        m_nextIndicator++;
                        return;
                    }
                    else {
                        m_nextIndicator = 0;
                    }
                }
            }
        }


    }
   

       
        
        
}
