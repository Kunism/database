#include <iostream>
#include <iomanip>
#include <limits>
#include "ix.h"
static const int page_padding = 0;
std::ostream& operator<<(std::ostream& os, const RID &rid) {
    os << rid.pageNum << ' ' << rid.slotNum;
}

Key::Key()
:keyInt{}, keyFloat{}, keyString{}, rid{}
{

}

Key::Key(const Key &k) {
    this->keyInt = k.keyInt;
    this->keyFloat = k.keyFloat;
    this->keyString = k.keyString;
    this->rid = k.rid;
    this->attrType = k.attrType;
}

Key::Key(const void *key, const RID &rid, AttrType attrType) {
    Key();
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

        char* strBuffer = new char [strLen + 1];
        memcpy(strBuffer, (char*)key + sizeof(uint32_t), strLen);
        strBuffer[strLen] = '\0';
        keyString = std::string(strBuffer, strLen);
        delete[] strBuffer;

        memcpy(&this->rid, &rid, sizeof(RID));
    }
}

Key::Key(char *data, AttrType attrType) {
    Key();
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
        char* strBuffer = new char [actualKeyLen + 1];
        memcpy(strBuffer, data + sizeof(uint32_t), actualKeyLen);
        strBuffer[actualKeyLen] = '\0';
        keyString = std::string(strBuffer, actualKeyLen);

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
        if(keyString == k.keyString) {
            if(this->rid.pageNum == k.rid.pageNum) {
                res = this->rid.slotNum < k.rid.slotNum;
            }
            else res = this->rid.pageNum < k.rid.pageNum;
        }
        else res =  keyString < k.keyString;
        // if(this->keyString.length() == k.keyString.length()) {
        //     res = !strcmp(this->keyString.c_str(), k.keyString.c_str());
        // }
        // else res = this->keyString.length() < k.keyString.length();
    }

    return res;
}

bool Key::operator==(const Key &k) const {
    bool res = false;
    if(k.attrType == TypeInt) {
        res = (keyInt == k.keyInt) && (rid.pageNum == k.rid.pageNum) && (rid.slotNum == k.rid.slotNum);
    }
    else if(k.attrType == TypeReal) {
        res = (keyFloat == k.keyFloat) && (rid.pageNum == k.rid.pageNum) && (rid.slotNum == k.rid.slotNum);
    }
    else if(k.attrType == TypeVarChar) {
        res = (keyString == k.keyString) && (rid.pageNum == k.rid.pageNum) && (rid.slotNum == k.rid.slotNum);
    }
    return res;
}

