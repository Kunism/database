#include <cmath>
#include <iostream>
#include "rbfm.h"
#include "pfm.h"

#include <sstream>

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
                temp.writeRecord(record,fileHandle,lastPageNum, rid);
                record.rid = rid;
                return 0;
            }
        }
    }
    void* buf = new char [PAGE_SIZE];
    unsigned var[3] = {sizeof(unsigned) * 3, 0, 0};
    memcpy((char*)buf + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM, var, sizeof(unsigned) * DATA_PAGE_VAR_NUM);
    DataPage newPage(buf);
    newPage.writeRecord(record,fileHandle,lastPageNum+1,rid);
    return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor,const RID &rid, void *data) {
    void* pageData = new uint8_t [PAGE_SIZE];
    if(fileHandle.readPage(rid.pageNum,pageData) == 0) {
        DataPage dataPage(pageData);
        dataPage.readRecord(fileHandle, rid, data);
        //memcpy(data,record.recordData,record.dataSize);
        return 0;
    }
    return -1;
}

RC RecordBasedFileManager::printRecord(const std::vector<Attribute> &recordDescriptor, const void *data) {
    RID fakeRid = {0,0};
    Record record(recordDescriptor,data,fakeRid);

    // std::cout << "BYTE OFFSET " << Record::indexSize * (1 + recordDescriptor.size()) <<std::endl;
    // for(int i = 0 ; i < recordDescriptor.size() ; i++) {
    //     std::cout << "TYPE :" << recordDescriptor[i].type << ' ';
    //     std::cout << "INDEX " << record.indexData[i] << ' ';
    //     std::cout <<std::endl;
    // }

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

    // std::cerr << "CONVERT: size 1 " << size << std::endl;
    
    // // treat it as byte and move to data part
    const uint8_t* pos = reinterpret_cast<const uint8_t*>(_data) + (uint8_t)this->indicatorSize; 
    
    for(int i = 0 ; i < this->numOfField ; i++ ) {
        // std::cerr << "CONVERT: size 3 " << size << std::endl;
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
            // uint32_t varCharSize = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
            uint32_t varCharSize; 
            memcpy(&varCharSize, pos, sizeof(uint32_t));
            // std::cerr << "CONVERT: var char size: " << varCharSize <<std::endl;
            pos += (4 + varCharSize);
            size += (4 + varCharSize);
        }
    }

    // std::cerr << "CONVERT: SIZE: " << size <<std::endl;
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
    page = new char [PAGE_SIZE];
    memcpy(page, data, PAGE_SIZE);
    
    pageHeader = new std::pair<uint16_t , uint16_t> [var[SLOT_NUM]];
    memcpy(&pageHeader, (char*)data + PAGE_SIZE - sizeof(unsigned) * DATA_PAGE_VAR_NUM - 
                        sizeof(std::pair<uint16_t, uint16_t>) * var[SLOT_NUM],
                        sizeof(std::pair<uint16_t, uint16_t>) * var[SLOT_NUM]);

    
}

DataPage::~DataPage() {
    // delete[] pageHeader;
    // delete[] page;

    // TODO: find the share ptr

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
    std::cerr << "PAGE OFFSET: " << var[RECORD_OFFSET_FROM_BEGIN] << " " << 
                  this->getFreeSpaceSize()  << " " <<  record.recordSize << std::endl;
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

    //TODO: update header in memory

    fileHandle.writePage(availablePage, page);
    std::cout << "availalepage: " << availablePage << std::endl;
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

    // std::cerr << "RID: " << rid.pageNum << ' ' << rid.slotNum << std::endl;
    // std::cerr << "RECORD INDEX: " << indexValue << std::endl;
    // std::cerr << "RECORD SIZE: " << lenValue << std::endl;
    // std::cerr << "# of Field: " << numOfField << std::endl;
    // std::cerr << "Null indicator: " << indicatorSize << std::endl;
    // std::cerr << "Null MAST: " << (int)nullPos[0] << std::endl;
    
    memcpy(data, nullPos, indicatorSize);
    memcpy((char*)data+indicatorSize, dataPos, lenValue-Record::indexSize * (1+numOfField));

//     memcpy(data, (char*)page + pageHeader[rid.slotNum].first, pageHeader[rid.slotNum].second * sizeof(uint16_t));

// --------------
//    uint16_t N, nullIndicatorLength, indexLength, dataLength, recordLength;
//
//    memcpy(&N, (char*)page + pageHeader[rid.slotNum].first, sizeof(uint16_t));
//    recordLength = sizeof(uint16_t) * pageHeader[rid.slotNum].second;
//    indexLength = sizeof(uint16_t) * N;
//    nullIndicatorLength = ceil((double)N / 8);
//    dataLength = recordLength - indexLength - nullIndicatorLength - sizeof(uint16_t);
//    std::cout << N << std::endl;
//    char* buffer = new char [recordLength];
//    memcpy(buffer, (char*)page + pageHeader[rid.slotNum].first + sizeof(uint16_t), nullIndicatorLength);
//    memcpy(buffer + nullIndicatorLength, (char*)page + pageHeader[rid.slotNum].first + sizeof(uint16_t) + nullIndicatorLength + indexLength, dataLength);
//
//    memcpy(data, buffer, recordLength);
}

unsigned DataPage::getFreeSpaceSize() {
    return PAGE_SIZE - var[RECORD_OFFSET_FROM_BEGIN] - var[HEADER_OFFSET_FROM_END];
}

DataPage::DataPage(const DataPage& datapage) {
    std::cerr << "copy ctor" << std::endl;
}


DataPage& DataPage::operator=(const DataPage &dataPage) {
    std::cerr << "assignment operator" <<std::endl;
}
