#include <iostream>
#include "ix.h"


BTreeNode::BTreeNode() {
    attrType = TypeInt;
    isLeafNode = false;
    pageNum = -1;

    keys = nullptr;
    children = nullptr;
}

RC BTreeNode::insertToLeaf(const void *key, const RID &rid) {

}

RC BTreeNode::insertToInternal(const void *key, const int &childPageNum) {

}

RC BTreeNode::readNode(IXFileHandle &ixFileHandle) {

}

RC BTreeNode::writeNode(IXFileHandle &ixFileHandle) {


}

BTree::BTree() {
    rootPageNum = 0;
    totalPageNum = 0;
}

RC BTree::insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid) {
    if(rootPageNum == 0) {
        uint32_t TMP_ORDER = 10;
        if(attribute.type == TypeInt) {
            //  TODO: calculate order
            //  ( sizeof(int) + sizeof(RID) ) * m + sizeof(p) + sizeof(metadata) <= PAGE_SIZE
            order = TMP_ORDER;
        }
        else if(attribute.type == TypeReal) {
            order = TMP_ORDER;
        }
        else if(attribute.type == TypeVarChar) {
            order = TMP_ORDER;
        }

        BTreeNode root;
        createNode(ixFileHandle, root, attribute.type, totalPageNum + 1, true);
        root.insertToLeaf(key, rid);
        updateMeta(ixFileHandle, rootPageNum + 1, totalPageNum + 1, attribute.type);
        root.writeNode(ixFileHandle);
    }
    else {
        BTreeNode node;
        node.readNode(ixFileHandle);
    }
}

RC BTree::createNode(IXFileHandle &ixFileHandle, BTreeNode &node, AttrType attrType, uint32_t pageNum, bool isLeafNode) {
    node.attrType = attrType;
    node.isLeafNode = isLeafNode;
    node.pageNum = pageNum;
}

RC BTree::updateMeta(IXFileHandle &ixFileHandle, uint32_t rootPageNum, uint32_t totalPageNum, AttrType attrType) {
    this->rootPageNum = rootPageNum;
    this->totalPageNum = totalPageNum;
    this->attrType = attrType;

    //  TODO: write back to file
    //  ixFileHandle.fileHandle.writePage(0, buffer);
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
    BTree bTree;
    readBTree(ixFileHandle, bTree);
    bTree.insertEntry(ixFileHandle, attribute, key, rid);
    //writeBTree();
    return -1;
}

RC IndexManager::deleteEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid) {
    return -1;
}

RC IndexManager::readBTree(IXFileHandle &ixFileHandle, BTree &bTree) {
    char* page = new char [PAGE_SIZE];
    ixFileHandle.fileHandle.readPage(0, page);

    //  TODO: parse page variables to bTree
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

