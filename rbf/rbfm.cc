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


void RecordBasedFileManager::appendPage(FileHandle &fileHandle) {
    uint8_t* newPage = new uint8_t [PAGE_SIZE];
    unsigned var[4] = {sizeof(unsigned) * DATA_PAGE_VAR_NUM, 0, 0, 0};
    memcpy(newPage + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    fileHandle.appendPage(newPage);
    delete [] newPage;
}


RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,const void *data, RID &rid) {
    Record record(recordDescriptor, data, rid);
    uint8_t* pageData = new uint8_t [PAGE_SIZE];
    uint32_t availablePageNum = getPageNumWithEmptySlot(record.recordSize,fileHandle);
    if(availablePageNum != -1) {
        std::cerr << "FIND DELTED SLOT PAGE " <<std::endl;
        fileHandle.readPage(availablePageNum, pageData);
        DataPage page(pageData);
        uint16_t slotNum = page.findEmptySlot();
        page.updateRecord(fileHandle,record,availablePageNum, page.var[RECORD_OFFSET_FROM_BEGIN]);
        page.updateIndexPair(fileHandle,availablePageNum,slotNum, {page.var[RECORD_OFFSET_FROM_BEGIN], record.recordSize});
        page.updateOffsetFromBegin(fileHandle,availablePageNum,record.recordSize);
        rid = {availablePageNum, slotNum};
    }
    else {
        uint32_t lastPageNum = fileHandle.getNumberOfPages()-1;
        availablePageNum = getNextAvailablePageNum(record.recordSize + sizeof(std::pair<uint16_t, uint16_t>), fileHandle, lastPageNum);
        fileHandle.readPage(availablePageNum, pageData);
        DataPage page(pageData);
        page.writeRecord(record,fileHandle,availablePageNum,rid);
    }
    delete[] pageData;
    return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,const RID &rid, void *data) {

    // TODO do DELTED_MASK && TOMB_MASK

    void* pageData = new uint8_t [PAGE_SIZE];
    if(fileHandle.readPage(rid.pageNum,pageData) == 0) {
        DataPage basePage(pageData);
        auto indexPair = basePage.getIndexPair(rid.slotNum);

        if(indexPair.second == 0) {
            return -1;
        }
        if( !basePage.isRecord(fileHandle,rid)) {
            Tombstone tombstone;
            basePage.readTombstone(tombstone,rid);

            void* recordPageBuffer = new uint8_t [PAGE_SIZE];  
            fileHandle.readPage(tombstone.pageNum, recordPageBuffer);
            DataPage recordPage(recordPageBuffer);
            
            recordPage.readRecord(fileHandle,tombstone.offsetFromBegin,indexPair.second,data);
            return 0;
        }
        else {
            basePage.readRecord(fileHandle, indexPair.first, indexPair.second , data);
            return 0;
        }
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
                    delete[] s;
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

uint32_t RecordBasedFileManager::getNextAvailablePageNum(uint16_t insertSize, FileHandle& fileHandle, const uint32_t& pageFrom) {

    //  Loop all pages to find a page with enough space to relocate record, return the page number
    for(uint32_t i = 0; i < fileHandle.getNumberOfPages() ; i++) {
        uint8_t* pageData = new uint8_t[PAGE_SIZE];
        fileHandle.readPage((pageFrom + i) % fileHandle.getNumberOfPages(), pageData);
        DataPage page(pageData);
        if(insertSize <= page.getFreeSpaceSize()) {
            delete[] pageData;
            return (pageFrom + i) % fileHandle.getNumberOfPages();
        }
        delete[] pageData;
    }
    //  All pages are full, append new blank page and return page number
    this->appendPage(fileHandle);

    return fileHandle.getNumberOfPages()-1;
}

uint32_t RecordBasedFileManager::getPageNumWithEmptySlot(uint16_t insertSize, FileHandle& fileHandle) {
    for( uint32_t i = 0 ; i < fileHandle.getNumberOfPages() ; i++) {
        uint8_t* pageBuffer = new uint8_t [PAGE_SIZE];
        fileHandle.readPage(i,pageBuffer);
        DataPage page(pageBuffer);
        if(page.var[SLOT_NUM] > page.var[RECORD_NUM]  && insertSize <= page.getFreeSpaceSize()) {
            delete[] pageBuffer;
            return i;
        }
        delete[] pageBuffer;
    }
    return -1;
}

//////////////////////////////////////////////////////////////
// PROJECT 1 TO HERE                                        //
//////////////////////////////////////////////////////////////


RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                        const RID &rid) {
    uint8_t* buffer = new uint8_t [PAGE_SIZE];
    fileHandle.readPage(rid.pageNum,buffer);
    DataPage initPage(buffer);

    std::pair<uint16_t, uint16_t> indexPair = initPage.getIndexPair(rid.slotNum);
    
    if(!initPage.isRecord(fileHandle,rid)) {
        Tombstone tombstone;
        initPage.readTombstone(tombstone, rid);
        
        
        uint8_t* recordPageBuffer = new uint8_t [PAGE_SIZE];
        fileHandle.readPage(tombstone.pageNum, recordPageBuffer);
        DataPage recordPage(recordPageBuffer);

        recordPage.shiftRecords(fileHandle,tombstone.pageNum, tombstone.offsetFromBegin + indexPair.second, -indexPair.second);
        recordPage.shiftIndexes(fileHandle,tombstone.pageNum, tombstone.offsetFromBegin, -indexPair.second);
        recordPage.updateOffsetFromBegin(fileHandle,tombstone.pageNum, -indexPair.second);


        initPage.shiftRecords(fileHandle, rid.pageNum, indexPair.first + sizeof(Tombstone), -sizeof(Tombstone));
        initPage.shiftIndexes(fileHandle,rid.pageNum, indexPair.first, -sizeof(Tombstone));
        initPage.updateOffsetFromBegin(fileHandle, rid.pageNum, -sizeof(Tombstone));
        initPage.updateIndexPair(fileHandle, rid.pageNum, rid.slotNum, {0,0});

        delete[] recordPageBuffer;
    }
    else {
        initPage.shiftRecords(fileHandle, rid.pageNum, indexPair.first+indexPair.second, -indexPair.second); 
        initPage.shiftIndexes(fileHandle, rid.pageNum, indexPair.first, -indexPair.second);
        initPage.updateOffsetFromBegin(fileHandle, rid.pageNum, -indexPair.second);
        initPage.updateIndexPair(fileHandle,rid.pageNum, rid.slotNum, {0,0});
    }
    ///////////////////////////////////////////////////////////////////////////////////////
    std::cerr <<"RBFM: delete: " <<std::endl;
    std::cerr <<"FROM " << initPage.var[RECORD_NUM] ;
    // initPage.var[RECORD_NUM]--;
    initPage.decreaseRecordNum(fileHandle,rid.pageNum);
    std::cerr <<" TO " << initPage.var[RECORD_NUM] <<std::endl;
    
    ///////////////////////////////////////////////////////////////////////////////////////
    delete[] buffer;
    return 0;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, 
                                        const std::vector<Attribute> &recordDescriptor,
                                        const void *data, const RID &rid) {

    char* initPageBuffer = new char [PAGE_SIZE];
    fileHandle.readPage(rid.pageNum, initPageBuffer);
    DataPage initPage(initPageBuffer);
    std::pair<uint16_t,uint16_t> indexPair = initPage.getIndexPair(rid.slotNum);

    Record newRecord(recordDescriptor,data, rid);
    int16_t recordsDiff = newRecord.recordSize - indexPair.second;
    int16_t tombstoneDiff = sizeof(Tombstone) - indexPair.second;

    // recordPage.shiftRecords(fileHandle, recordPtr.first, recordPtr.second + oldRecordSize, recordsDiff);
    //     // recordPage.updateRecord(newRecord,fileHandle,rid);
    //     // recordPage.updateIndexPair

    if(initPage.isRecord(fileHandle, rid)) {
        if(recordsDiff <= initPage.getFreeSpaceSize()) {
            std::cerr << "old record size: " << indexPair.second << std::endl;
            std::cerr << "new record size: " << newRecord.recordSize << std::endl;
            std::cerr << "record diff: " << recordsDiff << std::endl;
            std::cerr << "record start: " << indexPair.first << std::endl; 
            initPage.shiftRecords(fileHandle, rid.pageNum, indexPair.first + indexPair.second, recordsDiff);
            initPage.shiftIndexes(fileHandle, rid.pageNum, indexPair.first, recordsDiff);
            initPage.updateRecord(fileHandle, newRecord, rid.pageNum, initPage.getIndexPair(rid.pageNum).first);
            initPage.updateOffsetFromBegin(fileHandle, rid.pageNum, recordsDiff);
            initPage.updateIndexPair(fileHandle, rid.pageNum, rid.slotNum, {indexPair.first, newRecord.recordSize});
        }
        else {
            //  tombstone
            uint32_t availablePageNum = getNextAvailablePageNum(newRecord.recordSize, fileHandle, rid.pageNum);
            char* availablePageBuffer = new char [PAGE_SIZE];
            fileHandle.readPage(availablePageNum, availablePageBuffer);
            DataPage availablePage(availablePageBuffer);

            Tombstone tombstone = {TOMB_MASK, availablePageNum, 0};
            availablePage.writeRecordFromTombstone(fileHandle, newRecord, availablePageNum, tombstone);

            initPage.shiftRecords(fileHandle, rid.pageNum, indexPair.first + indexPair.second, tombstoneDiff);
            initPage.writeTombstone(fileHandle, rid.pageNum, tombstone, indexPair.first);
            initPage.updateOffsetFromBegin(fileHandle, rid.pageNum, tombstoneDiff);
            initPage.updateIndexPair(fileHandle,rid.pageNum, rid.slotNum, {indexPair.first,newRecord.recordSize});

        }
    }
    else {
        Tombstone tombstone;
        initPage.readTombstone(tombstone, rid);
        char* recordPageBuffer = new char [PAGE_SIZE];
        fileHandle.readPage(tombstone.pageNum, recordPageBuffer);
        DataPage recordPage(recordPageBuffer);

        if(recordsDiff <= recordPage.getFreeSpaceSize() ) {
            recordPage.shiftRecords(fileHandle, tombstone.pageNum, tombstone.offsetFromBegin + indexPair.second , recordsDiff);
            recordPage.shiftIndexes(fileHandle, tombstone.pageNum, tombstone.offsetFromBegin, recordsDiff);
            recordPage.updateRecord(fileHandle, newRecord, tombstone.pageNum,  tombstone.offsetFromBegin);
            recordPage.updateOffsetFromBegin(fileHandle, tombstone.pageNum, recordsDiff);
            initPage.updateIndexPair(fileHandle, rid.pageNum, rid.slotNum, {indexPair.first, newRecord.recordSize});
        }
        else {
            //  tombstone
            uint32_t availablePageNum = getNextAvailablePageNum(newRecord.recordSize, fileHandle, tombstone.pageNum);

            char* availablePageBuffer = new char [PAGE_SIZE];
            fileHandle.readPage(availablePageNum, availablePageBuffer);
            DataPage availablePage(availablePageBuffer);

            availablePage.writeRecordFromTombstone(fileHandle, newRecord, availablePageNum, tombstone);
            recordPage.shiftRecords(fileHandle, tombstone.pageNum, tombstone.offsetFromBegin + indexPair.second, -indexPair.second);
            recordPage.shiftIndexes(fileHandle, tombstone.pageNum, tombstone.offsetFromBegin + indexPair.second, -indexPair.second);
            initPage.writeTombstone(fileHandle, rid.pageNum, tombstone, indexPair.first);
            initPage.updateIndexPair(fileHandle, rid.pageNum, rid.slotNum, {indexPair.first, newRecord.recordSize});

            delete[] availablePageBuffer;
        }
        delete[] recordPageBuffer;
    }
    delete[] initPageBuffer;

    return 0;

}

RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,
                                         const RID &rid, const std::string &attributeName, void *data) {
    //  Get page
    char* pageBuffer = new char [PAGE_SIZE];
    fileHandle.readPage(rid.pageNum, pageBuffer);
    DataPage page(pageBuffer);

    //  Get Record
    std::pair<uint16_t,uint16_t> indexPair = page.getIndexPair(rid.slotNum);


    void* recordBuffer = new char [indexPair.second];
    readRecord(fileHandle, recordDescriptor, rid, recordBuffer);
    Record record(recordDescriptor, recordBuffer, rid);

    //  Get Attribute
    record.getAttribute(attributeName, recordDescriptor, data);

    delete[] pageBuffer;
    delete[] recordBuffer;
    return 0;
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
    delete[] buffer;
    return 0;
}



