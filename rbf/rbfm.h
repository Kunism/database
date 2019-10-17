#ifndef _rbfm_h_
#define _rbfm_h_

#include <string>
#include <vector>
#include <climits>

#include "pfm.h"


static const uint16_t DELETED_MASK = 0x8000; // 1000 0000 ...
static const uint16_t TOMB_MASK = 0x4000;    // 0100 0000 ...

class Record;
class Tombstone;

// Record ID
typedef struct {
    unsigned pageNum;    // page number
    unsigned slotNum;    // slot number in the page
} RID;

// Attribute
typedef enum {
    TypeInt = 0, TypeReal, TypeVarChar
} AttrType;

typedef unsigned AttrLength;

struct Attribute {
    std::string name;     // attribute name
    AttrType type;     // attribute type
    AttrLength length; // attribute length
};

// Comparison Operator (NOT needed for part 1 of the project)
typedef enum {
    EQ_OP = 0, // no condition// =
    LT_OP,      // <
    LE_OP,      // <=
    GT_OP,      // >
    GE_OP,      // >=
    NE_OP,      // !=
    NO_OP       // no condition
} CompOp;


/********************************************************************
* The scan iterator is NOT required to be implemented for Project 1 *
********************************************************************/

# define RBFM_EOF (-1)  // end of a scan operator

// RBFM_ScanIterator is an iterator to go through records
// The way to use it is like the following:
//  RBFM_ScanIterator rbfmScanIterator;
//  rbfm.open(..., rbfmScanIterator);
//  while (rbfmScanIterator(rid, data) != RBFM_EOF) {
//    process the data;
//  }
//  rbfmScanIterator.close();

class RBFM_ScanIterator {
public:
    RBFM_ScanIterator() = default;;

    ~RBFM_ScanIterator() = default;;

    // Never keep the results in the memory. When getNextRecord() is called,
    // a satisfying record needs to be fetched from the file.
    // "data" follows the same format as RecordBasedFileManager::insertRecord().
    RC getNextRecord(RID &rid, void *data) { return RBFM_EOF; };

    RC close() { return -1; };
};

class RecordBasedFileManager {
public:
    static RecordBasedFileManager &instance();                          // Access to the _rbf_manager instance

    RC createFile(const std::string &fileName);                         // Create a new record-based file

    RC destroyFile(const std::string &fileName);                        // Destroy a record-based file

    RC openFile(const std::string &fileName, FileHandle &fileHandle);   // Open a record-based file

    RC closeFile(FileHandle &fileHandle);                               // Close a record-based file

    //  Format of the data passed into the function is the following:
    //  [n byte-null-indicators for y fields] [actual value for the first field] [actual value for the second field] ...
    //  1) For y fields, there is n-byte-null-indicators in the beginning of each record.
    //     The value n can be calculated as: ceil(y / 8). (e.g., 5 fields => ceil(5 / 8) = 1. 12 fields => ceil(12 / 8) = 2.)
    //     Each bit represents whether each field value is null or not.
    //     If k-th bit from the left is set to 1, k-th field value is null. We do not include anything in the actual data part.
    //     If k-th bit from the left is set to 0, k-th field contains non-null values.
    //     If there are more than 8 fields, then you need to find the corresponding byte first,
    //     then find a corresponding bit inside that byte.
    //  2) Actual data is a concatenation of values of the attributes.
    //  3) For Int and Real: use 4 bytes to store the value;
    //     For Varchar: use 4 bytes to store the length of characters, then store the actual characters.
    //  !!! The same format is used for updateRecord(), the returned data of readRecord(), and readAttribute().
    // For example, refer to the Q8 of Project 1 wiki page.

