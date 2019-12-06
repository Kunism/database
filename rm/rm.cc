#include "rm.h"
#include <iostream>
#include <algorithm>
RelationManager *RelationManager::_relation_manager = nullptr;

RelationManager &RelationManager::instance() {
    static RelationManager _relation_manager = RelationManager();
    return _relation_manager;
}

RC RM_ScanIterator::getNextTuple(RID &rid, void *data) {
    // std::cerr << "RM_ScanIterator::getNextTuple" <<std::endl;
    return rbfmScanIterator.getNextRecord(rid, data);
}

RC RM_ScanIterator::close() {
    // rbfmScanIterator.close();
    // fileHandle.closeFile();
}

RC RM_IndexScanIterator::getNextEntry(RID &rid, void *key) {
    if(ixScan_it.getNextEntry(rid, key) != 0) {
        return RM_EOF;
    }
    return 0;
}

RC RM_IndexScanIterator::close() {
    //  TODO
    return 0;
}

const std::vector<Attribute> RelationManager::m_indexDescriptor = {
    {
        "tableName",
        TypeVarChar,
        (AttrLength)50
    },
    {
        "attrName", //ColumnName
        TypeVarChar,
        (AttrLength)50
    }
};

const std::vector<Attribute> RelationManager::m_tablesDescriptor = {
    {
        "tableId",
        TypeInt,
        (AttrLength)4
    },        
    {
        "tableName",
        TypeVarChar,
        (AttrLength)50
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
    rbfm.createFile("Indexes");
    
    // Three catalog file: Tables, Columns, Indexes
    this->tableCountInit(3);

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
    memset(tableData,0,m_tableDataSize);
    tableformat(3, "Indexes", "Indexes", tableData);
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

    columnformat(3, {"tableName", TypeVarChar, 50}, 1, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
    memset(columnData, 0, m_columnDataSize);

    columnformat(3, {"attrName", TypeVarChar, 50}, 2, columnData);
    rbfm.insertRecord(columnFile, m_collumnsDescriptor, columnData, rid);
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
    rbfm.destroyFile("Indexes");
    return 0;
}

RC RelationManager::createTable(const std::string &tableName, const std::vector<Attribute> &attrs) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    RID rid;

    FileHandle tableFile;
    FileHandle columnFile;
    if (rbfm.openFile("Tables",tableFile) != 0 || rbfm.openFile("Columns", columnFile) != 0 ) {
        tableFile.closeFile();
        columnFile.closeFile();
        return -1;
    }

    if(rbfm.createFile(tableName) != 0) {
        tableFile.closeFile();
        columnFile.closeFile();
        return -1;
    }

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

    delete[] tableData;
    delete[] columnData;
    return 0;
}

RC RelationManager::deleteTable(const std::string &tableName) {
    if( tableName == "Tables" || tableName == "Columns") {
        return -1;
    }

    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    RBFM_ScanIterator table_it;
    RBFM_ScanIterator column_it;
    RBFM_ScanIterator index_it;
    FileHandle tableFile;
    FileHandle columnFile;
    FileHandle indexFile;
    RID deleteID;
    uint8_t* tableData = new uint8_t [m_tableDataSize];
    uint8_t* columnData = new uint8_t [m_columnDataSize];
    uint8_t* indexData = new uint8_t [m_indexDataSize];
    if (rbfm.openFile("Tables", tableFile)   == 0 && 
        rbfm.openFile("Columns", columnFile) == 0 && 
        rbfm.openFile("Indexes", indexFile)  == 0 ) {
        
        // delete Table's record
        uint8_t* value = new uint8_t [tableName.size()+ sizeof(int) + 1];
        prepareString(tableName, value);
        std::cerr << "\nTABLE: " << decodeString(value) <<std::endl;
        rbfm.scan(tableFile, m_tablesDescriptor, "tableName", EQ_OP, value, {"tableId"},  table_it);
        if (table_it.getNextRecord(deleteID, tableData) == RBFM_EOF) {
            return -1;
        }
        std::cerr << "TABLE: " << decodeString(value) <<std::endl;
        rbfm.deleteRecord(tableFile, m_tablesDescriptor, deleteID);
        rbfm.destroyFile(tableName);
        // delete Column's record
        rbfm.scan(columnFile, m_collumnsDescriptor, "tableId", EQ_OP, tableData+1, {"tableId"}, column_it);
        while(column_it.getNextRecord(deleteID, columnData) != RBFM_EOF) {
            rbfm.deleteRecord(columnFile, m_collumnsDescriptor, deleteID);
        }

        // delete Index's record
        rbfm.scan(indexFile, m_indexDescriptor, "tableName", EQ_OP, value, {"tableName"}, index_it);
        while( index_it.getNextRecord(deleteID, indexData) != RBFM_EOF) {
            std::cerr << "HI" << std::endl;
            rbfm.deleteRecord(indexFile, m_indexDescriptor, deleteID);
        }

        table_it.close();
        column_it.close();
        index_it.close();

        tableFile.closeFile();

        delete [] value;
        delete [] tableData;
        delete [] columnData;
        return 0;
    }
    delete [] tableData;
    delete [] columnData;
    return -1;
}

RC RelationManager::getAttributes(const std::string &tableName, std::vector<Attribute> &attrs) {
    uint8_t* tableData = new uint8_t [m_tableDataSize+1];
    uint8_t* columnData = new uint8_t [m_columnDataSize+1];
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    RBFM_ScanIterator rbfm_TB_it;
    int tableID = 87;
    RID targetID;
    FileHandle tableFile;
    FileHandle columnFile;
    if (rbfm.openFile("Tables", tableFile) != 0 || rbfm.openFile("Columns", columnFile) != 0) {
        // std::cerr << "OPEN Catalog fail" << std::endl;
        return -1;
    }
    uint8_t* value = new uint8_t [tableName.size()+ sizeof(int) + 1];
    prepareString(tableName, value);
    rbfm.scan(tableFile, m_tablesDescriptor, "tableName", EQ_OP, value, {"tableId"},  rbfm_TB_it);
    if(rbfm_TB_it.getNextRecord(targetID, tableData) == RBFM_EOF) {
        // std::cerr << "GET TABLE ID  FAIL!!" <<std::endl;
        return -1;
    }
    memcpy(&tableID, tableData+1, sizeof(int));
    

    RBFM_ScanIterator rbfm_Col_it;
    uint8_t* idValue = new uint8_t [sizeof(int) + 1];

    prepareInt(tableID, idValue);
    rbfm.scan(columnFile, m_collumnsDescriptor, "tableId", EQ_OP, &tableID, {"columnName","columnType","columnLength"}, rbfm_Col_it);
    
    RID rid;
    while(rbfm_Col_it.getNextRecord(rid, columnData) != RBFM_EOF) {
        int stringSize;
        int attrType;
        int attrLength;
        memcpy(&stringSize, columnData+1, sizeof(int));
        char* stringBuffer = new char [stringSize + 100];
        memcpy(stringBuffer, columnData+5, stringSize);
        memcpy(&attrType, columnData+stringSize+5, sizeof(int));
        memcpy(&attrLength, columnData+stringSize+9, sizeof(int));
        std::string attrName(stringBuffer, stringSize);
        attrs.push_back({attrName, (AttrType)attrType, (AttrLength)attrLength});
        delete[] stringBuffer;
    }

    delete[] columnData;
    delete[] tableData;
    delete[] value;
    delete[] idValue;
    columnFile.closeFile();
    tableFile.closeFile();
    return 0;
}

RC RelationManager::insertIndexes(const std::string &tableName, const RID &rid) {

    std::vector<Attribute> attrs;
    getAttributes(tableName, attrs);

    IndexManager &ix = IndexManager::instance();
    RM_ScanIterator rm_it;
    uint8_t* tableNameData = new uint8_t [m_tableDataSize];    
    prepareString(tableName, tableNameData);
    scan("Indexes", "tableName", EQ_OP, tableNameData, {"attrName"}, rm_it);
    
    RID returnRID;
    uint8_t* returnData = new uint8_t [m_indexDataSize];
    ////////////////////////////////////////////////////
    // TODO:: DEAL WITH NULL ???
    ////////////////////////////////////////////////////
    while(rm_it.getNextTuple(returnRID, returnData) != RM_EOF) {
        std::string attrName = decodeString(returnData+1);
        // std::cerr << "Get Index Name: " << attrName << std::endl;
        Attribute targetAttr;
        for(auto attr : attrs) {
            if(attr.name == attrName) {
                targetAttr = attr;
            }
        }
        std::string indexFileName = tableName+"_"+attrName+".index";
        IXFileHandle ixFileHandle;
        uint8_t* keyData = new uint8_t [targetAttr.length + sizeof(int) + 1];
        if(ix.openFile(indexFileName, ixFileHandle) != 0) {
            std::cerr << "RelationManager::insertIndexes open file fail" <<std::endl;
            return -1;
        }

        if(readAttribute(tableName, rid, attrName, keyData) != 0) {
            std::cerr << "RelationManager::insertIndexes readAttribute fail" <<std::endl;
            return -1;
        }

        if(ix.insertEntry(ixFileHandle, targetAttr, keyData+1, rid) != 0) {
            std::cerr << "RelationManager::insertIndexes insertEntry fail" <<std::endl;
            return -1;
        }
        delete[] keyData;
    }
    delete[] tableNameData;
    delete[] returnData;
    rm_it.close();
    return 0;
}

RC RelationManager::insertTuple(const std::string &tableName, const void *data, RID &rid) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    FileHandle targetFile;
    if( tableName == "Tables" || tableName == "Columns") {
        return -1;
    }

    std::vector<Attribute> attrs;
    this->getAttributes(tableName,attrs);
    if (rbfm.openFile(tableName, targetFile) == 0) {
        rbfm.insertRecord(targetFile, attrs, data, rid);
        this->insertIndexes(tableName, rid);

        targetFile.closeFile();
        return 0;
    }
    else {
        // std::cerr << "insertTuple: file open failed!! " <<std::endl;
        return -1;
    }
}


