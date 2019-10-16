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

    // TODO do DELTED_MASK && TOMB_MASK

    void* pageData = new uint8_t [PAGE_SIZE];
    if(fileHandle.readPage(rid.pageNum,pageData) == 0) {
        DataPage dataPage(pageData);
        // auto pair = dataPage.getIndexPair(rid.slotNum);
        // check whether need to jump;
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

uint32_t RecordBasedFileManager::getNextAvailablePageNum(Record& record, FileHandle& fileHandle, const uint32_t& pageFrom) {

    //  Loop all pages to find a page with enough space to relocate record, return the page number
    for(uint32_t i = 0; i < fileHandle.getNumberOfPages() - 1; i++) {
        void* pageData = new char[PAGE_SIZE];
        fileHandle.readPage((pageFrom + i) % fileHandle.getNumberOfPages(), pageData);
        DataPage page(pageData);
        if(record.recordSize <= page.getFreeSpaceSize()) {
            delete[] pageData;
            return (pageFrom + i) % fileHandle.getNumberOfPages();
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

//////////////////////////////////////////////////////////////
// PROJECT 1 TO HERE                                        //
//////////////////////////////////////////////////////////////


RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                        const RID &rid) {
    return -1;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, 
                                        const std::vector<Attribute> &recordDescriptor,
                                        const void *data, const RID &rid) {

    char* buffer = new char [PAGE_SIZE];
    fileHandle.readPage(rid.pageNum, buffer);
    DataPage page(buffer);

    Record newRecord(recordDescriptor,data, rid);
    std::pair<uint16_t, uint16_t> indexPair = page.getIndexPair(rid.slotNum);

    uint16_t index = indexPair.first;
    uint16_t length = indexPair.second;

    int16_t diff = newRecord.recordSize - indexPair.second;
    if (diff <= page.getFreeSpaceSize())
    {
        page.shiftRecords(indexPair.first+indexPair.second, diff);
        page.updateRecord(newRecord,fileHandle,rid);
    } 

    //  Space is not enough to update, only enough to put a tombstone
    else if(indexPair.second >= sizeof(Tombstone)) {
        //  TODO: replace old record to tombstone
        //  TODO: move other records forward

        // create tombstone(flag bits + page num + slot num)

        char* availablePage = new char [PAGE_SIZE];
        uint32_t availablePageNum = getNextAvailablePageNum(newRecord, fileHandle, rid.pageNum + 1);
        fileHandle.readPage(availablePageNum, availablePage);
        uint16_t offsetFromBegin;
        memcpy(&offsetFromBegin, availablePage + PAGE_SIZE - sizeof(uint16_t) * 2, sizeof(uint16_t));
        Tombstone tombstone = {0x80, availablePageNum, offsetFromBegin};
        //  TODO: put tombstone
        //  TODO: shift


        writeRecordFromTombstone(newRecord, fileHandle, availablePageNum, offsetFromBegin, tombstone);
        delete[] availablePage;
    }

    //  Space is not enough to update, need to rearrange free space for a tombstone
    else if((indexPair.second + page.getFreeSpaceSize()) >= sizeof(Tombstone)) {
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

RC RecordBasedFileManager::writeRecordFromTombstone(Record& record, FileHandle& fileHandle, uint32_t availablePageNum, uint16_t offsetFromBegin, const Tombstone& tombstone) {
    void* buffer = new char [PAGE_SIZE];
    fileHandle.readPage(availablePageNum, buffer);
    DataPage page(buffer);
    //page.writeRecordFromTombstone(fileHandle, data, recordSize, offsetFromBegin);
}

Record::Record(const std::vector<Attribute> &_descriptor, const void* _data, const RID &_rid) {
    
    this->descriptor = _descriptor;
    this->numOfField = _descriptor.size();
    this->indicatorSize = std::ceil((double)descriptor.size() /CHAR_BIT);
    //TODO: need to store rid?
    this->nullData = reinterpret_cast<const uint8_t*>(_data);
    this->indexData = new uint16_t [this->numOfField];
    convertData(_data);
}

bool Record::isNull(int fieldNum) {
    int byteOffset = fieldNum / CHAR_BIT;
    int bitOffset = fieldNum % CHAR_BIT;

    return this->nullData[byteOffset] >> (7 - bitOffset) & 1;
}

Record::~Record() {
     delete[] recordHead;
    // delete[] this->nullData;
    // delete[] this->recordData;
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
    //calculate the total record size;
    this->recordSize = Record::indexSize + this->indicatorSize + Record::indexSize * this->numOfField + size + Record::paddingSize;
    this->recordHead = new uint8_t [this->recordSize];
    
    uint16_t offset = 0;
    // num of fields
    memcpy(recordHead + offset, &this->numOfField, Record::indexSize);
    offset += Record::indexSize;
    // null indicator part
    memcpy(recordHead + offset, _data, this->indicatorSize);
    offset += this->indicatorSize;
    // indexing part
    memcpy(recordHead + offset, indexData, Record::indexSize * this->numOfField);
    offset += Record::indexSize * this->numOfField;
    // content part
    memcpy(recordHead + offset, reinterpret_cast<const uint8_t*>(_data)+this->indicatorSize, size);
    offset += size;

    delete[] indexData; 
    this->nullData = recordHead + Record::indexSize;
    this->indexData = reinterpret_cast<uint16_t*>(recordHead + Record::indexSize + this->indicatorSize);
}

const uint8_t* Record::getRecord() const {
    return this->recordHead;
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

// encode record
void DataPage::writeRecord(Record &record, FileHandle &fileHandle, unsigned availablePage, RID &rid ) {

    // write record
    memcpy((char*)page + var[RECORD_OFFSET_FROM_BEGIN], record.getRecord(), record.recordSize);

    // write index,length
    std::pair<uint16_t ,uint16_t> newRecordHeader;
    newRecordHeader = {var[RECORD_OFFSET_FROM_BEGIN], record.recordSize};
    memcpy((char*)page + PAGE_SIZE - var[HEADER_OFFSET_FROM_END] - sizeof(std::pair<uint16_t, uint16_t>), &newRecordHeader, sizeof(std::pair<uint16_t, uint16_t>));

    // update header
    rid = {availablePage,var[SLOT_NUM]};
    var[RECORD_OFFSET_FROM_BEGIN] += record.recordSize;
    var[HEADER_OFFSET_FROM_END] += sizeof(std::pair<uint16_t, uint16_t>);
    var[SLOT_NUM]++;
    // write header
    memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    // write page
    fileHandle.writePage(availablePage, page);
    // std::cerr << "DataPage: writeRecord size : " << record.recordSize << std::endl;
}
// Endoe data in page -> Record();
// Decode data in page
void DataPage::readRecord(FileHandle& fileHandle, const RID& rid, void* data) {

    std::pair<uint16_t,uint16_t> indexPair = getIndexPair(rid.slotNum);
    if( indexPair.first & DELETED_MASK || indexPair.first & TOMB_MASK) {
        std::cerr << "DataPage: RID should be valid" << std::endl;
    }
    uint16_t indexValue = indexPair.first;
    uint16_t lenValue = indexPair.second;
    // interpret index
    uint8_t* recordPos =  reinterpret_cast<uint8_t*>(page) + indexValue;
    uint16_t numOfField = recordPos[1] << 8 | recordPos[0];
    uint8_t* nullPos = recordPos + Record::indexSize;
    uint16_t indicatorSize = ceil(static_cast<double>(numOfField)/CHAR_BIT);
    uint8_t* dataPos = nullPos + indicatorSize + Record::indexSize * numOfField;
    
    memcpy(data, nullPos, indicatorSize);
    memcpy((char*)data+indicatorSize, dataPos, lenValue-Record::indexSize * (1+numOfField) - Record::paddingSize);
}

void DataPage::shiftRecords(uint16_t startPos, int16_t diff) {
    // startPos - this->var[]
    int16_t size = var[RECORD_OFFSET_FROM_BEGIN] - startPos;
    if( startPos < 0) {
        std::cerr << "shiftRecords: shift Record with out of bound [ startPos ]" << std::endl; 
    }
    else if (var[RECORD_OFFSET_FROM_BEGIN] + diff > var[HEADER_OFFSET_FROM_END] ) {
        std::cerr << "shiftRecords: shift Record with out of bound [ diff ]" << std::endl;
    }
    else if (startPos + diff < 0) {
        std::cerr << "shiftRecords: shift Record with out of bound [ diff ]" << std::endl;
    }

    // This method guarantee correct behavior for overlapping buffer.
    memmove(reinterpret_cast<uint8_t*>(startPos) + diff, reinterpret_cast<uint8_t*>(startPos), size);
}

void DataPage::updateRecord(Record& newRecord, FileHandle& fileHandle, const RID& rid) {
    
    std::pair<uint16_t,uint16_t> indexPair = this->getIndexPair(rid.slotNum);
    int16_t diff = newRecord.recordSize - indexPair.second;

    //write Record
    memcpy((char*)page + indexPair.first, newRecord.getRecord(), newRecord.recordSize);
    // write index,length
    std::pair<uint16_t ,uint16_t> newRecordHeader;
    newRecordHeader = {indexPair.first, newRecord.recordSize};

    void* indexPos = reinterpret_cast<uint8_t*>(page) + PAGE_SIZE -                 
                     sizeof(unsigned) * DATA_PAGE_VAR_NUM  -                       
                     (rid.slotNum +1) * sizeof(std::pair<uint16_t, uint16_t>); 

    memcpy(indexPos, &newRecordHeader, sizeof(std::pair<uint16_t, uint16_t>));

    var[RECORD_OFFSET_FROM_BEGIN] += diff;
    // write header
    memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    fileHandle.writePage(rid.pageNum, page);
}

void DataPage::writeRecordFromTombstone(FileHandle& fileHandle, const char* data, uint16_t recordize, uint16_t offsetFromBegin) {

    //  Past record
    memcpy((char*)page + offsetFromBegin, data, recordize);

    //  Update Record offset in var
    var[RECORD_OFFSET_FROM_BEGIN] += recordize;

    //  Write var
    memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
}


unsigned DataPage::getFreeSpaceSize() {
    return PAGE_SIZE - var[RECORD_OFFSET_FROM_BEGIN] - var[HEADER_OFFSET_FROM_END];
}

std::pair<uint16_t,uint16_t> DataPage::getIndexPair(const uint16_t slotNum) {
    void* indexPos = reinterpret_cast<uint8_t*>(page) + PAGE_SIZE -                 // end of PAGE file
                     sizeof(unsigned) * DATA_PAGE_VAR_NUM  -                        // metadata
                     (slotNum +1) * sizeof(std::pair<uint16_t, uint16_t>);          // No. i index Offset;

    return reinterpret_cast<std::pair<uint16_t,uint16_t>*>(indexPos)[0];
}

DataPage::DataPage(const DataPage& datapage) {
}


DataPage& DataPage::operator=(const DataPage &dataPage) {
}
