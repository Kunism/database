#include <iostream>
#include "ix.h"


BTreeNode::BTreeNode() {
    attrType = TypeInt;
    isLeafNode = false;
    isDeleted = false;
    pageNum = -1;
    curKeyNum = 0;
    maxKeyNum = 0;

    rightNode = 0;

    keys = nullptr;
    children = nullptr;

    page = new char [PAGE_SIZE];
    memset(page, 0, PAGE_SIZE);
}

BTreeNode::~BTreeNode() {
    delete[] page;
    delete[] keys;
    delete[] children;
    delete[] records;
}

RC BTreeNode::insertToLeaf(const void *key, const RID &rid) {   // TODO:

    //  in order to insert, need to move elements after index backward one unit
    uint32_t index = searchKey((char*)key);
    char* keyBuffer = new char [attrLength * (maxKeyNum - index + 1)];
    RID* ridBuffer = new RID [maxKeyNum - index + 1];
    memset(keyBuffer, 0 , attrLength * (maxKeyNum - index + 1));
    memset(ridBuffer, 0, sizeof(RID) * (maxKeyNum - index + 1));

    uint32_t keyLen = attrLength;
    if(attrType == TypeVarChar) {
        memcpy(&keyLen, key, sizeof(uint32_t));
        keyLen += sizeof(uint32_t);
    }

    //  newKey + oldKeys(bigger than newKey)
    memcpy(keyBuffer, key, keyLen);
    memcpy(keyBuffer + attrLength, page + getKeysBegin() + attrLength * index, attrLength * (maxKeyNum - index));
    memcpy(page + getKeysBegin() + attrLength * index, keyBuffer, attrLength * (maxKeyNum - index + 1));

    //  newRID + oldRIDs
    memcpy(ridBuffer, &rid, sizeof(RID));
    memcpy(ridBuffer + sizeof(RID), page + getRecordsBegin() + sizeof(RID) * index, sizeof(RID) * (maxKeyNum - index));
    memcpy(page + getRecordsBegin() + sizeof(RID) * index, ridBuffer, sizeof(RID) * (maxKeyNum - index + 1));

    return 0;
}

RC BTreeNode::insertToInternal(const void *key, const int &childPageNum) {

}

