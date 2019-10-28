#include "rm.h"
#include <iostream>
RelationManager *RelationManager::_relation_manager = nullptr;

RelationManager &RelationManager::instance() {
    static RelationManager _relation_manager = RelationManager();
    return _relation_manager;
}


const std::vector<Attribute> RelationManager::m_tablesDescriptor = {
    {
        "tableId",
        TypeInt,
        (AttrLength)4
    },        
    {
        "tableName",
        TypeVarChar,
        (AttrLength)50,
    },
    {
        "fileName",
        TypeVarChar,
        (AttrLength) 50
    } 
};


const std::vector<Attribute> RelationManager::m_collumnsDescriptor = {
    {
        "tableId",
        TypeInt,
        (AttrLength) 4
    },
    {
        "columnName",
        TypeVarChar,
        (AttrLength) 50
    },
    {
        "columnType",
        TypeInt,
        (AttrLength) 4
    },
    {
        "columnLength",
        TypeInt,
        (AttrLength) 4
    },
    {
        "columnPosition",
        TypeInt,
        (AttrLength) 4
    }
};



RelationManager::RelationManager() = default;

RelationManager::~RelationManager() { delete _relation_manager; }

RelationManager::RelationManager(const RelationManager &) = default;

RelationManager &RelationManager::operator=(const RelationManager &) = default;

