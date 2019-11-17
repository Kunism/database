#include <iostream>
#include <iomanip>
#include "ix.h"

Key::Key(const void *key, const RID &rid, AttrType attrType) {
    this->attrType = attrType;

    if(attrType == TypeInt) {
        memcpy(&keyInt, key, sizeof(int));
        memcpy(&this->rid, &rid, sizeof(RID));
    }
    else if(attrType == TypeReal) {
        memcpy(&keyFloat, key, sizeof(float));
        memcpy(&this->rid, &rid, sizeof(RID));
    }
    else if(attrType == TypeVarChar) {
        uint32_t strLen = 0;
        memcpy(&strLen, key, sizeof(uint32_t));

        char* strBuffer = new char [strLen];
        memcpy(strBuffer, (char*)key + sizeof(uint32_t), strLen);
        keyString = std::string(strBuffer);
        delete[] strBuffer;

        memcpy(&this->rid, &rid, sizeof(RID));
    }
}

Key::Key(char *data, AttrType attrType) {
    this->attrType = attrType;

    uint32_t actualKeyLen = 0;

    if(attrType == TypeInt) {
        //  read key
        actualKeyLen = sizeof(int);
        memcpy(&keyInt, data, actualKeyLen);

        //  read rid
        memcpy(&rid, data + actualKeyLen, sizeof(RID));
    }
    else if(attrType == TypeReal) {
        //  read key
        actualKeyLen = sizeof(float);
        memcpy(&keyFloat, data, actualKeyLen);

        //  read rid
        memcpy(&rid, data + actualKeyLen, sizeof(RID));
    }
    else if(attrType == TypeVarChar) {
        //  read key length
        memcpy(&actualKeyLen, data, sizeof(uint32_t));

        //  read key
        char* strBuffer = new char [actualKeyLen];
        memcpy(strBuffer, data + sizeof(uint32_t), actualKeyLen);
        keyString = std::string(strBuffer);

        //  read rid
        memcpy(&rid, data + sizeof(uint32_t) + actualKeyLen, sizeof(RID));

        delete[] strBuffer;
    }
}

bool Key::operator<(const Key &k) const {
    bool res = false;
    if(attrType == TypeInt) {
        if(this->keyInt == k.keyInt) {
            if(this->rid.pageNum == k.rid.pageNum) {
                res = this->rid.slotNum < k.rid.slotNum;
            }
            else res = this->rid.pageNum < k.rid.pageNum;
        }
        else res = this->keyInt < k.keyInt;
    }
    else if(attrType == TypeReal) {
        if(this->keyFloat == k.keyFloat) {
            if(this->rid.pageNum == k.rid.pageNum) {
                res = this->rid.slotNum < k.rid.slotNum;
            }
            else res = this->rid.pageNum < k.rid.pageNum;
        }
        else res = this->keyFloat < k.keyFloat;
    }
    else if(attrType == TypeVarChar) {
        if(this->keyString.length() == k.keyString.length()) {
            res = !strcmp(this->keyString.c_str(), k.keyString.c_str());
        }
        else res = this->keyString.length() < k.keyString.length();
    }

    return res;
}

uint32_t Key::size() const {
    uint32_t size = 0;
    if(attrType == TypeInt) {
        size = sizeof(uint32_t) + sizeof(int) + sizeof(RID);
    }
    else if(attrType == TypeReal) {
        size = sizeof(uint32_t) + sizeof(float) + sizeof(RID);
    }
    else if(attrType == TypeVarChar) {
        size = sizeof(uint32_t) + sizeof(uint32_t) + keyString.length() + sizeof(RID);
    }
    return size;
}

std::ostream& operator<<(std::ostream& os, const Key& key) {
    if(key.attrType == TypeInt) {
        os << key.keyInt;
        return os;
    }
    else if(key.attrType == TypeReal) {
        os << key.keyFloat;
        return os;
    }
    else if(key.attrType == TypeVarChar) {
        os << key.keyString;
        return os;
    }
}

