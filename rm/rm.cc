#include "rm.h"

RelationManager *RelationManager::_relation_manager = nullptr;

RelationManager &RelationManager::instance() {
    static RelationManager _relation_manager = RelationManager();
    return _relation_manager;
}

RelationManager::RelationManager() = default;

RelationManager::~RelationManager() { delete _relation_manager; }

RelationManager::RelationManager(const RelationManager &) = default;

RelationManager &RelationManager::operator=(const RelationManager &) = default;

RC RelationManager::createCatalog() {
    //TODO init cont descriptor 
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    rbfm.createFile("Tables");
    rbfm.createFile("Columns");
    RID rid;

    // insert Tables
    FileHandle TableFile;
    rbfm.openFile("Tables",TableFile);
    uint8_t* tableData = new uint8_t [m_tableDataSize];
    tableformat(1,"Tables","Tables",tableData);
    rbfm.insertRecord(TableFile,m_tablesDescriptor,tableData,rid);
    memset(tableData,0,m_tableDataSize);
    tableformat(2,"Columns","Columns",tableData);
    rbfm.insertRecord(TableFile,m_tablesDescriptor,tableData,rid);
    rbfm.closeFile(TableFile);
    
    // insert column
    FileHandle columnFile;
    rbfm.openFile("Columns", columnFile);
    uint8_t* columnData = new uint8_t [m_columnDataSize];
    // For table's column
    columnformat(1, "table-id", TypeInt, 4, 1, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(1, "table-name", TypeVarChar, 50, 2, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(1, "file-name", TypeVarChar, 50, 3, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);
     // For column's column
    columnformat(2, "table-id", TypeInt, 4, 1, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(2, "column-name", TypeVarChar, 50, 2, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(2, "column-type", TypeInt, 4, 3, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(2, "column-length", TypeInt, 4, 4, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(2, "column-position", TypeInt, 4, 5, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);


    delete[] tableData;
    delete[] columnData;
    return 0;
}

RC RelationManager::deleteCatalog() {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    rbfm.destroyFile("Tables");
    rbfm.destroyFile("Columns");
    return 0;
}

RC RelationManager::createTable(const std::string &tableName, const std::vector<Attribute> &attrs) {
    return -1;
}

RC RelationManager::deleteTable(const std::string &tableName) {
    return -1;
}

RC RelationManager::getAttributes(const std::string &tableName, std::vector<Attribute> &attrs) {
    getRecordDescriptor(tableName, attrs);
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


void RelationManager::columnformat(int tableId, std::string columnName, AttrType columnType, int columnLength,  int columnPos, uint8_t* data) {
    int columnNameSize =  columnName.size();
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