void Key::operator=(const Key &k) {
    this->keyInt = k.keyInt;
    this->keyFloat = k.keyFloat;
    this->keyString = k.keyString;
    this->rid = k.rid;
    this->attrType = k.attrType;
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

void Key::toData(void *_key) const {
    if(attrType == TypeInt) {
        memcpy(_key, &keyInt, sizeof(int));
    }
    else if(attrType == TypeReal) {
        memcpy(_key, &keyFloat, sizeof(float));
    }
    else if(attrType == TypeVarChar) {
        uint32_t strLen = keyString.length();
        memcpy(_key, &strLen, sizeof(uint32_t));
        memcpy((char*)_key + sizeof(uint32_t), keyString.c_str(), strLen);
    }
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

    leftNode = -1;
    rightNode = -1;
    leftNode = -1;

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

RC BTreeNode::readNode(IXFileHandle &ixFileHandle, uint32_t _pageNum) {

    RC rc = 0;
    
    rc = ixFileHandle.fileHandle.readBTreePage(_pageNum, page);
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

    memcpy(&leftNode, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&rightNode, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    //  read keyNum
    uint32_t keyNum = 0;
    memcpy(&keyNum, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);


    //  read key to vector

    keys.clear();
    for(int i = 0; i < keyNum; i++) {
        //  keyDataLength = keyLen + ridLen
        uint32_t keyDataLength = 0;
        memcpy(&keyDataLength, page + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        char* keyData = new char [keyDataLength];
        memcpy(keyData, page + offset, keyDataLength);
        offset += keyDataLength;

        // std::cerr << "Before push" << std::endl;
        keys.push_back(Key(keyData, attrType));

        // std::cerr << "Key Int = " << keys[0].keyInt << " \tRid = {" << keys[0].rid.pageNum << ", " << keys[0].rid.slotNum << "}" << std::endl;

        delete[] keyData;
    }

    uint32_t childrenNum;
    memcpy(&childrenNum, page + PAGE_SIZE - sizeof(uint32_t), sizeof(uint32_t));
    // std::cerr << "READ NODE CHILDREN SIZE: " << childrenNum << std::endl;

    //  read child to vector
    children.clear();
    for(int i = 0; i < childrenNum; i++) {
        uint32_t child = 0;
        memcpy(&child, page + PAGE_SIZE - sizeof(uint32_t) * (i+2), sizeof(uint32_t));
        children.push_back(child);
    }

    return rc;
}

RC BTreeNode::writeNode(IXFileHandle &ixFileHandle) {

//    std::cerr << "BTreeNode::WriteNode getFreeSpace = " << getFreeSpace() << std::endl;
//    std::cerr << "IsLeafNode = " << isLeafNode << std::endl;
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

    memcpy(page + offset, &leftNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &rightNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    uint32_t keyNum = keys.size();
    // keyNum = 0x87;
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

    // write children size
    uint32_t childrenNum = children.size();
    // std::cerr << "WRITE NODE CHILDREN SIZE: " << childrenNum << std::endl;
    memcpy(page + PAGE_SIZE - sizeof(uint32_t), &childrenNum, sizeof(uint32_t));

    //  write children vector
    for(int i = 0; i < children.size(); i++) {
        memcpy(page + PAGE_SIZE - sizeof(uint32_t) * (i + 2), &children[i], sizeof(uint32_t));
    }

    return ixFileHandle.fileHandle.writeBTreePage(pageNum, page);
}

RC BTreeNode::updateMetaToDisk(IXFileHandle &ixFileHandle, bool isLeafNode, bool isDeleted, uint32_t leftNode, uint32_t rightNode) {
    //  update meta variables
    this->pageNum = pageNum;
    this->isLeafNode = isLeafNode;
    this->isDeleted = isDeleted;
    this->leftNode = leftNode;
    this->rightNode = rightNode;

    return writeNode(ixFileHandle);
}

RC BTreeNode::searchKey(const Key &key) {
    //     1   2   4   6   8   9
    //  [0] [1] [2] [3] [4] [5] [6]!!!
    //  if key = 4, return 3
    //  if key = 5, return 3
    //  if key = 10, return 6

    for(int i = 0; i < keys.size(); i++) {
        if(key < keys[i]) {
            return i;
        }
    }

    return keys.size();
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
    if(index <= children.size()) {
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
    //////////////////////////////////////TOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOO////////////////////////////////////////////////
    //////////////////////////////////////TOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOO////////////////////////////////////////////////
    //////////////////////////////////////TOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOO////////////////////////////////////////////////
    //////////////////////////////////////TOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOO////////////////////////////////////////////////
    return int32_t(PAGE_SIZE) - int32_t(NODE_META_SIZE + keysSize + childrenSize + page_padding);
    //////////////////////////////////////TOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOO////////////////////////////////////////////////
    //////////////////////////////////////TOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOO////////////////////////////////////////////////
    //////////////////////////////////////TOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOO////////////////////////////////////////////////
}


BTree::BTree() {
    rootPageNum = -1;
    totalPageNum = 0;
}

RC BTree::insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const Key &key) {



    // std::cerr << "InsertEntry: " << key.keyString << std::endl;


    // std::cerr << "BTree:Insert: " << key << std::endl;
    if(rootPageNum == -1) {  // no root
        BTreeNode root;
        uint32_t newRootPageNum = totalPageNum;
        createNode(ixFileHandle, root, newRootPageNum, true, false
                , attribute.type, -1, -1);
        this->updateHiddenPageToDisk(ixFileHandle, newRootPageNum, totalPageNum + 1, attribute.type);
        
        // can use general insert instead.

        root.insertToLeaf(key);
        root.updateMetaToDisk(ixFileHandle, root.isLeafNode, root.isDeleted, root.leftNode, root.rightNode);
        
    }
    else {
        // BTreeNode node;
        // node.readNode(ixFileHandle, rootPageNum);
        // std::cerr << "BTree: has Root: "  << rootPageNum << std::endl; 
        bool split = false;
        uint32_t upPageNum;
        std::vector<std::pair<Key, uint32_t>> pushEntities;
        //  allocate space for copykey, if split is needed

        recInsert(ixFileHandle, rootPageNum, key, split, pushEntities);
        if(split)
        {
            BTreeNode newRoot;
            uint32_t newRootPageNum = totalPageNum;
            createNode(ixFileHandle, newRoot, newRootPageNum, false, false, this->attrType, -1, -1);

            for(auto i : pushEntities)
            {
                newRoot.insertKey(i.first, newRoot.keys.size());
            }

            newRoot.insertChild(rootPageNum, newRoot.children.size());
            for(auto i : pushEntities)
            {
                newRoot.insertChild(i.second, newRoot.children.size());
            }
            pushEntities.clear();
            newRoot.writeNode(ixFileHandle);

            BTreeNode left;
            left.readNode(ixFileHandle, rootPageNum);
            BTreeNode right;
            right.readNode(ixFileHandle, pushEntities[0].second);

            // std::cerr << left.keys.size() << ' ' << left.children.size() << std::endl;
            // std::cerr << right.keys.size() << ' ' << right.children.size() << std::endl;

            updateHiddenPageToDisk(ixFileHandle, newRootPageNum, totalPageNum+1, attrType);
        }
    }
    return 0;
}

RC BTree::deleteEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const Key &key) {
    if (this->rootPageNum == -1) {
        // std::cerr << "No Root" <<std::endl;
        return -1;
    }
    else {
        bool isUpdated = false;
        bool isDeleted = false;
        Key updateKey;
        return recDelete(ixFileHandle, rootPageNum, key, isUpdated, isDeleted, updateKey);

        // root need no update
        // root may need to be deleted.
    }
    return 0;
}


RC BTree::recDelete(IXFileHandle &ixFileHandle, uint32_t pageNum, const Key &tar, bool &isUpdated, bool &isDeleted, Key &updateKey) {
    BTreeNode node;
    node.readNode(ixFileHandle, pageNum);

    // std::cerr << "DEL KEY: " << tar << std::endl;
    if(node.isLeafNode) {
        int deleteIndex = -1;

        for(int i = 0 ; i < node.keys.size() ; i++) {
            // std::cerr << node.keys[i] << ' ';
            // std::cerr <<(node.keys[i] == tar )<<  std::endl;
            if(node.keys[i] == tar) {
                deleteIndex = i;
                break;
            }
        }
        // std::cerr <<"INDEX: " <<  deleteIndex <<std::endl;
        if(deleteIndex != -1) {
            node.deleteKey(deleteIndex);
            node.writeNode(ixFileHandle);
            return 0;
        }
        else {
            return -1;
        }
    }
    else {
        int childIndex = node.searchKey(tar);
        uint32_t childPageNum = node.children[childIndex];
        return recDelete(ixFileHandle, childPageNum, tar, isUpdated, isDeleted, updateKey);
    }
}

RC BTree::createNode(IXFileHandle &ixFileHandle, BTreeNode &node, uint32_t pageNum, bool isLeafNode, bool isDeleted
        , AttrType attrType, uint32_t leftNode, uint32_t rightNode) {
    node.pageNum = pageNum;
    node.isLeafNode = isLeafNode;
    node.isDeleted = isDeleted;
    node.attrType = attrType;
    node.leftNode = leftNode;
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

    memcpy(node.page + offset, &leftNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(node.page + offset, &rightNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if(offset + sizeof(uint32_t) != NODE_OFFSET) {
        // std::cerr << "Memcpy Error in BTree::createNode" << std::endl;
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
        if(node.getFreeSpace() > key.size()) {
            node.insertToLeaf(key);

            node.writeNode(ixFileHandle);
            hasSplit = false;
            return 0;
        }
        else {

            BTreeNode newNode;
            uint32_t newNodePageNum = totalPageNum;
            uint32_t oldRightNodePageNum = newNodePageNum;
            createNode(ixFileHandle, newNode, newNodePageNum, true, false, this->attrType, node.pageNum, node.rightNode);
            updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
            node.updateMetaToDisk(ixFileHandle, node.isLeafNode, node.isDeleted, node.leftNode, newNodePageNum);



            uint32_t insertPos = node.searchKey(key);
            node.insertKey(key, insertPos);

            // split index
            int startIndex = 0;
            int totalSize = NODE_META_SIZE;
            for(;startIndex < node.keys.size() && totalSize < (PAGE_SIZE-page_padding) / 2; startIndex++)
            {
                totalSize += node.keys[startIndex].size();
            }



            // prepare for upKey;
            pushEntries.push_back({node.keys[startIndex], newNodePageNum});

            for(int i = startIndex ; i < node.keys.size() ; i++)
            {
                if( newNode.getFreeSpace() > node.keys[i].size()) {
                    newNode.insertToLeaf(node.keys[i]);
                }
                else {
                    newNode.writeNode(ixFileHandle);
                    newNodePageNum = totalPageNum;
                    uint32_t oldRightNode = newNode.rightNode;
                    uint32_t oldLeftNode = newNode.pageNum;
                    newNode.updateMetaToDisk(ixFileHandle, newNode.isLeafNode, newNode.isDeleted, newNode.leftNode, newNodePageNum);
                    createNode(ixFileHandle, newNode, newNodePageNum, true, false, this->attrType, oldLeftNode, oldRightNode);
                    updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
                    // prepare for upKey;
                    pushEntries.push_back({node.keys[i], newNodePageNum});
                    newNode.insertToLeaf(node.keys[i]);
                }
            }

            newNode.writeNode(ixFileHandle);
            // delete overflow keys in vector   

            // std::cerr << "recInsert:START INDEX: " << startIndex << " " << node.keys.size() << std::endl;
            while(node.keys.size() > startIndex)
            {
                node.keys.pop_back();
            }

            node.writeNode(ixFileHandle);

            // IndexManager &indexManager = IndexManager::instance();
            // indexManager.printBtree(ixFileHandle, {"aaa",TypeInt,4});

            hasSplit = true;
            return 0;
        }
    }
    else {
        // std::cerr << "BTree:recInsert: NON-LEAF" <<std::endl;
        int subTreePos = node.searchKey(key);
        uint32_t childPageNum = node.getChild(subTreePos);

        recInsert(ixFileHandle, childPageNum, key, hasSplit, pushEntries);
        if( hasSplit ) {
            int totalPushSize = 0;
            for(auto i : pushEntries) {
                totalPushSize += i.first.size();
                totalPushSize += sizeof(uint32_t);
            }

            if (node.getFreeSpace() > totalPushSize) {

                for(int i = 0 ; i < pushEntries.size() ; i++)
                {
                    int searchPos = node.searchKey(pushEntries[i].first);
                    node.insertKey(pushEntries[i].first, searchPos);
                    node.insertChild(pushEntries[i].second, searchPos + 1);
                }
                node.writeNode(ixFileHandle);
                pushEntries.clear();
                hasSplit = false;
                return 0;
            }
            else {
                // std::vector<Key> temp(node.keys.begin(), node.keys.end());
                // find proper place to insert
                for(auto i : pushEntries) {
                    uint32_t insertPos = node.searchKey(key);
                    node.insertKey(i.first, insertPos);
                    node.insertChild(i.second, insertPos+1);
                }
                pushEntries.clear();

                
                // split index
                int pushIndex = 0;
                int totalSize = NODE_META_SIZE;
                for(;pushIndex < node.keys.size() && totalSize < (PAGE_SIZE - page_padding) / 2 ; pushIndex++)
                {
                    totalSize += node.keys[pushIndex].size();
                    totalSize += sizeof(uint32_t);
                }
                // std::cerr << "Push Index: " << pushIndex << " Key: " << node.keys[pushIndex] << std::endl;

                BTreeNode newNode;
                uint32_t newNodePageNum = totalPageNum;
                createNode(ixFileHandle, newNode, newNodePageNum, false, false, this->attrType, -1, -1); // inter has no left & right node
                updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
                pushEntries.push_back({node.keys[pushIndex],newNodePageNum});
                // vector need to delete too
                // node.updateMetaToDisk(ixFileHandle, node.pageNum, node.isLeafNode, node.isDeleted, pushIndex, newNodePageNum);


                // New Node
                for(int i = pushIndex + 1 ; i < node.keys.size() ; i++) {
                    if( newNode.getFreeSpace() > node.keys[i].size() + sizeof(uint32_t)) {
                        // std::cerr << "New Node Insert: " << node.keys[i] << " " << node.children[i] <<std::endl;
                        newNode.insertKey(node.keys[i], newNode.keys.size());
                        newNode.insertChild(node.children[i], newNode.children.size());
                    }
                    else {
                        // std::cerr << "Current new Node is full!!!" <<std::endl;
                        newNode.writeNode(ixFileHandle);
                        newNodePageNum = totalPageNum;
                        createNode(ixFileHandle, newNode, newNodePageNum, false, false, node.attrType, -1, -1);
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

                node.writeNode(ixFileHandle);

                hasSplit = true;
                // std::cerr << "INTER NODE SPLIT: " << std::endl;
                // for(auto i : pushEntries) {
                //     std::cerr << "KEY: " << i.first << ' ' << "CHILD: " << i.second << std::endl;
                // }
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
        // std::cerr << "ReadPage fail in BTree::readBTree." << std::endl;
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
    BTree bTree;
    if(bTree.readBTreeHiddenPage(ixFileHandle) != 0) {
        // std::cerr << "IndexManager: read Hidden page fail" <<std::endl; 
        return -1;
    }
    if( bTree.insertEntry(ixFileHandle, attribute, Key(key, rid, attribute.type))) {
        // std::cerr << "IndexManager: Insert fail" <<std::endl; 
        return -1;
    }
    //rc |= bTree.writeBTree(ixFileHandle);
    return 0;
}

RC IndexManager::deleteEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const void *key, const RID &rid) {
    BTree bTree;
    if( bTree.readBTreeHiddenPage(ixFileHandle) != 0) {
        // std::cerr << "IndexManager: read Hidden page fail" <<std::endl; 
        return -1;
    }

    if( bTree.deleteEntry(ixFileHandle, attribute, Key(key, rid, attribute.type)) != 0) {
        return -1;
    }

    return 0;

}

RC IndexManager::scan(IXFileHandle &ixFileHandle,
                      const Attribute &attribute,
                      const void *lowKey,
                      const void *highKey,
                      bool lowKeyInclusive,
                      bool highKeyInclusive,
                      IX_ScanIterator &ix_ScanIterator) {

    RC rc = 0;
    // std::cerr << "IX:SCAN " << std::endl;
    if( ix_ScanIterator.init(ixFileHandle, attribute, lowKey, highKey, lowKeyInclusive, highKeyInclusive) != 0 ) {
        // std::cerr<< "IX init fail" <<std::endl;
        return -1;
    }


    return rc;
}

void IndexManager::printBtree(IXFileHandle &ixFileHandle, const Attribute &attribute) const {

    BTree bTree;
    bTree.readBTreeHiddenPage(ixFileHandle);

    recursivePrint(ixFileHandle, bTree.rootPageNum, 0);
    std::cout << std::endl;
}

void IndexManager::recursivePrint(IXFileHandle &ixFileHandle, uint32_t pageNum, int depth) const
{
    BTreeNode btNode;
    btNode.readNode(ixFileHandle, pageNum);


    std::cout << std::setw(4*depth) << "" << "{\"keys\": [";
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
        std::cout << std::setw(4*depth) << "" << "\"children\":[" << std::endl;

        for(int i = 0 ; i < btNode.children.size() ; i++) {
            BTreeNode bt;
            bt.readNode(ixFileHandle,btNode.children[i]);
            // std::cerr << "ISLEAF: " << bt.isLeafNode << std::endl; 
            recursivePrint(ixFileHandle, btNode.children[i], depth+1);
            if(i != btNode.children.size()-1)
            {
                std::cout << "," << std::endl;
            }
        }
        std::cout << std::endl;
        std::cout << std::setw(4*depth) << "" << "]";
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
    // need fault constructor!!!! for lowKey and High Key
    curNodePageNum = -1;
    curIndex = -1;
    firstValid = false;

}

IX_ScanIterator::~IX_ScanIterator() {
}

RC IX_ScanIterator::init(IXFileHandle &_ixFileHandle, const Attribute &_attribute, const void *_lowKey,
                         const void *_highKey, bool _lowKeyInclusive, bool _highKeyInclusive) {
    //std::cerr << "IX:INIT " << std::endl;
    if(_ixFileHandle.fileHandle.isOpen() != 0) {
        //std::cerr << "SCAN file open error" <<std::endl;
        return -1;
    }
    
    this->ixFileHandle = &_ixFileHandle;
    this->attribute = _attribute;
    //  TODO: need space?
    this->lowKeyInclusive = _lowKeyInclusive;
    this->highKeyInclusive = _highKeyInclusive;
    RID lowRID;
    RID highRID;

    // construct range [lowRID, highRID)
    if(lowKeyInclusive) {
        lowRID = {0,0};
    }
    else {
        lowRID = {INT_MAX, INT_MAX};
    }

    if(highKeyInclusive) {
        highRID = {INT_MAX, INT_MAX};
    }
    else {
        highRID = {0,0};
    }

    if(_lowKey == NULL) {
        if ( attribute.type == TypeInt) {
            int temp = 0;
            this->lowKey = Key(&temp, {0,0}, attribute.type);
        }
        else if (attribute.type == TypeReal) {
            float temp = 0.0f;
            this->lowKey = Key(&temp, {0,0}, attribute.type);
        }
        else if (attribute.type == TypeVarChar) {
            uint32_t stringSize = 0;
            /////////////////////////////////////////////////////DANGER!!!!/////////////////////////////////
            /////////////////////////////////////////////////////DANGER!!!!/////////////////////////////////
            this->lowKey = Key(&stringSize, {0,0} , attribute.type);
            /////////////////////////////////////////////////////DANGER!!!!/////////////////////////////////
        }
    }
    else {
        this->lowKey = Key(_lowKey, lowRID ,attribute.type);
    }

    if(_highKey == NULL) {
        if ( attribute.type == TypeInt) {
            int temp = std::numeric_limits<int>::max();
            this->highKey = Key(&temp, {INT_MAX,INT_MAX}, attribute.type);
        }
        else if (attribute.type == TypeReal) {
            float temp = std::numeric_limits<float>::max();
            this->highKey = Key(&temp, {INT_MAX,INT_MAX}, attribute.type);
        }
        else if (attribute.type == TypeVarChar) {
            uint32_t stringSize = attribute.length + 1;
            // '~' ascii = 127
            std::string temp(stringSize,'~');
            uint32_t bufferSize = sizeof(uint32_t) + stringSize;
            uint8_t *buffer = new uint8_t [bufferSize];
            memset(buffer, 0, bufferSize);
            memcpy(buffer, &stringSize, sizeof(uint32_t));
            memcpy(buffer+ sizeof(uint32_t), temp.c_str(), temp.size());
            this->highKey = Key(buffer, {INT_MAX,INT_MAX} , attribute.type);
        }
    }
    else {
        this->highKey = Key(_highKey, highRID, attribute.type);
    }

    // std::cerr << "INIT" << std::endl;
    // std::cerr << lowKey  << ' ' << lowKey.rid << std::endl;
    // std::cerr << highKey << ' '<< highKey.rid << std::endl;


    if( bTree.readBTreeHiddenPage(*ixFileHandle) != 0) {
        //std::cerr << "SCAN BTREE FAIL" <<std::endl;
        return -1;
    }
    this->curNodePageNum = bTree.recSearch(*ixFileHandle, lowKey, bTree.rootPageNum);
    BTreeNode node;
    node.readNode(*ixFileHandle, curNodePageNum);

    

    this->curIndex = 0;
    this->curKey = lowKey;

    if(lowKeyInclusive) {
        if(node.keys.size()>0 && !(node.keys[0] < lowKey)) {
            firstValid = true;
            curKey = node.keys[0];
            // std::cerr << "SCAN VAR: lowkey: " << lowKey << "highKey: " << highKey << "lowRID: " << lowRID << "highRID: " << highRID << "curIndex: " << curIndex << "curKey: " << curKey <<std::endl;
            // return 0;
        }
        for(int i = 0 ; i < node.keys.size() ; i++) {
            // take last '<' or first '>='  ?
            // currently take last '<'
            if(node.keys[i] < lowKey)
            {
                this->curIndex = i;
                curKey = node.keys[i];

//                std::cerr << "node key i = " << node.keys[i] << "\tlowKey = " << lowKey << std::endl;
//                std::cerr << "==========================================" << std::endl;
            }
        }

//        std::cerr << "High key = " << highKey << "\tLow key = " << lowKey << std::endl;
//        std::cerr << "High RID = {" << highRID.pageNum << "," << highRID.slotNum << "}" << "\tLow RID = " << "{"<< lowRID.pageNum << "," << lowRID.slotNum << "}" << std::endl;
//        std::cerr << "Current Index = " << curIndex << "\tCurrent Key = " << curKey << std::endl;
//        std::cerr << "==========================================" << std::endl;
    }

    
    // std::cerr << "SCAN VAR: lowkey: " << lowKey << "highKey: " << highKey << "lowRID: " << lowRID << "highRID: " << highRID << "curIndex: " << curIndex << "curKey: " << curKey <<std::endl;
    

  

    return 0;
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key) {
    BTreeNode node;
    node.readNode(*ixFileHandle, curNodePageNum);

    
    //std::cerr << "IX_ScanIterator::getNextEntry" << std::endl;
    if(firstValid) {
        //std::cerr << "FIRST VALID" <<std::endl;
       this->curKey = node.keys[this->curIndex];
       //std::cerr << curKey << " v.s " << highKey << ' ' << (curKey < highKey) << std::endl;
       if(curKey < highKey) {
            curKey.toData(key);
            rid = curKey.rid;
            firstValid = false;
            return 0;
        }
        else {
            return IX_EOF;
        }
   }


    
    // next element 
    while(true) {
        if (this->curNodePageNum == -1) {
            return IX_EOF;
        }
        node.readNode(*ixFileHandle,curNodePageNum);
        int nextIndex = node.searchKey(curKey);
        // std::cerr << "search Read Node: " << nextIndex;
        if( nextIndex != node.keys.size()) {
            this->curKey = node.keys[nextIndex];
            // std::cerr << " record is " << curKey <<std::endl;
            // std::cerr << curKey << " v.s " << highKey << ' ' << (curKey < highKey) << std::endl;
            if(curKey < highKey) {
            // return value
                curKey.toData(key);
                rid = curKey.rid;
                return 0;
            }
            else {
                // std::cerr << "END" <<std::endl;
                return IX_EOF;
            }
        }
        else { 
            // std::cerr << " next page "  <<std::endl;
            this->curNodePageNum = node.rightNode;
            // this->curIndex = -1;
        }
    }

}

RC IX_ScanIterator::close() {
    return 0;
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