BTreeNode::BTreeNode() {
    attrType = TypeInt;
    isLeafNode = false;
    isDeleted = false;
    pageNum = -1;

    rightNode = -1;

    keys.clear();
    children.clear();

    page = new char [PAGE_SIZE];
    memset(page, 0, PAGE_SIZE);
}

BTreeNode::~BTreeNode() {
    delete[] page;
}

RC BTreeNode::insertToLeaf(const Key &key) {

    // TODO: need to check leaf is full???

    //  in order to insert, need to move elements after index backward one unit
    uint32_t index = searchKey(key);

    insertKey(key, index);
    // TO DO NEED WRITE ????
    return 0;
}

RC BTreeNode::insertToInternal(const void *key, const int &childPageNum) {
    // TODO
    // TODO
}

RC BTreeNode::readNode(IXFileHandle &ixFileHandle, uint32_t pageNum) {

    RC rc = 0;
    rc = ixFileHandle.fileHandle.readBTreePage(pageNum, page);
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

    memcpy(&rightNode, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    //  read keyNum
    uint32_t keyNum = 0;
    memcpy(&keyNum, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    //  read key to vector
    for(int i = 0; i < keyNum; i++) {
        //  keyDataLength = keyLen + ridLen
        uint32_t keyDataLength = 0;
        memcpy(&keyDataLength, page + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        char* keyData = new char [keyDataLength];
        memcpy(keyData, page + offset, keyDataLength);
        offset += keyDataLength;

        std::cerr << "Before push" << std::endl;
        keys.push_back(Key(keyData, attrType));

        std::cerr << "Key Int = " << keys[0].keyInt << " \tRid = {" << keys[0].rid.pageNum << ", " << keys[0].rid.slotNum << "}" << std::endl;

        delete[] keyData;
    }

    //  read child to vector
    for(int i = 0; i < children.size(); i++) {
        uint32_t child = 0;
        memcpy(&child, page + PAGE_SIZE - sizeof(uint32_t) * (i+1), sizeof(uint32_t));
        children.push_back(child);
    }

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

//    memcpy(page + offset, &attrLength, sizeof(AttrLength));
//    offset += sizeof(AttrLength);
//
//    memcpy(page + offset, &curKeyNum, sizeof(uint32_t));
//    offset += sizeof(uint32_t);
//
//    memcpy(page + offset, &curChildNum, sizeof(uint32_t));
//    offset += sizeof(uint32_t);

    memcpy(page + offset, &rightNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    uint32_t keyNum = keys.size();
    memcpy(page + offset, &keyNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    //  write key vector
    for(int i = 0; i < keys.size(); i++) {

        //  keyDataLength = keyLen + ridLen
        uint32_t keyDataLength = 0;

        if(attrType == TypeInt) {
            keyDataLength = sizeof(int) + sizeof(RID);
            memcpy(page + offset, &keyDataLength, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            memcpy(page + offset, &keys[i].keyInt, sizeof(int));
            offset += sizeof(int);

            memcpy(page + offset, &keys[i].rid, sizeof(RID));
            offset += sizeof(RID);
        }
        else if(attrType == TypeReal) {
            keyDataLength = sizeof(float) + sizeof(RID);
            memcpy(page + offset, &keyDataLength, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            memcpy(page + offset, &keys[i].keyFloat, sizeof(float));
            offset += sizeof(float);

            memcpy(page + offset, &keys[i].rid, sizeof(RID));
            offset += sizeof(RID);
        }
        else if(attrType == TypeVarChar) {
            keyDataLength = sizeof(uint32_t) + keys[i].keyString.length() + sizeof(RID);
            memcpy(page + offset, &keyDataLength, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            uint32_t strLen = keys[i].keyString.length();
            memcpy(page + offset, &strLen, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            memcpy(page + offset, keys[i].keyString.c_str(), strLen);
            offset += strLen;

            memcpy(page + offset, &keys[i].rid, sizeof(RID));
            offset += sizeof(RID);
        }
    }

    //  write children vector
    for(int i = 0; i < children.size(); i++) {
        memcpy(page + PAGE_SIZE - sizeof(uint32_t) * (i + 1), &children[i], sizeof(uint32_t));
    }

    return ixFileHandle.fileHandle.writeBTreePage(pageNum, page);
}

RC BTreeNode::updateMetaToDisk(IXFileHandle &ixFileHandle, bool isLeafNode, bool isDeleted, uint32_t rightNode) {
    //  update meta variables
    this->pageNum = pageNum;
    this->isLeafNode = isLeafNode;
    this->isDeleted = isDeleted;
    this->rightNode = rightNode;

    return writeNode(ixFileHandle);
}

RC BTreeNode::searchKey(const Key &key) {
    //     1   2   4   6   8   9
    //  [0] [1] [2] [3] [4] [5]
    //  if key = 4, return 3
    //  if key = 5, return 3

    int result = 0;

    for(int i = 0; i < keys.size(); i++) {
        if(key < keys[i]) {
            result = i;
            break;
        }
    }

    return result;
}



RC BTreeNode::insertKey(const Key &key, uint32_t index) {
    keys.insert(keys.begin() + index, key);
    return 0;
}

RC BTreeNode::deleteKey(uint32_t index) {
    keys.erase(keys.begin() + index);
    return 0;
}

RC BTreeNode::printKey() {
    for(int i = 0; i < keys.size(); i++) {
        if(attrType == TypeInt) {
            std::cerr << "key = " << keys[i].keyInt << std::endl;
        }
        else if(attrType == TypeReal) {
            std::cerr << "key = " << keys[i].keyFloat << std::endl;
        }
        else if(attrType == TypeVarChar) {
            std::cerr << "key = " << keys[i].keyString << std::endl;
        }
    }
    return 0;
}

RC BTreeNode::printRID() {
    for(int i = 0; i < keys.size(); i++) {
        std::cerr << "RID:  " << keys[i].rid.pageNum << "\t" << keys[i].rid.slotNum << std::endl;
    }
}

RC BTreeNode::getChild(uint32_t index) {
    if(index < children.size()) {
        return children[index];
    }
    else {
        return -1;
    }
}

RC BTreeNode::insertChild(uint32_t childPageNum, uint32_t index) {
    if(index < children.size()) {
        children.insert(children.begin() + index, childPageNum);
        return 0;
    }
    else {
        return -1;
    }
}

RC BTreeNode::deleteChild(uint32_t index) {
    children.erase(children.begin() + index);
    return 0;
}

uint32_t BTreeNode::getKeysBegin() {
    return NODE_OFFSET;
}

//  first child addr.
uint32_t BTreeNode::getChildrenBegin() {
    return PAGE_SIZE - sizeof(uint32_t);
}

int32_t BTreeNode::getFreeSpace() {

    uint32_t keysSize = 0;
    uint32_t childrenSize = sizeof(uint32_t) * children.size();

    for(int i = 0; i < keys.size(); i++) {
        keysSize += keys[i].size();
    }

    return (int32_t)PAGE_SIZE - (int32_t)(NODE_OFFSET + keysSize + childrenSize);
}


BTree::BTree() {
    rootPageNum = -1;
    totalPageNum = 0;
}

RC BTree::insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const Key &key) {

    if(rootPageNum == -1) {  // no root

        if(attribute.type == TypeInt) {
            //  TODO: calculate order
            //  ( sizeof(int) + sizeof(RID) ) * m + sizeof(p) + sizeof(metadata) <= PAGE_SIZE
        }
        else if(attribute.type == TypeReal) {

        }
        else if(attribute.type == TypeVarChar) {

        }

        BTreeNode root;
        uint32_t newRootPageNum = totalPageNum;
        createNode(ixFileHandle, root, newRootPageNum, true, false
                , attribute.type, -1);
        this->updateHiddenPageToDisk(ixFileHandle, newRootPageNum, totalPageNum + 1, attribute.type);
        
        // can use general insert instead.

        root.insertToLeaf(key);
        root.updateMetaToDisk(ixFileHandle, root.isLeafNode, root.isDeleted, root.rightNode);
        
    }
    else {
        // BTreeNode node;
        // node.readNode(ixFileHandle, rootPageNum);
        bool split = false;
        uint32_t upPageNum;
        std::vector<std::pair<Key, uint32_t>> pushEntities;
        //  allocate space for copykey, if split is needed

        recInsert(ixFileHandle, rootPageNum, key, split, pushEntities);
        if(split)
        {
            BTreeNode newRoot;
            uint32_t newRootPageNum = totalPageNum;
            createNode(ixFileHandle, newRoot, newRootPageNum, false, false, this->attrType, -1);

            for(auto i : pushEntities)
            {
                newRoot.insertKey(i.first, newRoot.keys.size());
            }

            newRoot.insertChild(rootPageNum, newRoot.children.size());
            for(auto i : pushEntities)
            {
                newRoot.insertChild(i.second, newRoot.children.size());
            }

            newRoot.writeNode(ixFileHandle);
            updateHiddenPageToDisk(ixFileHandle, newRootPageNum, totalPageNum+1, attrType);
        }
    }
    return 0;
}

RC BTree::createNode(IXFileHandle &ixFileHandle, BTreeNode &node, uint32_t pageNum, bool isLeafNode, bool isDeleted
        , AttrType attrType, uint32_t rightNode) {
    node.pageNum = pageNum;
    node.isLeafNode = isLeafNode;
    node.isDeleted = isDeleted;
    node.attrType = attrType;
    node.rightNode = rightNode;

    uint32_t offset = 0;
    memset(node.page, 0, PAGE_SIZE);

    //  write node metadata in the beginning of page
    memcpy(node.page + offset, &pageNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(node.page + offset, &isLeafNode, sizeof(bool));
    offset += sizeof(bool);

    memcpy(node.page + offset, &isDeleted, sizeof(bool));
    offset += sizeof(bool);

    memcpy(node.page + offset, &attrType, sizeof(AttrType));
    offset += sizeof(AttrType);

    memcpy(node.page + offset, &rightNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if(offset != NODE_OFFSET) {
        std::cerr << "Memcpy Error in BTree::createNode" << std::endl;
    }

    //  update hiddenPage
    ixFileHandle.fileHandle.createNodePage(node.page);
    // updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
    return 0;
}


RC BTree::updateHiddenPageToDisk(IXFileHandle &ixFileHandle, uint32_t rootPageNum, uint32_t totalPageNum, AttrType attrType) {

    RC rc;
    //TO DO rc unint
    unsigned data[HIDDEN_PAGE_VAR_NUM];
    
    rc |= ixFileHandle.fileHandle.readBTreeHiddenPage(data);
    
    this->rootPageNum = rootPageNum;
    this->totalPageNum = totalPageNum;
    this->attrType = attrType;

    data[ROOT_PAGE_NUM] = rootPageNum;
    data[TOTAL_PAGE_NUM] = totalPageNum;
    data[ATTRTYPE] = attrType;

    rc |= ixFileHandle.fileHandle.writeBTreeHiddenPage(data);

    return rc;
}

RC BTree::findAvailablePage(IXFileHandle &ixFileHandle) {
    for(int i = 0; i < totalPageNum; i++) {
        BTreeNode node;
        node.readNode(ixFileHandle, i);
        if(node.isDeleted) {
            return i;
        }
    }
    return -1;
}

RC BTree::recInsert(IXFileHandle &ixFileHandle, const uint32_t nodePageNum, const Key &key,   // insert element
                    bool &hasSplit, std::vector<std::pair<Key, uint32_t>> &pushEntries) {     // return element

    BTreeNode node;
    node.readNode(ixFileHandle, nodePageNum);
    if(node.isLeafNode) {  
        // leaf only insert key
        if(node.getFreeSpace() > key.size()) {
            node.insertToLeaf(key);
            node.writeNode(ixFileHandle);
            hasSplit = false;
            return 0;

        }
        else {
            std::vector<Key> temp(node.keys.begin(), node.keys.end());
            temp.insert(temp.begin() + node.searchKey(key), key);

            // split index
            int startIndex = 0;
            int totalSize = NODE_OFFSET;    
            for(;startIndex < temp.size() && totalSize < PAGE_SIZE / 2 ; startIndex++)
            {
                totalSize += temp[startIndex].size();
            }


            BTreeNode newNode;
            uint32_t newNodePageNum = totalPageNum;
            createNode(ixFileHandle, newNode, newNodePageNum, true, false, this->attrType, node.rightNode);
            updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
            node.updateMetaToDisk(ixFileHandle, node.isLeafNode, node.isDeleted, newNodePageNum);
            // prepare for upKey;
            pushEntries.push_back({temp[startIndex], newNodePageNum});

            for(int i = startIndex ; i < temp.size() ; i++)
            {
                if( newNode.getFreeSpace() > temp[i].size()) {
                    newNode.insertToLeaf(temp[i]);
                }
                else {
                    newNode.writeNode(ixFileHandle);
                    newNodePageNum = totalPageNum;
                    uint32_t oldRightNode = newNode.rightNode;
                    newNode.updateMetaToDisk(ixFileHandle, newNode.isLeafNode, newNode.isDeleted, newNodePageNum);
                    createNode(ixFileHandle, newNode, newNodePageNum, true, false, this->attrType, oldRightNode);
                    updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
                    // prepare for upKey;
                    pushEntries.push_back({temp[i], newNodePageNum});
                    newNode.insertToLeaf(temp[i]);
                }
            }
            
            newNode.writeNode(ixFileHandle);
            // delete overflow keys in vector   

            while(node.keys.size() > startIndex)
            {
                node.keys.pop_back();
            }

            hasSplit = true;
            return 0;
        }
    }
    else {
        
        int subTreePos = node.searchKey(key);
        uint32_t childPageNum = node.getChild(subTreePos);
        recInsert(ixFileHandle, childPageNum, key, hasSplit, pushEntries);
        if( hasSplit ) {
            int totalPushSize = 0;
            for(auto i : pushEntries) {
                totalPushSize += i.first.size();
                totalPushSize += sizeof(uint32_t);
            }

            if (node.getFreeSpace() >  totalPushSize) {

                for(int i = 0 ; i < pushEntries.size() ; i++)
                {
                    int searchPos = node.searchKey(pushEntries[i].first);
                    node.insertKey(pushEntries[i].first, searchPos);
                    node.insertChild(pushEntries[i].second, searchPos + 1);
                }
                node.writeNode(ixFileHandle);
                hasSplit = false;
                return 0;
            }
            else {
                // std::vector<Key> temp(node.keys.begin(), node.keys.end());
                // find proper place to insert
                
                for(auto i : pushEntries) {
                    uint32_t insertPos = node.searchKey(key);
                    node.insertKey(i.first, insertPos);
                    node.insertChild(i.second, insertPos);
                }
                
                
                // split index
                int pushIndex = 0;
                int totalSize = NODE_OFFSET; 
                for(;pushIndex < node.keys.size() && totalSize < PAGE_SIZE / 2 ; pushIndex++)
                {
                    totalSize += node.keys[pushIndex].size();
                    totalSize += sizeof(uint32_t);
                }


                BTreeNode newNode;
                uint32_t newNodePageNum = totalPageNum;
                createNode(ixFileHandle, newNode, newNodePageNum, false, false, this->attrType, -1);
                updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
                // vector need to delete too
                // node.updateMetaToDisk(ixFileHandle, node.pageNum, node.isLeafNode, node.isDeleted, pushIndex, newNodePageNum);


                // New Node
                for(int i = pushIndex + 1 ; i < node.keys.size() ; i++) {
                    if( newNode.getFreeSpace() > node.keys[i].size() + sizeof(uint32_t)) {
                        newNode.insertKey(node.keys[i], newNode.keys.size());
                        newNode.insertChild(node.children[i], newNode.children.size());
                    }
                    else {
                        newNode.writeNode(ixFileHandle);
                        newNodePageNum = totalPageNum;
                        createNode(ixFileHandle, newNode, newNodePageNum, false, false, node.attrType, -1);
                        updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
                        // prepare for upKey;
                        pushEntries.push_back({node.keys[i], newNodePageNum});
                        newNode.insertKey(node.keys[i], newNode.keys.size());
                        newNode.insertChild(node.children[i], newNode.children.size());
                    }
                    
                }

                // insert last child
                newNode.insertChild(node.children.back(), newNode.children.size());
                newNode.writeNode(ixFileHandle);

                while(node.keys.size() > pushIndex)
                {
                    node.keys.pop_back();
                }

                while(node.children.size() > pushIndex + 1)
                {
                    node.children.pop_back();
                }

                hasSplit = true;
                return 0;
            }
        }        
    }
}

RC BTree::recSearch(IXFileHandle &ixFileHandle, const Key &key, uint32_t pageNum) {
    BTreeNode node;
    node.readNode(ixFileHandle, pageNum);

    if(node.isLeafNode) {
        return node.pageNum;
    }
    else {
        uint32_t index = node.searchKey(key);
        return recSearch(ixFileHandle, key, node.getChild(index));
    }
}

RC BTree::readBTreeHiddenPage(IXFileHandle &ixFileHandle) {
    // char* hiddenPage = new char [PAGE_SIZE];
    // memset(hiddenPage, 0, PAGE_SIZE);
    unsigned data[HIDDEN_PAGE_VAR_NUM];
    if( ixFileHandle.fileHandle.readBTreeHiddenPage(data) != 0) {
        std::cerr << "ReadPage fail in BTree::readBTree." << std::endl;
        return -1;
    }

    rootPageNum = data[ROOT_PAGE_NUM];
    totalPageNum = data[TOTAL_PAGE_NUM];
    attrType = (AttrType)data[ATTRTYPE];
    
    return 0;
    //  var in the beginning belongs to RBF
    // uint32_t offset = sizeof(uint32_t) * NON_BTREE_VAR_NUM;

    // memcpy(&rootPageNum, hiddenPage + offset, sizeof(uint32_t));
    // offset += sizeof(uint32_t);

    // memcpy(&totalPageNum, hiddenPage + offset, sizeof(uint32_t));
    // offset += sizeof(uint32_t);

    // memcpy(&attrType, hiddenPage + offset, sizeof(AttrType));
    // offset += sizeof(AttrType);

    // memcpy(&attrLength, hiddenPage + offset, sizeof(AttrLength));
    // offset += sizeof(AttrLength);

    // memcpy(&order, hiddenPage + offset, sizeof(uint32_t));
    // offset += sizeof(uint32_t);

    // delete[] hiddenPage;

    
}

IndexManager &IndexManager::instance() {
    static IndexManager _index_manager = IndexManager();
    return _index_manager;
}

RC IndexManager::createFile(const std::string &fileName) {
    PagedFileManager& pfm = PagedFileManager::instance();
    return pfm.createFile(fileName);
    // hidden page init properly!!
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
    rc |= bTree.readBTreeHiddenPage(ixFileHandle);
    rc |= bTree.insertEntry(ixFileHandle, attribute, Key(key, rid, attribute.type));
    //rc |= bTree.writeBTree(ixFileHandle);
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

    RC rc = 0;
    rc |= ix_ScanIterator.init(ixFileHandle, attribute, lowKey, highKey, lowKeyInclusive, highKeyInclusive);


    return rc;
}

void IndexManager::printBtree(IXFileHandle &ixFileHandle, const Attribute &attribute) const {

    BTree bTree;
    bTree.readBTreeHiddenPage(ixFileHandle);
    // BTreeNode root;
    // std::cerr << "totalPageNum = " << bTree.totalPageNum << std::endl;
    // std::cerr << "rootPageNum = " << bTree.rootPageNum << std::endl;
    // root.readNode(ixFileHandle, bTree.rootPageNum);
    // std::cerr << "curKeyNum = " << root.keys.size() << std::endl;

    // root.printKey();
    // root.printRID();
    recursivePrint(ixFileHandle, bTree.rootPageNum, 0);
    std::cout << std::endl;

}

void IndexManager::recursivePrint(IXFileHandle &ixFileHandle, uint32_t pageNum, int depth) const
{
    BTreeNode btNode;
    btNode.readNode(ixFileHandle, pageNum);
   
    std::cout << std::setw(4*depth) << " " << "{\"keys\": [";
    
    for(int i = 0 ; i < btNode.keys.size() ; i++) {
        std::cout << "\"" <<    btNode.keys[i];
        if(btNode.isLeafNode) {
            std::cout <<":[(" << btNode.keys[i].rid.pageNum << ',' << btNode.keys[i].rid.slotNum << ")]";
        }
        std::cout << "\"";
        if(i != btNode.keys.size()-1)
        {
            std::cout << ",";
        }
    }
    std::cout << "]"; 
    
    if(!btNode.isLeafNode)
    {
        std::cout << "," <<std::endl;
        std::cout << std::setw(4*depth) << "\"children\":[\"" << std::endl;
        for(int i = 0 ; i < btNode.children.size() ; i++) {
            recursivePrint(ixFileHandle, btNode.children[i], depth+1);
            if(i != btNode.children.size()-1)
            {
                std::cout << "," << std::endl;
            }
        }
        std::cout << std::endl;
        std::cout << std::setw(4*depth) << " " << "]";
    }
    std::cout << "}"; 
}

// void IndexManager::padding(int depth)
// {
//     for(int i = 0 ; i < depth ; i++)
//     {
//         std::cout << '\t';
//     }
// }



IX_ScanIterator::IX_ScanIterator() {
    curNodePageNum = -1;
    curIndex = -1;
    finished = false;
}

IX_ScanIterator::~IX_ScanIterator() {
}

RC IX_ScanIterator::init(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *lowKey,
                         const void *highKey, bool lowKeyInclusive, bool highKeyInclusive) {
    this->ixFileHandle = &ixFileHandle;
    this->attribute = attribute;
    //  TODO: need space?
    this->lowKey = lowKey;
    this->highKey = highKey;
    this->lowKeyInclusive = lowKeyInclusive;
    this->highKeyInclusive = highKeyInclusive;




    return 0;
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key) {

    if(finished) {
        return IX_EOF;
    }
    //  first time to call getNext, so search tree and return the left most leaf
    else if(curNodePageNum == -1) {
        if(lowKey == NULL) {

            bTree.readBTreeHiddenPage(*ixFileHandle);
            curNodePageNum = bTree.recSearch(*ixFileHandle, Key(key, {0, 0}, attribute.type), bTree.rootPageNum);

            BTreeNode node;
            node.readNode(*ixFileHandle, curNodePageNum);
            curIndex = node.searchKey(Key(key, {0, 0}, attribute.type));

        }
        else {
            bTree.readBTreeHiddenPage(*ixFileHandle);
            curNodePageNum = bTree.recSearch(*ixFileHandle, Key(lowKey, {0, 0}, attribute.type), bTree.rootPageNum);

            BTreeNode node;
            node.readNode(*ixFileHandle, curNodePageNum);
            curIndex = node.searchKey(Key(lowKey, {0, 0}, attribute.type));
        }



    }
    //  iterator already on leaf node, so keep read more key in leaf node, or go to next leaf node
    else {
        BTreeNode node;
        node.readNode(*ixFileHandle, curNodePageNum);
    }


//    rid = node.keys[curIndex].rid;
//
//    curIndex++;
//    if(curIndex >= node.keys.size()) {
//        curIndex = 0;
//        curNodePageNum = node.rightNode;
//    }

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

