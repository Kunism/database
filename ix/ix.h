#ifndef _ix_h_
#define _ix_h_

#include <vector>
#include <string>

#include "../rbf/rbfm.h"

# define IX_EOF (-1)  // end of the index scan

class IX_ScanIterator;

class IXFileHandle;

const uint32_t NODE_OFFSET = sizeof(uint32_t) + sizeof(bool) + sizeof(bool) + sizeof(AttrType) + sizeof(AttrLength)
        + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);

class BTreeNode {
public:
    uint32_t pageNum;
    bool isLeafNode;
    bool isDeleted;
    AttrType attrType;
    AttrLength attrLength;      // 4 if type is int or float, 4 + maxLength of string if type is varchar
    uint32_t curKeyNum;
    uint32_t maxKeyNum;

    uint32_t rightNode;

    char* keys;
    int* children;              //  list of children's pageNum
    RID* records;

    char* page;

    BTreeNode();
    ~BTreeNode();
    RC insertToLeaf(const void *key, const RID &rid);
    RC insertToInternal(const void *key, const int &childPageNum);
    RC updateMeta(IXFileHandle &ixFileHandle, uint32_t pageNum, bool isLeafNode, bool isDeleted
            , uint32_t curKeyNum, uint32_t rightNode);
    RC readNode(IXFileHandle &ixFileHandle, uint32_t pageNum);
    RC writeNode(IXFileHandle &ixFileHandle);

    RC searchKey(const char *key);
    RC compareKey(const char *key, const char *val);

    uint32_t getKeysBegin();
    uint32_t getChildrenBegin();
    uint32_t getRecordsBegin();

    uint32_t getFreeSpace();



private:


};

class BTree {
public:
    uint32_t rootPageNum;
    uint32_t totalPageNum;
    AttrType attrType;
    AttrLength attrLength;
    uint32_t order;

    BTree();
    RC insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid);
    RC createNode(IXFileHandle &ixFileHandle, BTreeNode &node, uint32_t pageNum, bool isLeafNode, bool isDeleted
            , AttrType attrType, AttrLength attrLength, uint32_t order, uint32_t rightNode);
    RC updateMeta(IXFileHandle &ixFileHandle, uint32_t rootPageNum, uint32_t totalPageNum, AttrType attrType);
    RC recInsert(IXFileHandle &ixFileHandle, const void *key, const RID &rid, uint32_t nodePageNum
            , uint32_t splitNodePageNum, void * copyKey, bool &hasSplit);

    RC readBTree(IXFileHandle &ixFileHandle);
    RC writeBTree(IXFileHandle &ixFileHandle);
};

class IndexManager {

public:
    static IndexManager &instance();

    // Create an index file.
    RC createFile(const std::string &fileName);

    // Delete an index file.
    RC destroyFile(const std::string &fileName);

    // Open an index and return an ixFileHandle.
    RC openFile(const std::string &fileName, IXFileHandle &ixFileHandle);

    // Close an ixFileHandle for an index.
    RC closeFile(IXFileHandle &ixFileHandle);

    // Insert an entry into the given index that is indicated by the given ixFileHandle.
    RC insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid);

    // Delete an entry from the given index that is indicated by the given ixFileHandle.
    RC deleteEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid);

    // Initialize and IX_ScanIterator to support a range search
    RC scan(IXFileHandle &ixFileHandle,
            const Attribute &attribute,
            const void *lowKey,
            const void *highKey,
            bool lowKeyInclusive,
            bool highKeyInclusive,
            IX_ScanIterator &ix_ScanIterator);

    // Print the B+ tree in pre-order (in a JSON record format)
    void printBtree(IXFileHandle &ixFileHandle, const Attribute &attribute) const;

protected:
    IndexManager() = default;                                                   // Prevent construction
    ~IndexManager() = default;                                                  // Prevent unwanted destruction
    IndexManager(const IndexManager &) = default;                               // Prevent construction by copying
    IndexManager &operator=(const IndexManager &) = default;                    // Prevent assignment

private:
//    FileHandle* fileHandle;
};

class IX_ScanIterator {
public:

    // Constructor
    IX_ScanIterator();

    // Destructor
    ~IX_ScanIterator();

    // Get next matching entry
    RC getNextEntry(RID &rid, void *key);

    // Terminate index scan
    RC close();
};

class IXFileHandle {
public:

    // variables to keep counter for each operation
    unsigned ixReadPageCounter;
    unsigned ixWritePageCounter;
    unsigned ixAppendPageCounter;

    // Constructor
    IXFileHandle();

    // Destructor
    ~IXFileHandle();

    // Put the current counter values of associated PF FileHandles into variables
    RC collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount);
    FileHandle fileHandle;
};

#endif