    // Insert a record into a file
    RC insertRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor, const void *data, RID &rid);

    // Read a record identified by the given rid.
    RC readRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor, const RID &rid, void *data);

    // Print the record that is passed to this utility method.
    // This method will be mainly used for debugging/testing.
    // The format is as follows:
    // field1-name: field1-value  field2-name: field2-value ... \n
    // (e.g., age: 24  height: 6.1  salary: 9000
    //        age: NULL  height: 7.5  salary: 7500)
    RC printRecord(const std::vector<Attribute> &recordDescriptor, const void *data);

    /*****************************************************************************************************
    * IMPORTANT, PLEASE READ: All methods below this comment (other than the constructor and destructor) *
    * are NOT required to be implemented for Project 1                                                   *
    *****************************************************************************************************/
    // Delete a record identified by the given rid.
    RC deleteRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor, const RID &rid);

    // Assume the RID does not change after an update
    RC updateRecord(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor, const void *data,
                    const RID &rid);

    // Read an attribute given its name and the rid.
    RC readAttribute(FileHandle &fileHandle, const std::vector<Attribute> &recordDescriptor, const RID &rid,
                     const std::string &attributeName, void *data);

    // Scan returns an iterator to allow the caller to go through the results one by one.
    RC scan(FileHandle &fileHandle,
            const std::vector<Attribute> &recordDescriptor,
            const std::string &conditionAttribute,
            const CompOp compOp,                  // comparision type such as "<" and "="
            const void *value,                    // used in the comparison
            const std::vector<std::string> &attributeNames, // a list of projected attributes
            RBFM_ScanIterator &rbfm_ScanIterator);

    RC writeRecordFromTombstone(Record& record, FileHandle& fileHandle, uint32_t availablePageNum, uint16_t offsetFromBegin, const Tombstone& tombstone);
    uint32_t getNextAvailablePageNum(Record& record, FileHandle& fileHandle, const uint32_t& pageFrom);

public:

protected:
    RecordBasedFileManager();                                                   // Prevent construction
    ~RecordBasedFileManager();                                                  // Prevent unwanted destruction
    RecordBasedFileManager(const RecordBasedFileManager &);                     // Prevent construction by copying
    RecordBasedFileManager &operator=(const RecordBasedFileManager &);          // Prevent assignment


private:
    static RecordBasedFileManager *_rbf_manager;
};


class Record {
public:
    Record(const std::vector<Attribute> &_descriptor, const void* _data, const RID &_rid);
    ~Record();
    bool isNull(int fieldNum);
    // convert Raw data to void*
    const uint8_t* getRecord() const;

    // Record ID
    RID rid;
    // number of fields = descriptor.size();
    uint16_t numOfField; 
    // total rocord size in bytes (size + null part + index part + data part)
    uint16_t recordSize;
    std::vector<Attribute> descriptor;

    uint16_t* indexData;


    // each index size used 2 byte: uint16_t:  65535
    const static uint16_t indexSize = 2;
    // record padding size = 1 byte;
    const static uint16_t paddingSize = 1;

private:
    void convertData(const void* _data);
    // use Record::getRecord to access
    uint8_t* recordHead;

    // null indicator size (bytes) 
    const uint8_t* nullData;
    uint16_t indicatorSize;
    
};

enum dataPageVar {HEADER_OFFSET_FROM_END, RECORD_OFFSET_FROM_BEGIN, SLOT_NUM};

class DataPage {
public:

    DataPage(const void* data);
    ~DataPage();

    void readRecord(FileHandle& fileHandle, const RID& rid, void* data);
    void writeRecord(Record &record, FileHandle &fileHandle, unsigned availablePage, RID &rid);
    void writeRecordFromTombstone(FileHandle& fileHandle, Record& record, uint32_t pageNum);
    void shiftRecords(uint16_t startPos, int16_t diff);
    

    void updateRecord(Record &record, FileHandle &fileHandle, const RID &rid);
    void insertTombstone(Tombstone &tombstone, FileHandle &fileHandle, const RID &rid);
    unsigned getFreeSpaceSize();
    std::pair<uint16_t,uint16_t> getIndexPair(uint16_t index);

    //HEADER_OFFSET_FROM_END, RECORD_OFFSET_FROM_BEGIN, SLOT_NUM
    uint16_t var[DATA_PAGE_VAR_NUM];
    std::pair<uint16_t,uint16_t>* pageHeader;   //  TODO: no use


    DataPage& operator=(const DataPage &dataPage);
    DataPage(const DataPage& d);

private:
    void* page;

};

struct Tombstone {
    uint8_t flag;
    uint32_t pageNum;
    uint16_t offsetFromBegin;
};
#endif