RC RelationManager::createCatalog() {
    //TODO init cont descriptor 
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    rbfm.createFile("Variables");
    rbfm.createFile("Tables");
    rbfm.createFile("Columns");

    this->tableCountInit(2);

    RID rid;
    // insert Tables
    FileHandle tableFile;
    rbfm.openFile("Tables",tableFile);
    uint8_t* tableData = new uint8_t [m_tableDataSize];
    tableformat(1, "Tables", "Tables", tableData);
    rbfm.insertRecord(tableFile,m_tablesDescriptor,tableData,rid);
    memset(tableData,0,m_tableDataSize);
    tableformat(2, "Columns", "Columns", tableData);
    rbfm.insertRecord(tableFile,m_tablesDescriptor,tableData,rid);
    rbfm.closeFile(tableFile);
    
    // insert column
    FileHandle columnFile;
    rbfm.openFile("Columns", columnFile);
    uint8_t* columnData = new uint8_t [m_columnDataSize];
    // For table's column
    columnformat(1, {"table-id", TypeInt, 4}, 1, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(1, {"table-name", TypeVarChar, 50}, 2, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(1, {"file-name", TypeVarChar, 50}, 3, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);
     // For column's column
    columnformat(2, {"table-id", TypeInt, 4}, 1, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(2, {"column-name", TypeVarChar, 50}, 2, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(2, {"column-type", TypeInt, 4}, 3, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(2, {"column-length", TypeInt, 4}, 4, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(2, {"column-position", TypeInt, 4}, 5, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);
    columnFile.closeFile();

    delete[] tableData;
    delete[] columnData;
    return 0;
}

RC RelationManager::deleteCatalog() {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    rbfm.destroyFile("Variables");
    rbfm.destroyFile("Tables");
    rbfm.destroyFile("Columns");
    return 0;
}

RC RelationManager::createTable(const std::string &tableName, const std::vector<Attribute> &attrs) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    if(rbfm.createFile(tableName) == 0) {
        RID rid;
        FileHandle tableFile;
        FileHandle columnFile;
        rbfm.openFile("Tables",tableFile);
        rbfm.openFile("Columns", columnFile);

        uint8_t* tableData = new uint8_t [m_tableDataSize];
        uint8_t* columnData = new uint8_t [m_columnDataSize];
        memset(tableData, 0, m_tableDataSize);
        memset(columnData, 0, m_columnDataSize);

        int tableID = getTableCount() + 1;
        this->addTableCount();

        //write table
        tableformat(tableID, tableName, tableName, tableData);
        rbfm.insertRecord(tableFile, m_tablesDescriptor, tableData, rid);

        // write coulmn
        for(int i = 0 ; i < attrs.size() ; i++) {
            memset(columnData, 0, m_columnDataSize);
            columnformat(tableID, attrs[i], i+1, columnData);
            rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
        }
        tableFile.closeFile();
        columnFile.closeFile();
        return 0;
    }
    std::cerr << "fail" << std::endl;
    return -1;
}

RC RelationManager::deleteTable(const std::string &tableName) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    RBFM_ScanIterator rbfm_it;
    FileHandle tableFile;
    RID deleteID;
    uint8_t* data = new uint8_t [m_tableDataSize];
    rbfm.openFile("Tables", tableFile);
    rbfm.scan(tableFile, m_tablesDescriptor, "tableName", EQ_OP, tableName.c_str(), {"tableName"},  rbfm_it);
    rbfm_it.getNextRecord(deleteID, data);
    rbfm.deleteRecord(tableFile, m_tablesDescriptor, deleteID);
    rbfm.destroyFile(tableName);
    tableFile.closeFile();
    return 0;
}

RC RelationManager::getAttributes(const std::string &tableName, std::vector<Attribute> &attrs) {
    uint8_t* tableData = new uint8_t [m_tableDataSize];
    uint8_t* columnData = new uint8_t [m_columnDataSize];
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    RBFM_ScanIterator rbfm_TB_it;
    int tableID;
    RID targetID;
    FileHandle tableFile;
    FileHandle columnFile;
    rbfm.openFile("Tables", tableFile);
    rbfm.scan(tableFile, m_tablesDescriptor, "tableName", EQ_OP, tableName.c_str(), {"tableId"},  rbfm_TB_it);
    rbfm_TB_it.getNextRecord(targetID, tableData);
    memcpy(&tableID, tableData+1, sizeof(int));
    tableFile.closeFile();

    RBFM_ScanIterator rbfm_Col_it;
    rbfm.openFile("Columns", columnFile);
    rbfm.scan(columnFile, m_collumnsDescriptor, "tableId", EQ_OP, &tableID, {"columnName","columnType","columnLength"}, rbfm_Col_it);
    
    RID rid;
    Attribute buffer;
    memset(&buffer, 0 , sizeof(buffer));
    while(rbfm_Col_it.getNextRecord(rid, columnData) != RBFM_EOF) {
        int stringSize;
        int attrType;
        int attrLength;
        memcpy(&stringSize, columnData+1, sizeof(int));
        char* stringBuffer = new char [stringSize + 1];
        memcpy(stringBuffer, columnData+5, stringSize);
        std::string attrName(stringBuffer, stringSize);
        memcpy(&attrType, columnData+5+stringSize, sizeof(int));
        memcpy(&attrLength, columnData+9+stringSize, sizeof(int));
        attrs.push_back({attrName, (AttrType)attrType, attrLength});
    }
    return 0;
}

RC RelationManager::insertTuple(const std::string &tableName, const void *data, RID &rid) {
    return -1;
}

RC RelationManager::deleteTuple(const std::string &tableName, const RID &rid) {
    return -1;
}

RC RelationManager::updateTuple(const std::string &tableName, const void *data, const RID &rid) {
    return -1;
}

RC RelationManager::readTuple(const std::string &tableName, const RID &rid, void *data) {
    return -1;
}

RC RelationManager::printTuple(const std::vector<Attribute> &attrs, const void *data) {
    return -1;
}

RC RelationManager::readAttribute(const std::string &tableName, const RID &rid, const std::string &attributeName,
                                  void *data) {
    return -1;
}

RC RelationManager::scan(const std::string &tableName,
                         const std::string &conditionAttribute,
                         const CompOp compOp,
                         const void *value,
                         const std::vector<std::string> &attributeNames,
                         RM_ScanIterator &rm_ScanIterator) {

    RecordBasedFileManager &recordBasedFileManager = RecordBasedFileManager::instance();

    //  TODO: get filePath by tableName
    //  TODO: create FileHandle by open filePath
    //  TODO: rbfm.scan(fileHandle)
    //recordBasedFileManager.openFile()
    //recordBasedFileManager.scan();
    return -1;
}

// Extra credit work
RC RelationManager::dropAttribute(const std::string &tableName, const std::string &attributeName) {
    return -1;
}

// Extra credit work
RC RelationManager::addAttribute(const std::string &tableName, const Attribute &attr) {
    return -1;
}

void RelationManager::getRecordDescriptor(const std::string &tableName, std::vector<Attribute> &recordDescriptor) {
    if(tableName == "Tables") {
        Attribute attr;
        attr.name = "tableId";
        attr.type = TypeInt;
        attr.length = (AttrLength) 4;
        recordDescriptor.push_back(attr);

        attr.name = "tableName";
        attr.type = TypeVarChar;
        attr.length = (AttrLength) 50;
        recordDescriptor.push_back(attr);

        attr.name = "fileName";
        attr.type = TypeVarChar;
        attr.length = (AttrLength) 50;
        recordDescriptor.push_back(attr);
    }
    else if(tableName == "Columns") {
        Attribute attr;
        attr.name = "tableId";
        attr.type = TypeInt;
        attr.length = (AttrLength) 4;
        recordDescriptor.push_back(attr);

        attr.name = "columnName";
        attr.type = TypeVarChar;
        attr.length = (AttrLength) 50;
        recordDescriptor.push_back(attr);

        attr.name = "columnType";
        attr.type = TypeInt;
        attr.length = (AttrLength) 4;
        recordDescriptor.push_back(attr);

        attr.name = "columnLength";
        attr.type = TypeInt;
        attr.length = (AttrLength) 4;
        recordDescriptor.push_back(attr);

        attr.name = "columnPosition";
        attr.type = TypeInt;
        attr.length = (AttrLength) 4;
        recordDescriptor.push_back(attr);
    }
    else {

    }
}

void RelationManager::tableformat(int id, std::string tableName, std::string fileName, uint8_t* data) {
    int tableNameSize =  tableName.size();
    int fileNameSize = fileName.size();
    uint8_t nullpart = 0;
    memcpy(data, &nullpart, sizeof(uint8_t));
    data+=1;
    memcpy(data, &id, sizeof(int));
    data+=sizeof(int);
    memcpy(data, &tableNameSize, sizeof(int));
    data+=sizeof(int);
    memcpy(data, tableName.c_str() , tableNameSize);
    data+=tableNameSize;
    memcpy(data, &fileNameSize , sizeof(int));
    data+=sizeof(int);
    memcpy(data, fileName.c_str() , fileNameSize);
}


void RelationManager::columnformat(int tableId, Attribute attribute, int columnPos, uint8_t* data) {

    std::string columnName = attribute.name; 
    AttrType columnType = attribute.type; 
    int columnLength = attribute.length;
    int columnNameSize =  columnName.size();
    uint8_t nullpart = 0;
    memcpy(data, &nullpart, sizeof(uint8_t));
    data+=1;
    memcpy(data, &tableId, sizeof(int));
    data+=sizeof(int);
    memcpy(data, &columnNameSize, sizeof(int));
    data+=sizeof(int);
    memcpy(data, columnName.c_str(), columnNameSize);
    data+=columnNameSize;
    memcpy(data, &columnType, sizeof(AttrType));
    data+=sizeof(AttrType);
    memcpy(data, &columnLength, sizeof(int));
    data+=sizeof(int);
    memcpy(data, &columnPos, sizeof(int));
}

void RelationManager::tableCountInit(int count) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    FileHandle varFile;
    RID rid;
    uint8_t* countData = new uint8_t [5];
    memset(countData, 0, 5);
    memcpy(countData+1, &count, sizeof(int));
    rbfm.openFile("Variables",varFile);
    rbfm.insertRecord(varFile, {{"count", TypeInt, 4}}, countData, rid);
    varFile.closeFile();
}

void RelationManager::addTableCount() {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    FileHandle varFile;
    int count = getTableCount() + 1;
    RID rid;
    uint8_t* countData = new uint8_t [5];
    memset(countData, 0, 5);
    memcpy(countData+1, &count, sizeof(int));
    rbfm.openFile("Variables",varFile);
    rbfm.updateRecord(varFile, {{"count", TypeInt, 4}}, countData, {0,0});
    varFile.closeFile();
}

int RelationManager::getTableCount() {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    FileHandle varFile;
    RID rid;
    int count; 
    uint8_t* countData = new uint8_t [5];
    rbfm.openFile("Variables",varFile);
    rbfm.readRecord(varFile, {{"count", TypeInt, 4}}, {0,0}, countData);
    varFile.closeFile();
    memcpy(&count, countData+1, sizeof(int));
    delete[] countData;
    return count;
}