RC RelationManager::deleteIndexes(const std::string &tableName, const RID &rid) {
    std::vector<Attribute> attrs;
    getAttributes(tableName, attrs);

    IndexManager &ix = IndexManager::instance();
    RM_ScanIterator rm_it;
    uint8_t* tableNameData = new uint8_t [m_tableDataSize];    
    prepareString(tableName, tableNameData);
    scan("Indexes", "tableName", EQ_OP, tableNameData, {"attrName"}, rm_it);

    RID returnRID;
    uint8_t* returnData = new uint8_t [m_indexDataSize];
    ////////////////////////////////////////////////////
    // DEAL WITH NULL ???
    ////////////////////////////////////////////////////
    while(rm_it.getNextTuple(returnRID, returnData) != RM_EOF) {
        std::string attrName = decodeString(returnData+1);
        Attribute targetAttr;
        for(auto attr : attrs) {
            if(attr.name == attrName) {
                targetAttr = attr;
            }
        }
        std::string indexFileName = tableName+"_"+attrName+".index";
        IXFileHandle ixFileHandle;
        uint8_t* keyData = new uint8_t [targetAttr.length + sizeof(int) + 1];
        ix.openFile(indexFileName, ixFileHandle);

        readAttribute(tableName, rid, attrName, keyData);
        ix.deleteEntry(ixFileHandle, targetAttr, keyData+1, rid);
    }

    rm_it.close();
    return 0;
}

