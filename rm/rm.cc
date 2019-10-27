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
    RecordBasedFileManager &recordBasedFileManager = RecordBasedFileManager::instance();
    recordBasedFileManager.createFile("Tables");
    recordBasedFileManager.createFile("Columns");
    return -1;
}

RC RelationManager::deleteCatalog() {
    return -1;
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


