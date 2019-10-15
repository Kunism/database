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
    Record record(recordDescriptor, data, rid);

    if(fileHandle.getNumberOfPages() == 0) {
        void* buf = new char [PAGE_SIZE];
        unsigned var[3] = {sizeof(unsigned) * 3, 0, 0};
        memcpy((char*)buf + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
        fileHandle.appendPage(buf);
    }
    int lastPageNum = fileHandle.getNumberOfPages()-1;
    void* pageData = new uint8_t [PAGE_SIZE];
    fileHandle.readPage(lastPageNum,pageData);
    DataPage page(pageData);

    if( record.recordSize +4 <= page.getFreeSpaceSize()) {
        page.writeRecord(record,fileHandle,lastPageNum, rid);
        record.rid = rid;
        return 0;
    }
    else {
        for(int i = 0 ; i < lastPageNum ;i++) {
            fileHandle.readPage(i,pageData);
            DataPage temp(pageData);
            if( record.recordSize +4<= temp.getFreeSpaceSize()) {
                temp.writeRecord(record,fileHandle, i, rid);
                record.rid = rid;
                return 0;
            }
        }
        void* buffer = new char [PAGE_SIZE];
        unsigned var[3] = {sizeof(unsigned) * 3, 0, 0};
        memcpy((char*)buffer + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
        fileHandle.appendPage(buffer);
        DataPage newPage(buffer);
        newPage.writeRecord(record, fileHandle, lastPageNum + 1, rid);
        record.rid = rid;
        return 0;
    }
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,const RID &rid, void *data) {
    void* pageData = new uint8_t [PAGE_SIZE];
    if(fileHandle.readPage(rid.pageNum,pageData) == 0) {
        DataPage dataPage(pageData);
        dataPage.readRecord(fileHandle, rid, data);
        return 0;
    }
    return -1;
}

RC RecordBasedFileManager::printRecord(const std::vector<Attribute> &recordDescriptor, const void *data) {
    RID fakeRid = {0,0};
    Record record(recordDescriptor,data,fakeRid);

    uint16_t byteOffset = Record::indexSize * (1 + recordDescriptor.size());
    for(int i =  0 ; i < recordDescriptor.size() ; i++) {
        
        std::cout << recordDescriptor[i].name << ": ";
        if(record.isNull(i)) {
            std::cout << "NULL ";
        }
        else {
            uint16_t diff = record.indexData[i] - byteOffset;
            const uint8_t* pos = reinterpret_cast<const uint8_t*>(data);
            pos += diff;
            
            switch (recordDescriptor[i].type) {
                case TypeInt: {
                    uint32_t intValue;
                    memcpy(&intValue, pos, 4);
                    std::cout << intValue;
                    break;
                }
                case TypeReal: {
                    float realValue;
                    memcpy(&realValue, pos, sizeof(float));
                    std::cout << realValue;
                    break;
                }
                case TypeVarChar: {
                    uint32_t sizeValue;
                    memcpy(&sizeValue, pos, 4);
                    char* s = new char [static_cast<int>(sizeValue) + 1];
                    memcpy(s,pos+4,static_cast<int>(sizeValue));
                    s[static_cast<int>(sizeValue)] = '\0';
                    std::cout << s;
                    break;
                }
            }
        }
        if(i != recordDescriptor.size() -1) {
            std::cout << ' ';
        } else {
            std::cout << std::endl;
        }
    }

    return 0;

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
    char* buffer = new char [PAGE_SIZE];
    fileHandle.readPage(rid.pageNum, buffer);
    DataPage page(buffer);

    RID newRid = {rid.pageNum, rid.slotNum};
    Record newRecord(recordDescriptor, data, newRid);

    //  Space enough to update new record directly
    if(page.getRecordLength(newRid) >= newRecord.recordSize) {
        //  TODO: replace record
        //  TODO: move other records forward
    }

    //  Space enough to update, but need to rearrange
    else if((page.getRecordLength(newRid) + page.getFreeSpaceSize()) >= newRecord.recordSize) {
        //  TODO: move other records backward
        //  TODO: replace record
    }

    //  Space is not enough to update, only enough to put a tombstone
    else if(page.getRecordLength(newRid) >= sizeof(Tombstone)) {
        //  TODO: replace old record to tombstone
        //  TODO: move other records forward

        // create tombstone(flag bits + page num + slot num)
    }

    //  Space is not enough to update, need to rearrange free space for a tombstone
    else if((page.getRecordLength(newRid) + page.getFreeSpaceSize()) >= sizeof(Tombstone)) {
        //  TODO: move other records backward
        //  TODO: repace record to tombstone
    }

    //  No space for even a tombstone
    else {
        //  TODO: keep replace existing records to tombstones until there is enough space for a new tombstone
    }

    delete[] buffer;
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

Record::~Record() {
    delete[] this->indexData;
    delete[] this->nullData;
    delete[] this->recordData;
}


//TypeInt = 0, TypeReal, TypeVarChar
void Record::convertData(const void* _data) {
    uint16_t size = 0;
    uint16_t byteOffset = Record::indexSize + this->indicatorSize + Record::indexSize * this->numOfField;
    
    // // treat it as byte and move to data part
    const uint8_t* pos = reinterpret_cast<const uint8_t*>(_data) + (uint8_t)this->indicatorSize; 
    
    for(int i = 0 ; i < this->numOfField ; i++ ) {

        indexData[i] = byteOffset + size;
        if ( this->descriptor[i].type == TypeInt && !isNull(i) ) {
            pos += sizeof(int);
            size += sizeof(int);
        }
        else if (this->descriptor[i].type == TypeReal && !isNull(i)) {
            pos += sizeof(float);
            size += sizeof(float);
        }
        else if (this->descriptor[i].type == TypeVarChar && !isNull(i)) {
            // byte to int
            uint32_t varCharSize; 
            memcpy(&varCharSize, pos, sizeof(uint32_t));
            pos += (4 + varCharSize);
            size += (4 + varCharSize);
        }
    }

    this->recordData = new uint8_t [size];
    memcpy(recordData,(char*)_data+this->indicatorSize,size);
    this->dataSize = size;
    this->recordSize = Record::indexSize + this->indicatorSize + Record::indexSize * this->numOfField + this->dataSize;
                         
}

DataPage::DataPage(const void* data) {
    memcpy(&var, (char*)data + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, sizeof(unsigned) * DATA_PAGE_VAR_NUM);

    page = new char [PAGE_SIZE];
    memcpy(page, data, PAGE_SIZE);
    
    pageHeader = new std::pair<uint16_t , uint16_t> [var[SLOT_NUM]];
    memcpy(pageHeader, (char*)data + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM - 
                        sizeof(std::pair<uint16_t, uint16_t>) * var[SLOT_NUM],
                        sizeof(std::pair<uint16_t, uint16_t>) * var[SLOT_NUM]);

}

DataPage::~DataPage() {
    delete[] pageHeader;
    delete[] page;
}

void DataPage::writeRecord(Record &record, FileHandle &fileHandle, unsigned availablePage, RID &rid ) {
    std::pair<uint16_t ,uint16_t> newRecordHeader;

    uint16_t offset = 0;
    char* newRecordContent = new char [record.recordSize];
    // prepare for record
    memcpy(newRecordContent + offset, &record.numOfField, record.indexSize);
    offset += record.indexSize;

    memcpy(newRecordContent + offset, record.nullData, record.indicatorSize);
    offset += record.indicatorSize;

    memcpy(newRecordContent + offset, record.indexData, record.indexSize * record.numOfField);
    offset += record.indexSize * record.numOfField;

    memcpy(newRecordContent + offset, record.recordData, record.dataSize);
    offset += record.dataSize;

    // write record
    memcpy((char*)page + var[RECORD_OFFSET_FROM_BEGIN], newRecordContent, record.recordSize);

    newRecordHeader = {var[RECORD_OFFSET_FROM_BEGIN], offset};

    // write index,length
    memcpy((char*)page + PAGE_SIZE - var[HEADER_OFFSET_FROM_END] - sizeof(std::pair<uint16_t, uint16_t>), &newRecordHeader, sizeof(std::pair<uint16_t, uint16_t>));

    // update header
    rid = {availablePage,var[SLOT_NUM]};
    var[RECORD_OFFSET_FROM_BEGIN] += record.recordSize;
    var[HEADER_OFFSET_FROM_END] += sizeof(std::pair<uint16_t, uint16_t>);
    var[SLOT_NUM]++;
    memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);

    fileHandle.writePage(availablePage, page);
}

void DataPage::readRecord(FileHandle& fileHandle, const RID& rid, void* data) {

    void* indexPos = reinterpret_cast<uint8_t*>(page) + PAGE_SIZE -                 // end of PAGE file
                     sizeof(unsigned) * DATA_PAGE_VAR_NUM  -                        // metadata
                     (rid.slotNum +1) * sizeof(std::pair<uint16_t, uint16_t>);      // No. i index Offset;

    // get position of head
    std::pair<uint16_t,uint16_t> indexPair = reinterpret_cast<std::pair<uint16_t,uint16_t>*>(indexPos)[0];
    uint16_t indexValue = indexPair.first;
    uint16_t lenValue = indexPair.second;

    // interpret index
    uint8_t* recordPos =  reinterpret_cast<uint8_t*>(page) + indexValue;
    uint16_t numOfField = recordPos[1] << 8 | recordPos[0];
    uint8_t* nullPos = recordPos + Record::indexSize;
    uint16_t indicatorSize = ceil(static_cast<double>(numOfField)/CHAR_BIT);
    uint8_t* dataPos = nullPos + indicatorSize + Record::indexSize * numOfField;
    
    memcpy(data, nullPos, indicatorSize);
    memcpy((char*)data+indicatorSize, dataPos, lenValue-Record::indexSize * (1+numOfField));
}

void DataPage::updateRecord(Record &record, FileHandle &fileHandle, unsigned availablePage, const RID &rid ) {

}

unsigned DataPage::getFreeSpaceSize() {
    return PAGE_SIZE - var[RECORD_OFFSET_FROM_BEGIN] - var[HEADER_OFFSET_FROM_END];
}

uint16_t DataPage::getRecordLength(const RID &rid) {
    std::pair<uint16_t, uint16_t> targetSlot;
    memcpy(&targetSlot, (char*)page + PAGE_SIZE - sizeof(var) - sizeof(std::pair<uint16_t, uint16_t>) * (rid.slotNum + 1), sizeof(std::pair<uint16_t, uint16_t>));
    return targetSlot.second;
}

uint32_t DataPage::getNextAvailablePageNum(Record& record, FileHandle& fileHandle, const RID& rid) {

    //  Loop all pages to find a page with enough space to relocate record, return the page number
    for(uint32_t i = 0; i < fileHandle.getNumberOfPages(); i++) {
        void* pageData = new char[PAGE_SIZE];
        fileHandle.readPage((rid.pageNum + i) % fileHandle.getNumberOfPages(), pageData);
        DataPage page(pageData);
        if(record.recordSize <= page.getFreeSpaceSize()) {
            delete[] pageData;
            return (rid.pageNum + i) % fileHandle.getNumberOfPages();
        }
        delete[] pageData;
    }

    //  All pages are full, append new blank page and return page number
    void* buf = new char [PAGE_SIZE];
    unsigned var[3] = {sizeof(unsigned) * 3, 0, 0};
    memcpy((char*)buf + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    fileHandle.appendPage(buf);

    return fileHandle.getNumberOfPages()-1;
}

DataPage::DataPage(const DataPage& datapage) {
}


DataPage& DataPage::operator=(const DataPage &dataPage) {
}