RC RelationManager::deleteTuple(const std::string &tableName, const RID &rid) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    FileHandle targetFile;
    std::vector<Attribute> attrs;

    int totalSize = 0;
    for(auto attr : attrs) {
        totalSize = (attr.length + 5);
    }

    this->getAttributes(tableName,attrs);
    if (rbfm.openFile(tableName, targetFile) == 0) {
        uint8_t* data = new uint8_t [totalSize];
        if( deleteIndexes(tableName, rid) != 0) {
            return -1;
        }

        if( rbfm.deleteRecord(targetFile, attrs, rid) != 0) {
            return -1;
        }
        targetFile.closeFile();  
        delete[] data;  
        return 0;   
    }
    return -1;
}

RC RelationManager::updateTuple(const std::string &tableName, const void *data, const RID &rid) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    FileHandle targetFile;
    std::vector<Attribute> attrs;
    ///////////////////////////////////////////////////////
    //// delete readAttr key                        ///////
    ////  insert RID key                            ///////
    //// insert index                               ///////
    ///////////////////////////////////////////////////////
    this->getAttributes(tableName,attrs);
    if (rbfm.openFile(tableName, targetFile) == 0) {
        if (deleteIndexes(tableName, rid) != 0) {
            std::cerr << "delete index fail" <<std::endl;
            return -1;
        }
        if(rbfm.updateRecord(targetFile, attrs, data, rid) != 0) {
            std::cerr << "update record fail" << std::endl;
            return -1;
        }

        if( insertIndexes(tableName, rid) != 0) {
            std::cerr << "insert index fail " <<std::endl;
            return -1;
        }
        targetFile.closeFile();
        return 0;   
    }
    return -1;
}

