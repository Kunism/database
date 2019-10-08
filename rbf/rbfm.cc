#include <cmath>
#include <iostream>
#include "rbfm.h"
#include "pfm.h"

RecordBasedFileManager *RecordBasedFileManager::_rbf_manager = nullptr;

RecordBasedFileManager &RecordBasedFileManager::instance() {
    static RecordBasedFileManager _rbf_manager = RecordBasedFileManager();
    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager() = default;

RecordBasedFileManager::~RecordBasedFileManager() { delete _rbf_manager; }

RecordBasedFileManager::RecordBasedFileManager(const RecordBasedFileManager &) = default;

RecordBasedFileManager &RecordBasedFileManager::operator=(const RecordBasedFileManager &) = default;

RC RecordBasedFileManager::createFile(const std::string &fileName) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const std::string &fileName) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const std::string &fileName, FileHandle &fileHandle) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.openFile(fileName,fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.closeFile(fileHandle);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,const void *data, RID &rid) {
   return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                      const RID &rid, void *data) {
    return -1;
}

RC RecordBasedFileManager::printRecord(const std::vector<Attribute> &recordDescriptor, const void *data) {
    return -1;
}

//////////////////////////////////////////////////////////////
// PROJECT 1 TO HERE                                        //
//////////////////////////////////////////////////////////////


RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                        const RID &rid) {
    return -1;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                        const void *data, const RID &rid) {
    return -1;
}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                         const RID &rid, const std::string &attributeName, void *data) {
    return -1;
}

RC RecordBasedFileManager::scan(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                const std::string &conditionAttribute, const CompOp compOp, const void *value,
                                const std::vector<std::string> &attributeNames, RBFM_ScanIterator &rbfm_ScanIterator) {
    return -1;
}

Record::Record(const std::vector<Attribute> &_descriptor, const void* _data, RID &_rid) {
    
    this->descriptor = _descriptor;
    this->numOfField = _descriptor.size();
    this->indicatorSize = std::ceil((double)descriptor.size() /CHAR_BIT);
    
    this->indexData = new uint16_t [this->numOfField];
    this->nullData = new uint8_t [indicatorSize];
    memcpy(nullData, _data, indicatorSize);
    convertData(_data);
}

bool Record::isNull(int fieldNum) {
    int byteOffset = fieldNum / CHAR_BIT;
    int bitOffset = fieldNum % CHAR_BIT;

    return reinterpret_cast<uint8_t*>(this->nullData)[byteOffset] >> (7 - bitOffset) & 1;
}

//TypeInt = 0, TypeReal, TypeVarChar
void Record::convertData(const void* _data) {
    int size = 0;
    uint16_t byteOffset = Record::indexSize + this->indicatorSize + Record::indexSize * this->numOfField; 
    // // treat it as byte and move to data part
    const uint8_t* pos = reinterpret_cast<const uint8_t*>(_data) + this->indicatorSize; 
    for(int i = 0 ; i < this->numOfField ; i++ ) {
        if ((this->descriptor[i].type == TypeInt   || 
             this->descriptor[i].type == TypeReal) && 
             !isNull(i) ) {
            pos += 4;
            size += 4;
            indexData[i] = byteOffset + size;
        }
        else if (this->descriptor[i].type == TypeVarChar) {
            // byte to int
            uint32_t varCharSize = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
            pos += (4 + varCharSize);
            size += (4 + varCharSize);
            indexData[i] = byteOffset + size;
        }
    }
    this->recordData = new uint8_t [size];
    memcpy(recordData,(char*)_data+this->indicatorSize,size);
    this->dataSize = size;
    this->recordSize = Record::indexSize + this->indicatorSize + Record::indexSize * this->numOfField + this->dataSize;
                         
}

DataPage::DataPage(void* data) {
    memcpy(&var, (char*)data + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, sizeof(unsigned) * DATA_PAGE_VAR_NUM);

//    char* varBuffer = new char [sizeof(unsigned) * DATA_PAGE_VAR_NUM];
//    memcpy(varBuffer, (char*)data + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
//
//    var[HEADER_OFFSET_FROM_END] = ((unsigned*)varBuffer)[HEADER_OFFSET_FROM_END];
//    var[RECORD_OFFSET_FROM_BEGIN] = ((unsigned*)varBuffer)[RECORD_OFFSET_FROM_BEGIN];
//    var[SLOT_NUM] = ((unsigned*)varBuffer)[SLOT_NUM];

//    delete[](varBuffer);

    memcpy(&pageHeader, (char*)data, sizeof(std::pair<unsigned, unsigned>) * var[SLOT_NUM]);

//    char* headerBuffer = new char [sizeof(std::pair<unsigned, unsigned>) * var[SLOT_NUM]];
//    memcpy(headerBuffer, (char*)data, sizeof(std::pair<unsigned, unsigned>) * var[SLOT_NUM]);
    page = data;
}

DataPage::~DataPage() {

}

RID DataPage::writeRoecord(Record record, FileHandle fileHandle, unsigned availablePage) {

    std::pair<unsigned,unsigned> newRecordHeader;

    std::cout << record.recordSize << std::endl;
    unsigned offset = 0;
    char* newRecordContent = new char [record.recordSize];

    memcpy(newRecordContent + offset, &record.numOfField, record.indexSize);
    offset += record.indexSize;

    memcpy(newRecordContent + offset, record.nullData, record.indicatorSize);
    offset += record.indicatorSize;

    memcpy(newRecordContent + offset, &record.indexData, record.indexSize * record.numOfField);
    offset += record.indexSize * record.numOfField;

    memcpy(newRecordContent + offset, &record.recordData, record.dataSize);

    std::cout << offset << std::endl;

    memcpy((char*)page + var[RECORD_OFFSET_FROM_BEGIN], newRecordContent, record.recordSize);
    var[RECORD_OFFSET_FROM_BEGIN] += record.recordSize;

    memcpy((char*)page + PAGE_SIZE - var[HEADER_OFFSET_FROM_END] - sizeof(std::pair<uint16_t, uint16_t>),
           pageHeader + var[SLOT_NUM] - 1,
           sizeof(std::pair<uint16_t, uint16_t>));

    fileHandle.writePage(availablePage, page);
}

Record DataPage::readRecord(RID) {

}

unsigned DataPage::getFreeSpaceSize() {
    return PAGE_SIZE - var[RECORD_OFFSET_FROM_BEGIN] - var[HEADER_OFFSET_FROM_END];
}





