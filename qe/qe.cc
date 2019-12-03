
#include "qe.h"
#include <iostream>

Condition Condition::operator= (const Condition &rCondition) {
    this->lhsAttr = rCondition.lhsAttr;
    this->op = rCondition.op;
    this->bRhsIsAttr = rCondition.bRhsIsAttr;
    this->rhsAttr = rCondition.rhsAttr;

    this->rhsValue.type = rCondition.rhsValue.type;
    if(rCondition.rhsValue.type == TypeVarChar) {
        uint32_t strLen = 0;
        memcpy(&strLen, rCondition.rhsValue.data, sizeof(uint32_t));

        this->rhsValue.data = new char [strLen + sizeof(uint32_t)];
        memcpy(this->rhsValue.data, rCondition.rhsValue.data, strLen + sizeof(uint32_t));
    }
    else {
        this->rhsValue.data = new char [sizeof(uint32_t)];
        memcpy(this->rhsValue.data, rCondition.rhsValue.data, sizeof(uint32_t));
    }

    return *this;
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

    m_leftInput->getAttributes(m_leftAttribute);
    m_rightInput->getAttributes(m_rightAttribute);
}

INLJoin::~INLJoin() {

}

RC INLJoin::getNextTuple(void *data) {

}

void INLJoin::getAttributes(std::vector<Attribute> &attrs) const {

}


// ... the rest of your implementations go here