RC BTreeNode::readNode(IXFileHandle &ixFileHandle, uint32_t pageNum) {
    RC rc = 0;
    rc = ixFileHandle.fileHandle.readPage(pageNum, page);
    if(rc != 0) {
        return rc;
    }

    uint32_t offset = 0;

    memcpy(&pageNum, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&isLeafNode, page + offset, sizeof(bool));
    offset += sizeof(bool);

    memcpy(&isDeleted, page + offset, sizeof(bool));
    offset += sizeof(bool);

    memcpy(&attrType, page + offset, sizeof(AttrType));
    offset += sizeof(AttrType);

    memcpy(&attrLength, page + offset, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(&curKeyNum, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&maxKeyNum, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&rightNode, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    //  allocate spaces for keys, type difference are handled by attrLength
    keys = new char [attrLength * maxKeyNum];
    memset(keys, 0, attrLength * maxKeyNum);
    memcpy(keys, page + getKeysBegin(), attrLength * maxKeyNum);

    records = new RID [maxKeyNum];
    memset(records, 0, sizeof(RID) * maxKeyNum);
    memcpy(records, page + getRecordsBegin(), sizeof(RID) * maxKeyNum);

    children = (int*)(new char [attrLength * (maxKeyNum + 1)]);
    memset(children, 0, attrLength * (maxKeyNum + 1));
    memcpy(children, page + getChildrenBegin(), attrLength * (maxKeyNum + 1));

    return rc;
}

RC BTreeNode::writeNode(IXFileHandle &ixFileHandle) {
    memset(page, 0, PAGE_SIZE);

    uint32_t offset = 0;

    memcpy(page + offset, &pageNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &isLeafNode, sizeof(bool));
    offset += sizeof(bool);

    memcpy(page + offset, &isDeleted, sizeof(bool));
    offset += sizeof(bool);

    memcpy(page + offset, &attrType, sizeof(AttrType));
    offset += sizeof(AttrType);

    memcpy(page + offset, &attrLength, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(page + offset, &curKeyNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &maxKeyNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &rightNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + getKeysBegin(), keys, attrLength * maxKeyNum);

    if(isLeafNode) {
        memcpy(page + getRecordsBegin(), records, sizeof(RID) * maxKeyNum);
    }
    else {
        memcpy(page + getChildrenBegin(), children, attrLength * (maxKeyNum + 1));
    }

    if(offset != NODE_OFFSET) {
        std::cerr << "write error" << std::endl;
    }

    return ixFileHandle.fileHandle.writePage(pageNum, page);
}

RC BTreeNode::updateMeta(IXFileHandle &ixFileHandle, uint32_t pageNum, bool isLeafNode, bool isDeleted
        , uint32_t curKeyNum, uint32_t rightNode) {
    //  update meta variables
    this->pageNum = pageNum;
    this->isLeafNode = isLeafNode;
    this->isDeleted = isDeleted;
    this->curKeyNum = curKeyNum;
    this->rightNode = rightNode;

    //  write back to file
    uint32_t offset = 0;

    memcpy(page + offset, &this->pageNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &this->isLeafNode, sizeof(bool));
    offset += sizeof(bool);

    memcpy(page + offset, &this->isDeleted, sizeof(bool));
    offset += sizeof(bool);

    memcpy(page + offset, &attrType, sizeof(AttrType));
    offset += sizeof(AttrType);

    memcpy(page + offset, &attrLength, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(page + offset, &this->curKeyNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &maxKeyNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &this->rightNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    return writeNode(ixFileHandle);
}

RC BTreeNode::searchKey(const char *key) {
    //     1   2   4   6   8   9
    //  [0] [1] [2] [3] [4] [5]
    //  if key = 4, return 3
    //  if key = 5, return 3

    int result = 0;
    for(int i = 0; i < curKeyNum; i++) {

        uint32_t valLen = attrLength;
        if(attrType == TypeVarChar) {
            memcpy(&valLen, page + getKeysBegin() + attrLength * i, sizeof(uint32_t));
            valLen += sizeof(uint32_t);
        }

        char* val = new char [valLen];
        memset(val, 0, valLen);
        memcpy(val, page + getKeysBegin() + attrLength * i, valLen);

        if(compareKey(key, val) < 0) {
            result = i;
            delete[] val;
            break;
        }
        delete[] val;
    }
    return result;
}

RC BTreeNode::compareKey(const char *key, const char *val) {
    RC result = 0;

    if(attrType == TypeInt) {
        int _key, _val;
        memcpy(&_key, key, sizeof(int));
        memcpy(&_val, val, sizeof(int));
        if(_key > _val) result = 1;
        else if(_key == _val) result = 0;
        else if(_key < _val) result = -1;
    }
    else if(attrType == TypeReal) {
        float _key, _val;
        memcpy(&_key, key, sizeof(float));
        memcpy(&_val, val, sizeof(float));
        if(_key > _val) result = 1;
        else if(_key == _val) result = 0;
        else if(_key < _val) result = -1;
    }
    else if(attrType == TypeVarChar) {
        int keyLen, valLen;
        memcpy(&keyLen, key, sizeof(uint32_t));
        memcpy(&valLen, val, sizeof(uint32_t));

        if(keyLen < valLen) result = -1;
        else if(keyLen > valLen) result = 1;
        else {
            char* _key = new char [keyLen];
            char* _val = new char [valLen];

            memcpy(_key, key + sizeof(uint32_t), keyLen);
            memcpy(_val, val + sizeof(uint32_t), valLen);

            result = strcmp(_key, _val);

            delete[] _key;
            delete[] _val;
        }
    }


    return result;
}

uint32_t BTreeNode::getKeysBegin() {
    return NODE_OFFSET;
}

uint32_t BTreeNode::getChildrenBegin() {
    return NODE_OFFSET + attrLength * maxKeyNum;
}

uint32_t BTreeNode::getRecordsBegin() {
    return NODE_OFFSET + attrLength * maxKeyNum;;
}

uint32_t BTreeNode::getFreeSpace() {
    return maxKeyNum - curKeyNum;
}

BTree::BTree() {
    rootPageNum = 0;
    totalPageNum = 0;
    attrLength = 0;
}

RC BTree::insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid) {
    if(rootPageNum == 0) {
        uint32_t TMP_ORDER = 10;
        if(attribute.type == TypeInt) {
            attrLength = sizeof(int);
            //  TODO: calculate order
            //  ( sizeof(int) + sizeof(RID) ) * m + sizeof(p) + sizeof(metadata) <= PAGE_SIZE
            order = TMP_ORDER;

        }
        else if(attribute.type == TypeReal) {
            attrLength = sizeof(float);
            order = TMP_ORDER;

        }
        else if(attribute.type == TypeVarChar) {
            attrLength = attribute.length + sizeof(uint32_t);
            order = TMP_ORDER;

        }

        BTreeNode root;
        createNode(ixFileHandle, root, totalPageNum + 1, true, false
                , attribute.type, attrLength, order, 0);
        root.insertToLeaf(key, rid);
        root.updateMeta(ixFileHandle, totalPageNum + 1, root.isLeafNode, root.isDeleted
                , root.curKeyNum + 1, root.rightNode);
        updateMeta(ixFileHandle, rootPageNum + 1, totalPageNum + 1, attribute.type);

    }
    else {
        BTreeNode node;
        node.readNode(ixFileHandle, rootPageNum);
        bool split = false;

        //  allocate space for copykey, if split is needed
        char* copyKey;
        if(attribute.type == TypeInt) {
            copyKey = new char [sizeof(int)];
            memset(copyKey, 0, sizeof(int));
        }
        else if(attribute.type == TypeReal) {
            copyKey = new char [sizeof(float)];
            memset(copyKey, 0, sizeof(float));
        }
        else if(attribute.type == TypeVarChar) {
            uint32_t strLen = 0;
            memcpy(&strLen, key, sizeof(uint32_t));

            copyKey = new char [sizeof(uint32_t) + strLen];
            memset(copyKey, 0, sizeof(uint32_t) + strLen);
        }

        recInsert(ixFileHandle, key, rid, node.pageNum, -1, copyKey, split);
    }
}

RC BTree::createNode(IXFileHandle &ixFileHandle, BTreeNode &node, uint32_t pageNum, bool isLeafNode, bool isDeleted
        , AttrType attrType, AttrLength attrLength, uint32_t order, uint32_t rightNode) {
    node.pageNum = pageNum;
    node.isLeafNode = isLeafNode;
    node.isDeleted = isDeleted;
    node.attrType = attrType;
    node.attrLength = attrLength;
    node.curKeyNum = 0;
    node.maxKeyNum = order;
    node.rightNode = rightNode;

    uint32_t offset = 0;

    //  write node metadata in the beginning of page
    memcpy(node.page + offset, &pageNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(node.page + offset, &isLeafNode, sizeof(bool));
    offset += sizeof(bool);

    memcpy(node.page + offset, &isDeleted, sizeof(bool));
    offset += sizeof(bool);

    memcpy(node.page + offset, &attrType, sizeof(AttrType));
    offset += sizeof(AttrType);

    //  attrLength = 4 if int or float. if type is varchar, attrLength = 4 + charLength
    memcpy(node.page + offset, &attrLength, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(node.page + offset, &node.curKeyNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(node.page + offset, &order, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(node.page + offset, &rightNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    node.keys = new char [node.attrLength * node.maxKeyNum];
    memset(node.keys, 0, attrLength * node.maxKeyNum);

    node.records = new RID [node.maxKeyNum];
    memset(node.records, 0, sizeof(RID) * node.maxKeyNum);

    node.children = (int*)(new char [attrLength * (node.maxKeyNum + 1)]);
    memset(node.children, 0, attrLength * (node.maxKeyNum + 1));

    if(offset != NODE_OFFSET) {
        std::cerr << "Memcpy Error in BTree::createNode" << std::endl;
    }

    return 0;
}


RC BTree::updateMeta(IXFileHandle &ixFileHandle, uint32_t rootPageNum, uint32_t totalPageNum, AttrType attrType) {
    this->rootPageNum = rootPageNum;
    this->totalPageNum = totalPageNum;
    this->attrType = attrType;

    writeBTree(ixFileHandle);
}

RC BTree::recInsert(IXFileHandle &ixFileHandle, const void *key, const RID &rid, uint32_t nodePageNum, uint32_t splitNode,
                    void *copyKey, bool &hasSplit) {
    BTreeNode node;
    node.readNode(ixFileHandle, nodePageNum);
    if(node.isLeafNode) {
        if(node.getFreeSpace() > 0) {
            node.insertToLeaf(key, rid);
            //node.updateMeta(ixFileHandle, node.pageNum, node.getIsLeafNode(), node.getIsDeleted(), node.getCurKeyNum() + 1, )
        }
        else {

        }
    }
    else {

    }
}

RC BTree::readBTree(IXFileHandle &ixFileHandle) {
    RC rc = 0;
    char* hiddenPage = new char [PAGE_SIZE];
    memset(hiddenPage, 0, PAGE_SIZE);

    std::cerr << "Page Nums: " << ixFileHandle.fileHandle.getNumberOfPages() << std::endl;
    rc = ixFileHandle.fileHandle.readPage(0, hiddenPage);

    if(rc != 0) {
        std::cerr << "ReadPage fail in BTree::readBTree." << std::endl;
        return rc;
    }

    uint32_t offset = 0;

    memcpy(&rootPageNum, hiddenPage + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&totalPageNum, hiddenPage + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&attrType, hiddenPage + offset, sizeof(AttrType));
    offset += sizeof(AttrType);

    memcpy(&attrLength, hiddenPage + offset, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(&order, hiddenPage + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    delete[] hiddenPage;

    return rc;
}

RC BTree::writeBTree(IXFileHandle &ixFileHandle) {
    char* hiddenPage = new char [PAGE_SIZE];
    memset(hiddenPage, 0, PAGE_SIZE);

    uint32_t offset = 0;

    memcpy(hiddenPage + offset, &rootPageNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(hiddenPage + offset, &totalPageNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(hiddenPage + offset, &attrType, sizeof(AttrType));
    offset += sizeof(AttrType);

    memcpy(hiddenPage + offset, &attrLength, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(hiddenPage + offset, &order, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    delete[] hiddenPage;

    return ixFileHandle.fileHandle.writePage(0, hiddenPage);
}


IndexManager &IndexManager::instance() {
    static IndexManager _index_manager = IndexManager();
    return _index_manager;
}

RC IndexManager::createFile(const std::string &fileName) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.createFile(fileName);
}

RC IndexManager::destroyFile(const std::string &fileName) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.destroyFile(fileName);
}

RC IndexManager::openFile(const std::string &fileName, IXFileHandle &ixFileHandle) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.openFile(fileName, ixFileHandle.fileHandle);
}

RC IndexManager::closeFile(IXFileHandle &ixFileHandle) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.closeFile(ixFileHandle.fileHandle);
}

RC IndexManager::insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid) {
    RC rc = 0;
    BTree bTree;
    rc |= bTree.readBTree(ixFileHandle);
    rc |= bTree.insertEntry(ixFileHandle, attribute, key, rid);
    rc |= bTree.writeBTree(ixFileHandle);
    return rc;
}

RC IndexManager::deleteEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid) {
    return -1;
}

RC IndexManager::scan(IXFileHandle &ixFileHandle,
                      const Attribute &attribute,
                      const void *lowKey,
                      const void *highKey,
                      bool lowKeyInclusive,
                      bool highKeyInclusive,
                      IX_ScanIterator &ix_ScanIterator) {
    return -1;
}

void IndexManager::printBtree(IXFileHandle &ixFileHandle, const Attribute &attribute) const {
}

IX_ScanIterator::IX_ScanIterator() {
}

IX_ScanIterator::~IX_ScanIterator() {
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key) {
    return -1;
}

RC IX_ScanIterator::close() {
    return -1;
}

IXFileHandle::IXFileHandle() {
    ixReadPageCounter = 0;
    ixWritePageCounter = 0;
    ixAppendPageCounter = 0;
}

IXFileHandle::~IXFileHandle() {
}

RC IXFileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
    return fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
}

