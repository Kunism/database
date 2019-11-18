#ifndef _ix_h_
#define _ix_h_

#include <vector>
#include <string>

#include "../rbf/rbfm.h"

# define IX_EOF (-1)  // end of the index scan

class IX_ScanIterator;

class IXFileHandle;

const uint32_t NODE_OFFSET = sizeof(uint32_t) + sizeof(bool) + sizeof(bool)  + sizeof(AttrType) + sizeof(uint32_t);

class Key {
public:
    int keyInt;
    float keyFloat;
    std::string keyString;
    RID rid;

    AttrType attrType;
    
    Key();
    Key(const void *key, const RID &rid, AttrType attrType);
    Key(char* data, AttrType attrType);
    bool operator < (const Key &k) const;
    uint32_t size() const;
    
    friend std::ostream& operator<<(std::ostream& os, const Key& key);
};



class BTreeNode {
public:
    uint32_t pageNum;
    bool isLeafNode;
    bool isDeleted;
    AttrType attrType;
    uint32_t rightNode;

    std::vector<Key> keys;
    std::vector<uint32_t> children;

    char* page;

    BTreeNode();
    ~BTreeNode();
    RC insertToLeaf(const Key &key);
    RC insertToInternal(const void *key, const int &childPageNum);
    RC updateMetaToDisk(IXFileHandle &ixFileHandle, bool isLeafNode, bool isDeleted, uint32_t rightNode);
    RC readNode(IXFileHandle &ixFileHandle, uint32_t pageNum);
    RC writeNode(IXFileHandle &ixFileHandle);

    RC searchKey(const Key &key);
    //RC compareKey(const char *key, const char *val);
    RC insertKey(const Key &key, uint32_t index);
    RC deleteKey(uint32_t index);
    RC getCurKeyNum();
    RC printKey();

    RC printRID();

    RC getChild(uint32_t index);
    RC getCurChildNum();
    RC insertChild(uint32_t childPageNum, uint32_t index);
    RC deleteChild(uint32_t index);

    uint32_t getKeysBegin();
    uint32_t getChildrenBegin();
    uint32_t getRecordsBegin();
    int32_t getFreeSpace();



private:


};

class BTree {
public:
    uint32_t rootPageNum;
    uint32_t totalPageNum;
    AttrType attrType;

    BTree();
    RC insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const Key &key);
    RC createNode(IXFileHandle &ixFileHandle, BTreeNode &node, uint32_t pageNum, bool isLeafNode, bool isDeleted
            , AttrType attrType, uint32_t rightNode);
    RC recInsert(IXFileHandle &ixFileHandle, const uint32_t nodePageNum, const Key &key,   // insert element
                bool &hasSplit, std::vector<std::pair<Key, uint32_t>> &pushEntries);        // return element
    RC recSearch(IXFileHandle &ixFileHandle, const Key &key, uint32_t pageNum);
    RC readBTreeHiddenPage(IXFileHandle &ixFileHandle);
    RC updateHiddenPageToDisk(IXFileHandle &ixFileHandle, uint32_t rootPageNum, uint32_t totalPageNum, AttrType attrType);
    RC findAvailablePage(IXFileHandle &ixFileHandle);
    //RC writeBTree(IXFileHandle &ixFileHandle);

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
    void recursivePrint(IXFileHandle &ixFileHandle, uint32_t pageNum, int depth) const;
    // string padding(int depth);
//    FileHandle* fileHandle;
};

class IX_ScanIterator {
public:

    IXFileHandle* ixFileHandle;
    Attribute attribute;
    Key lowKey;
    Key highKey;
    Key curKey;
    bool lowKeyInclusive;
    bool highKeyInclusive;

    BTree bTree;
    uint32_t curNodePageNum;
    uint32_t curIndex;
    bool finished;

    // Constructor
    IX_ScanIterator();

    // Destructor
    ~IX_ScanIterator();

    // Init
    RC init(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *lowKey, const void *highKey, bool lowKeyInclusive, bool highKeyInclusive);

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