Record::Record(int none)
:numOfField(0),indicatorSize(0), nullData(nullptr),indexData(nullptr)
{
    this->recordSize = 0;
    this->recordHead = new uint8_t (1);
}

Record::Record(const std::vector<Attribute> &_descriptor, const void* _data, const RID &_rid) {
    
    this->numOfField = _descriptor.size();
    this->indicatorSize = std::ceil( static_cast<double>(_descriptor.size()) /CHAR_BIT);
    //TODO: need to store rid?
    this->nullData = reinterpret_cast<const uint8_t*>(_data);
    this->indexData = new uint16_t [this->numOfField];
    convertData(_descriptor,_data);
}

bool Record::isNull(int fieldNum) {
    int byteOffset = fieldNum / CHAR_BIT;
    int bitOffset = fieldNum % CHAR_BIT;

    return this->nullData[byteOffset] >> (7 - bitOffset) & 1;
}

void Record::getAttribute(const std::string attrName, const std::vector<Attribute> &recordDescriptor, void *attr) {
    uint8_t nullIndicator;
    for(int i = 0; i < recordDescriptor.size(); i++) {
        if(!isNull(i)) {
            if(recordDescriptor[i].name == attrName) {
                nullIndicator = 0;
                memcpy(attr, &nullIndicator, sizeof(uint8_t));
                if(recordDescriptor[i].type == TypeInt) {
                    memcpy((char*)attr + sizeof(uint8_t), recordHead + indexData[i], sizeof(int));
                }
                else if(recordDescriptor[i].type == TypeReal) {
                    memcpy((char*)attr + sizeof(uint8_t), recordHead + indexData[i], sizeof(float));
                }
                else if(recordDescriptor[i].type == TypeVarChar) {
                    uint32_t attrSize = 0;
                    memcpy(&attrSize, recordHead + indexData[i], sizeof(uint32_t));
                    memcpy((char*)attr + sizeof(uint8_t), recordHead + indexData[i] + sizeof(uint32_t), attrSize);
                }
                return;
            }
        }
    }
    nullIndicator = 128;
    memcpy(attr, &nullIndicator, sizeof(uint8_t));
}