RC RelationManager::readTuple(const std::string &tableName, const RID &rid, void *data) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    FileHandle targetFile;
    std::vector<Attribute> attrs;
    this->getAttributes(tableName,attrs);
    if (rbfm.openFile(tableName, targetFile) != 0) {
        // std::cerr << "CANNOT OPEN FILE" <<std::endl;
        return -1;
    }
    if (rbfm.readRecord(targetFile, attrs, rid, data) != 0 ) {
        // std::cerr << "CANNOT READ FILE" <<std::endl;
        return -1;
    }
        targetFile.closeFile();
    return 0;
}

RC RelationManager::printTuple(const std::vector<Attribute> &attrs, const void *data) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    rbfm.printRecord(attrs, data);
    return 0;
}

RC RelationManager::readAttribute(const std::string &tableName, const RID &rid, const std::string &attributeName,
                                  void *data) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    FileHandle targetFile;
    std::vector<Attribute> attrs;
    this->getAttributes(tableName,attrs);
    if (rbfm.openFile(tableName, targetFile) == 0 && 
        rbfm.readAttribute(targetFile, attrs, rid, attributeName, data) == 0 ) {
        targetFile.closeFile();
        return 0;   
    }
    return -1;
}

RC RelationManager::scan(const std::string &tableName,
                         const std::string &conditionAttribute,
                         const CompOp compOp,
                         const void *value,
                         const std::vector<std::string> &attributeNames,
                         RM_ScanIterator &rm_ScanIterator) {

    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    std::vector<Attribute> attrs;
    this->getAttributes(tableName,attrs);
    
    RC rc;
    rc = rbfm.openFile(tableName, rm_ScanIterator.fileHandle);
    if( rc != 0) {
        // std::cerr << "scan open file failed!" << std::endl;
        return -1;
    }

    rc = rbfm.scan(rm_ScanIterator.fileHandle, attrs, conditionAttribute, compOp, value, attributeNames, rm_ScanIterator.rbfmScanIterator);
    return 0;
}

// Extra credit work
RC RelationManager::dropAttribute(const std::string &tableName, const std::string &attributeName) {
    return -1;
}

// Extra credit work
RC RelationManager::addAttribute(const std::string &tableName, const Attribute &attr) {
    return -1;
}


