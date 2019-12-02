
#include "qe.h"
#include <iostream>

Filter::Filter(Iterator *input, const Condition &condition) {
    this->m_input = input;
    this->m_condition = condition;
}

RC Filter::getNextTuple(void *data) {
    while(true) {
        if(m_input->getNextTuple(data) == QE_EOF) {
            return QE_EOF;
        }

        std::vector<Attribute> attributes;
        m_input->getAttributes(attributes);

        uint32_t attrDataMaxLen = getAttributeMaxLength(attributes, m_condition.lhsAttr);
        char* attrData = new char [attrDataMaxLen];

        if(getAttribute(attributes, m_condition.lhsAttr, (const void*)data, attrData, m_attrType) == -1) {
            delete[] attrData;
            return QE_EOF;
        }

        if(compValue(attrData, (const char*)m_condition.rhsValue.data)) {
            return 0;
        }
    }
}

RC Filter::getAttribute(const std::vector<Attribute> &attrs, const std::string attrName, const void *tupleData, char* attrData, AttrType &attrType) {

    Record record(attrs, tupleData, ((TableScan*)m_input)->rid);
    record.getAttribute(attrName, attrs, attrData);

//    uint32_t attrOffset = 0;
//
//    for(int i = 0; i < attrs.size(); i++) {
//
//        uint32_t dataLength = sizeof(uint32_t);
//
//        if(attrs[i].type == TypeVarChar) {
//            uint32_t strLen = 0;
//            memcpy(&strLen, tupleData + attrOffset, sizeof(uint32_t));
//
//            dataLength += strLen;
//        }
//
//        if(attrs[i].name == attrName) {
//            memcpy(attrData, tupleData + attrOffset, dataLength);
//            return 0;
//        }
//
//        attrOffset += dataLength;
//    }
//    return -1;
}

void Filter::getAttributes(std::vector<Attribute> &attrs) const {
    m_input->getAttributes(attrs);
}

uint32_t Filter::getAttributeMaxLength(std::vector<Attribute> &attrs, const std::string attrName) {
    for(int i = 0; i < attrs.size(); i++) {
        if(attrs[i].name == attrName) {
            return attrs[i].length;
        }
    }
    return 0;
}

bool Filter::compValue(const char *lData, const char *conditionData) {
    Record record(0);
    return record.isMatch(m_attrType, lData, conditionData, m_condition.op);
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

}

RC Project::getNextTuple(void *data) {

}

void Project::getAttributes(std::vector<Attribute> &attrs) const {

}


// ... the rest of your implementations go here