Record::~Record() {
     delete[] recordHead;
    // delete[] this->nullData;
    // delete[] this->recordData;
}


//TypeInt = 0, TypeReal, TypeVarChar
void Record::convertData(const std::vector<Attribute> &descriptor, const void* _data) {
    uint16_t size = 0;
    uint16_t byteOffset = Record::indexSize + this->indicatorSize + Record::indexSize * this->numOfField;

    // // treat it as byte and move to data part
    const uint8_t* pos = reinterpret_cast<const uint8_t*>(_data) + (uint8_t)this->indicatorSize; 
    
    for(int i = 0 ; i < this->numOfField ; i++ ) {

        indexData[i] = byteOffset + size;
        if ( descriptor[i].type == TypeInt && !isNull(i) ) {
            pos += sizeof(int);
            size += sizeof(int);
        }
        else if (descriptor[i].type == TypeReal && !isNull(i)) {
            pos += sizeof(float);
            size += sizeof(float);
        }
        else if (descriptor[i].type == TypeVarChar && !isNull(i)) {
            // byte to int
            uint32_t varCharSize; 
            memcpy(&varCharSize, pos, sizeof(uint32_t));
            pos +=  (sizeof(uint32_t) + varCharSize);
            size += (sizeof(uint32_t) + varCharSize);
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
void DataPage::writeRecord(const Record &record, FileHandle &fileHandle, unsigned availablePage, RID &rid ) {

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
    var[RECORD_NUM]++;
    // write header
    memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    // write page
    fileHandle.writePage(availablePage, page);
    // std::cerr << "DataPage: writeRecord size : " << record.recordSize << std::endl;
}
// Endoe data in page -> Record();
// Decode data in page
void DataPage::readRecord(FileHandle& fileHandle, uint16_t offset, uint16_t recordSize, void* data) {

    uint16_t indexValue = offset;
    uint16_t lenValue = recordSize;
    // interpret index
    uint8_t* recordPos =  reinterpret_cast<uint8_t*>(page) + indexValue;
    uint16_t numOfField = recordPos[1] << 8 | recordPos[0];
    uint8_t* nullPos = recordPos + Record::indexSize;
    uint16_t indicatorSize = ceil(static_cast<double>(numOfField)/CHAR_BIT);
    uint8_t* dataPos = nullPos + indicatorSize + Record::indexSize * numOfField;
    
    memcpy(data, nullPos, indicatorSize);
    memcpy((char*)data+indicatorSize, dataPos, lenValue-Record::indexSize * (1+numOfField) - Record::paddingSize);
}

// shift startPos - this->var[] with diff bytes
// update var[RECORD_OFFSET_FROM_BEGIN]
void DataPage::shiftRecords(FileHandle& fileHandle, uint32_t pageNum, uint16_t startPos, int16_t diff) {
    int16_t size = var[RECORD_OFFSET_FROM_BEGIN] - startPos;
    
    std::cerr << startPos << ' ' << diff << ' ' << size << ' ' << var[HEADER_OFFSET_FROM_END] <<  std::endl;
    if( startPos < 0) {
        std::cerr << "shiftRecords: shift Record with out of bound [ startPos ]" << std::endl; 
    }
    else if (var[RECORD_OFFSET_FROM_BEGIN] + diff > PAGE_SIZE - var[HEADER_OFFSET_FROM_END] ) {
        std::cerr << "shiftRecords: shift Record with out of bound [ end ]" << std::endl;
    }
    else if (startPos + diff < 0) {
        std::cerr << "shiftRecords: shift Record with out of bound [ begin ]" << std::endl;
    }

    // This method guarantee correct behavior for overlapping buffer.
    memmove(reinterpret_cast<uint8_t*>(page) + startPos + diff, reinterpret_cast<uint8_t*>(page) + startPos, size);


    // memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);


    // // update header
    // var[RECORD_OFFSET_FROM_BEGIN] += diff;
    // // write header
    // memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    fileHandle.writePage(pageNum, page);
}

void DataPage::updateOffsetFromBegin(FileHandle &fileHandle, uint32_t pageNum, int16_t diff) {
    // update header
    var[RECORD_OFFSET_FROM_BEGIN] += diff;
    // write header
    memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    // write to disk
    fileHandle.writePage(pageNum, page);
}


// write Record
void DataPage::updateRecord(FileHandle& fileHandle, const Record& newRecord,  uint32_t pageNum, uint16_t offset) {
    
    if( newRecord.recordSize == 0)
    {
        std::cerr <<"DataPage: delete a record" << std::endl;
    }
    // write Record
    memcpy((char*)page + offset, newRecord.getRecord(), newRecord.recordSize);
    fileHandle.writePage(pageNum, page);
}

void DataPage::shiftIndexes(FileHandle& fileHandle, uint32_t pageNum, uint16_t startPos, int16_t diff) {
    for(uint16_t i = 0 ; i < var[SLOT_NUM] ; i++) {
        std::pair<uint16_t, uint16_t> indexPair = getIndexPair(i);
        if(indexPair.first > startPos) {
            std::pair<uint16_t, uint16_t> newIndexPair = {indexPair.first+diff, indexPair.second};
            updateIndexPair(fileHandle, pageNum,  i, newIndexPair);
        }
    }
}

// this function do not write to disk
void DataPage::updateIndexPair(FileHandle& fileHandle, uint32_t pageNum,  uint16_t slotNum, std::pair<uint16_t,uint16_t> newIndexPair) {
    void* indexPos = reinterpret_cast<uint8_t*>(page) + PAGE_SIZE - 
                     sizeof(unsigned) * DATA_PAGE_VAR_NUM  -        
                     (slotNum +1) * sizeof(std::pair<uint16_t, uint16_t>);
    memcpy(indexPos, &newIndexPair, sizeof(std::pair<uint16_t, uint16_t>));
    fileHandle.writePage(pageNum, page);
}

void DataPage::insertTombstone(Tombstone &tombstone, FileHandle &fileHandle, const RID &rid, const uint16_t recordSize) {
    std::pair<uint16_t,uint16_t> indexPair = this->getIndexPair(rid.slotNum);

    //  write tombstone
    memcpy((char*)page + indexPair.first, &tombstone, sizeof(Tombstone));

    //  update header::length
    std::pair<uint16_t, uint16_t> newRecordHeader = {indexPair.first, recordSize};

    updateIndexPair(fileHandle, rid.pageNum,  rid.slotNum, newRecordHeader);
    fileHandle.writePage(rid.pageNum, page);
}

void DataPage::readTombstone(Tombstone &tombstone, const RID &rid) {
    std::pair<uint16_t,uint16_t> indexPair = this->getIndexPair(rid.slotNum);
    memcpy(&tombstone, (char*)page + indexPair.first, sizeof(Tombstone));
}

void DataPage::writeTombstone(FileHandle &fileHandle, uint32_t pageNum, Tombstone &tombstone, const uint16_t offsetFromBegin) {
    memcpy((char*)page + offsetFromBegin, &tombstone, sizeof(Tombstone));
    fileHandle.writePage(pageNum, page);
}

void DataPage::decreaseRecordNum(FileHandle &fileHandle, uint32_t pageNum) {
    var[RECORD_NUM]--;
    memcpy(reinterpret_cast<uint8_t*>(page) + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    fileHandle.writePage(pageNum,page);
}

uint16_t DataPage::findEmptySlot() {
    for(uint16_t i = 0 ; i < var[SLOT_NUM] ; i++) {
        std::pair<uint16_t,uint16_t> indexPair = getIndexPair(i);
        if(indexPair.second == 0) {
            return i;
        }
    }
    std::cerr << "DataPage: Cannot find Empty Slot!!! " <<std::endl;
    return -1;
}

void DataPage::writeRecordFromTombstone(FileHandle& fileHandle, Record& record, uint32_t pageNum, Tombstone &tombstone) {

    //  Past record
    memcpy((char*)page + var[RECORD_OFFSET_FROM_BEGIN], record.getRecord(), record.recordSize);

    //  Keep record place in tombstone
    tombstone.offsetFromBegin = var[RECORD_OFFSET_FROM_BEGIN];

    //  Update Record offset in var
    var[RECORD_OFFSET_FROM_BEGIN] += record.recordSize;

    //  Update var
    memcpy((char*)page + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, &var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);

    //  Write back to file
    fileHandle.writePage(pageNum, page);
}

void DataPage::deleteRecordFromTombstone(FileHandle &fileHandle, uint32_t pageNum, Tombstone &tombstone, const int16_t tombstoneDiff) {



}

unsigned DataPage::getFreeSpaceSize() {
    return PAGE_SIZE - var[RECORD_OFFSET_FROM_BEGIN] - var[HEADER_OFFSET_FROM_END];
}

bool DataPage::isRecord(FileHandle &fileHandle, const RID &rid) {
    std::pair<uint16_t,uint16_t> indexPair = getIndexPair(rid.slotNum);
    uint16_t tombstoneTag;
    memcpy(&tombstoneTag, (char*)page + indexPair.first, sizeof(uint16_t));
    return tombstoneTag != TOMB_MASK;
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