// mapping tableName to "attribute Name"
void RelationManager::indexformat(std::string tableName, std::string attrName, uint8_t* data)
{
    int tableNameSize = tableName.size();
    int attrNameSize = attrName.size();

    uint8_t nullpart = 0;
    memcpy(data, &nullpart, sizeof(uint8_t));
    data+=1;
    memcpy(data, &tableNameSize, sizeof(int));
    data+=sizeof(int);
    memcpy(data, tableName.c_str(), tableNameSize);
    data+=tableNameSize;
    memcpy(data, &attrNameSize, sizeof(int));
    data+=sizeof(int);
    memcpy(data, attrName.c_str(), attrNameSize);
    data+=attrNameSize;
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


void RelationManager::prepareString(const std::string &s, uint8_t* data) {
    int size = s.size();
    memcpy(data, &size, sizeof(int));
    data+=sizeof(int);
    memcpy(data, s.c_str(), s.size());
}


// data* not include null indicator part
std::string RelationManager::decodeString(uint8_t* data) {
    int size = 0;
    memcpy(&size, data, sizeof(int)); ///////////////////////////////////////////////
    return std::string((char*)(data+4), size);
}

void RelationManager::prepareInt(const int i, uint8_t* data) {
    memcpy(data, &i, sizeof(int));
}


// QE IX related

RC RelationManager::createIndex(const std::string &tableName, const std::string &attributeName) {
    RecordBasedFileManager &rbfm = RecordBasedFileManager::instance();
    IndexManager &ix = IndexManager::instance();
    
    FileHandle indexMapping;
    IXFileHandle indexFile;
    std::string indexFileName = tableName + "_" + attributeName + ".index";
    if (ix.createFile(indexFileName) != 0 || ix.openFile(indexFileName, indexFile) != 0) {
        return -1;
    }
    
    RM_ScanIterator rm_it;
    Attribute insertAttr;
    std::vector<Attribute> attrs;
    getAttributes(tableName, attrs);
    for(auto attr : attrs) {
        if(attr.name == attributeName) {
            insertAttr = attr;
        }
    }
   
    RID rid;
    uint8_t* key = new uint8_t [insertAttr.length + sizeof(int) + 1];
    this->scan(tableName, "", NO_OP, NULL, {attributeName}, rm_it);
    while (rm_it.getNextTuple(rid, key) != RM_EOF)
    {
        ix.insertEntry(indexFile, insertAttr, key+1, rid);
    }

    uint8_t* indexData = new uint8_t [m_indexDataSize];
    indexformat(tableName, attributeName, indexData);
    rbfm.openFile("Indexes", indexMapping);
    rbfm.insertRecord(indexMapping, m_indexDescriptor, indexData, rid);


    delete[] key;
    delete[] indexData;
    return 0;
}


RC RelationManager::destroyIndex(const std::string &tableName, const std::string &attributeName) {
    IndexManager &indexManager = IndexManager::instance();
    std::string indexFileName = tableName + "_" + attributeName + ".index";
    if(indexManager.destroyFile(indexFileName) != 0) {
        return -1;
    }
    RID rid;
    RM_ScanIterator rm_it;
    uint8_t* tableNameData = new uint8_t [tableName.size()+ sizeof(int) + 1];
    uint8_t* returnData = new uint8_t [m_indexDataSize];
    prepareString(tableName, tableNameData);
    scan("Indexes", "tableName", EQ_OP, tableNameData, {"attrName"}, rm_it);

    //////////////////////////////////////////////////////////////
    //////////////////////TOODOOOOOOOOOO//////////////////////////
    //////////////////////////////////////////////////////////////
    while(rm_it.getNextTuple(rid, returnData) != RM_EOF) {
        if(memcmp(returnData+sizeof(int)+1, attributeName.c_str(), attributeName.size()) == 0) {
            deleteTuple("Indexes", rid);
        }
    }

    return 0;
}

RC RelationManager::indexScan(const std::string &tableName,
                              const std::string &attributeName,
                              const void *lowKey,
                              const void *highKey,
                              bool lowKeyInclusive,
                              bool highKeyInclusive,
                              RM_IndexScanIterator &rm_IndexScanIterator) {
    std::cerr << "RM:IX:SCAN " << std::endl;                                  
    std::vector<Attribute> attrs;
    getAttributes(tableName, attrs);

    Attribute targetAttr;
    for(auto attr : attrs) {
        if(attr.name == attributeName) {
            targetAttr = attr;
        }
    }

    IndexManager &indexManager = IndexManager::instance();
    std::string indexFileName = tableName + "_" + attributeName+ ".index";
    if(indexManager.openFile(indexFileName, rm_IndexScanIterator.ixFileHandle) != 0) {
        std::cerr<< "open file fail" <<std::endl;
        return -1;
    }

    if(indexManager.scan(rm_IndexScanIterator.ixFileHandle, targetAttr, lowKey, highKey, lowKeyInclusive, highKeyInclusive,  rm_IndexScanIterator.ixScan_it) == -1) {
        //std::cerr << "RM::indexScan scan return -1" << std::endl;
        return -1;
    }
    return 0;
}





