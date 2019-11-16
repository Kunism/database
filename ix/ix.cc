#include <iostream>
#include "ix.h"

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

BTreeNode::BTreeNode() {
    attrType = TypeInt;
    isLeafNode = false;
    isDeleted = false;
    pageNum = -1;
    curKeyNum = 0;
    curChildNum = 0;

    rightNode = -1;

    keys.clear();
    children.clear();

    page = new char [PAGE_SIZE];
    memset(page, 0, PAGE_SIZE);
}

BTreeNode::~BTreeNode() {
    delete[] page;
}

RC BTreeNode::insertToLeaf(const Key &key, const RID &rid) {

    // TODO: need to check leaf is full???

    //  in order to insert, need to move elements after index backward one unit
    uint32_t index = searchKey(key);
    insertKey(key, index);
    insertRID(rid, index);
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

    memcpy(&attrLength, page + offset, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(&curKeyNum, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&curChildNum, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&rightNode, page + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    //  read key to vector
    for(int i = 0; i < curKeyNum; i++) {
        //  keyDataLength = keyLen + ridLen
        uint32_t keyDataLength = 0;
        memcpy(&keyDataLength, page + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        char* keyData = new char [keyDataLength];
        memcpy(keyData, page + offset, keyDataLength);
        offset += keyDataLength;

        //Key key(keyData, attrType);
        keys.push_back(Key(keyData, attrType));

        std::cerr << "Key Int = " << keys[0].keyInt << " \tRid = {" << keys[0].rid.pageNum << ", " << keys[0].rid.slotNum << "}" << std::endl;

        delete[] keyData;
    }

    //  read child to vector
    for(int i = 0; i < curChildNum; i++) {
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

    memcpy(page + offset, &attrLength, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(page + offset, &curKeyNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &curChildNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(page + offset, &rightNode, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    //  write key vector
    for(int i = 0; i < curKeyNum; i++) {

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
    for(int i = 0; i < curChildNum; i++) {
        memcpy(page + PAGE_SIZE - sizeof(uint32_t) * (i + 1), &children[i], sizeof(uint32_t));
    }

    return ixFileHandle.fileHandle.writeBTreePage(pageNum, page);
}

RC BTreeNode::updateMetaToDisk(IXFileHandle &ixFileHandle, uint32_t pageNum, bool isLeafNode, bool isDeleted
        , uint32_t curKeyNum, uint32_t rightNode) {
    //  update meta variables
    this->pageNum = pageNum;
    this->isLeafNode = isLeafNode;
    this->isDeleted = isDeleted;
    this->curKeyNum = curKeyNum;
    this->rightNode = rightNode;

    return writeNode(ixFileHandle);
}

RC BTreeNode::searchKey(const Key &key) {
    //     1   2   4   6   8   9
    //  [0] [1] [2] [3] [4] [5]
    //  if key = 4, return 3
    //  if key = 5, return 3

    int result = 0;

    for(int i = 0; i < curKeyNum; i++) {

//        uint32_t valLen = attrLength;
//        if(attrType == TypeVarChar) {
//            memcpy(&valLen, page + getKeysBegin() + attrLength * i, sizeof(uint32_t));
//            valLen += sizeof(uint32_t);
//        }
//
//        char* val = new char [valLen];
//        memset(val, 0, valLen);
//        memcpy(val, page + getKeysBegin() + attrLength * i, valLen);

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

RC BTreeNode::printKey() {
    for(int i = 0; i < curKeyNum; i++) {
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
    for(int i = 0; i < curKeyNum; i++) {
        std::cerr << "RID:  " << keys[i].rid.pageNum << "\t" << keys[i].rid.slotNum << std::endl;
    }
}

RC BTreeNode::getChild(uint32_t index) {
    if(index < curChildNum) {
        return children[index];
    }
    else {
        return -1;
    }
}

RC BTreeNode::insertChild(uint32_t childPageNum, uint32_t index) {
    if(index < curChildNum) {
        children.insert(children.begin() + index, childPageNum);
        return 0;
    }
    else {
        return -1;
    }
}

uint32_t BTreeNode::getKeysBegin() {
    return NODE_OFFSET;
}

//  first child addr.
uint32_t BTreeNode::getChildrenBegin() {
    return PAGE_SIZE - sizeof(uint32_t);
}

int32_t BTreeNode::getFreeSpace() {
    int32_t freeSpace = 0;
    if(attrType == TypeInt) {
        freeSpace = (int32_t)PAGE_SIZE - (int32_t)(NODE_OFFSET + (sizeof(uint32_t) + sizeof(int) + sizeof(RID)) * curKeyNum + sizeof(uint32_t) * curChildNum);
    }
    else if(attrType == TypeReal) {
        freeSpace = (int32_t)PAGE_SIZE - (int32_t)(NODE_OFFSET + (sizeof(uint32_t) + sizeof(float) + sizeof(RID)) * curKeyNum + sizeof(uint32_t) * curChildNum);
    }
    else if(attrType == TypeVarChar) {
        //  all key size total
        uint32_t keySize = 0;
        for(int i = 0; i < curKeyNum; i++) {
            keySize += keys[i].size();
        }

        freeSpace = (int32_t)PAGE_SIZE - (int32_t)(NODE_OFFSET + keySize + sizeof(uint32_t) * curChildNum);
    }

    return freeSpace;
}


BTree::BTree() {
    rootPageNum = -1;
    totalPageNum = 0;
    attrLength = 0;
}

RC BTree::insertEntry(IXFileHandle &ixFileHandle, const Attribute &attribute, const Key &key, const RID &rid) {
    if(rootPageNum == -1) {  // no root
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
        uint32_t newRootPageNum = totalPageNum;
        createNode(ixFileHandle, root, newRootPageNum, true, false
                , attribute.type, attrLength, order, -1);
        this->updateHiddenPageToDisk(ixFileHandle, newRootPageNum, totalPageNum + 1, attribute.type);
        
        // can use general insert instead.  
        root.insertToLeaf(key, rid);
        root.updateMetaToDisk(ixFileHandle, newRootPageNum, root.isLeafNode, root.isDeleted
                , root.curKeyNum + 1, root.rightNode);
        
    }
    else {
        // BTreeNode node;
        // node.readNode(ixFileHandle, rootPageNum);
        bool split = false;
        uint32_t upPageNum;
        
        //  allocate space for copykey, if split is needed

        recInsert(ixFileHandle, rootPageNum, key, rid,  split, copyKey, upPageNum);
        if(split)
        {
            BTreeNode newRoot;
            uint32_t newRootPageNum = totalPageNum;
            createNode(ixFileHandle, newRoot, newRootPageNum, false, false, this->attrType, this->attrLength, this->order, -1);
            updateHiddenPageToDisk(ixFileHandle, newRootPageNum, totalPageNum+1, attrType);

            newRoot.insertKey(copyKey, 0);
            newRoot.insertChild(rootPageNum, 0);
            newRoot.insertChild(upPageNum, 1);
            newRoot.updateMetaToDisk(ixFileHandle, newRootPageNum, newRoot.isLeafNode, newRoot.isDeleted, newRoot.curKeyNum + 1, newRoot.rightNode);
        }
    }
    return 0;
}

RC BTree::createNode(IXFileHandle &ixFileHandle, BTreeNode &node, uint32_t pageNum, bool isLeafNode, bool isDeleted
        , AttrType attrType, AttrLength attrLength, uint32_t order, uint32_t rightNode) {
    node.pageNum = pageNum;
    node.isLeafNode = isLeafNode;
    node.isDeleted = isDeleted;
    node.attrType = attrType;
    node.attrLength = attrLength;
    node.curKeyNum = 0;
    node.curChildNum = 0;
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

    //  attrLength = 4 if int or float. if type is varchar, attrLength = 4 + charLength
    memcpy(node.page + offset, &attrLength, sizeof(AttrLength));
    offset += sizeof(AttrLength);

    memcpy(node.page + offset, &node.curKeyNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(node.page + offset, &node.curChildNum, sizeof(uint32_t));
    offset += sizeof(uint32_t);

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

    // RC rc = 0;

    // char* hiddenPage = new char [PAGE_SIZE];
    // memset(hiddenPage, 0, PAGE_SIZE);
    // rc |= ixFileHandle.fileHandle.readBTreeHiddenPage(hiddenPage);

    // uint32_t offset = sizeof(uint32_t) * NON_BTREE_VAR_NUM;
    // memset(hiddenPage + offset, 0, PAGE_SIZE - offset);

    // memcpy(hiddenPage + offset, &rootPageNum, sizeof(uint32_t));
    // offset += sizeof(uint32_t);

    // memcpy(hiddenPage + offset, &totalPageNum, sizeof(uint32_t));
    // offset += sizeof(uint32_t);

    // memcpy(hiddenPage + offset, &attrType, sizeof(AttrType));
    // offset += sizeof(AttrType);

    // memcpy(hiddenPage + offset, &attrLength, sizeof(AttrLength));
    // offset += sizeof(AttrLength);

    // memcpy(hiddenPage + offset, &order, sizeof(uint32_t));
    // offset += sizeof(uint32_t);

    // rc |= ixFileHandle.fileHandle.writeBTreeHiddenPage(hiddenPage);
    // // ixFileHandle.fileHandle.

    // delete[] hiddenPage;
    return rc;
}

RC BTree::recInsert(IXFileHandle &ixFileHandle, const uint32_t nodePageNum, const Key &key,   // insert element
                    bool &hasSplit, std::vector<std::pair<Key, uint32_t>> &pushEntries) {    // return element

    BTreeNode node;
    node.readNode(ixFileHandle, nodePageNum);
    if(node.isLeafNode) {  
        // leaf only insert key
        if(node.getFreeSpace() > key.size()) {
            node.insertToLeaf(key, key.rid);
            node.updateMetaToDisk(ixFileHandle, node.pageNum, node.isLeafNode, node.isDeleted, node.curKeyNum + 1, node.rightNode);
            hasSplit = false;
            return 0;

        }
        else {
            std::vector<Key> temp(node.keys.begin(), node.keys.end());
            // find proper place to insert
            
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
            createNode(ixFileHandle, newNode, newNodePageNum, true, false, this->attrType, this->attrLength,  this->order, node.rightNode);
            updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
            node.updateMetaToDisk(ixFileHandle, node.pageNum, node.isLeafNode, node.isDeleted, startIndex, newNodePageNum);
            // prepare for upKey;
            pushEntries.push_back({temp[startIndex], newNodePageNum});

            for(int i = startIndex ; i < temp.size() ; i++)
            {
                if( newNode.getFreeSpace() > temp[i].size()) {
                    newNode.insertToLeaf(key, key.rid);
                    newNode.updateMetaToDisk(ixFileHandle, newNode.pageNum, newNode.isLeafNode, newNode.isDeleted, newNode.curKeyNum + 1, newNode.rightNode);
                }
                else {
                    newNodePageNum = totalPageNum;
                    uint32_t oldRightNode = newNode.rightNode;
                    newNode.updateMetaToDisk(ixFileHandle, newNode.pageNum, newNode.isLeafNode, newNode.isDeleted, newNode.curKeyNum, newNodePageNum);
                    createNode(ixFileHandle, newNode, newNodePageNum, true, false, this->attrType, this->attrLength,  this->order, oldRightNode);
                    updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
                    // prepare for upKey;
                    pushEntries.push_back({temp[startIndex], newNodePageNum});
                }
            }
            
            //////
            // delete overflow keys in vector   

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
                    node.updateMetaToDisk(ixFileHandle, node.pageNum, node.isLeafNode, node.isDeleted, node.curKeyNum+1, node.rightNode);
                }    
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
                for(;pushIndex < node..size() && totalSize < PAGE_SIZE / 2 ; pushIndex++)
                {
                    totalSize += temp[pushIndex].size();
                    totalSize += sizeof(uint32_t);
                }


                BTreeNode newNode;
                uint32_t newNodePageNum = totalPageNum;
                createNode(ixFileHandle, newNode, newNodePageNum, false, false, this->attrType, this->attrLength, this->order, -1);
                updateHiddenPageToDisk(ixFileHandle, rootPageNum, totalPageNum+1, attrType);
                // vector need to delete too
                node.updateMetaToDisk(ixFileHandle, node.pageNum, node.isLeafNode, node.isDeleted, pushIndex, newNodePageNum);

              
               
               

                // New Node
                for(int i = pushIndex + 1 ; i < temp.size() ; i++) {

                    newNode.insertKey(temp[i], newNode.curKeyNum);
                    newNode.insertChild(childPos, newNode.curKeyNum);
                    newNode.updateMetaToDisk(ixFileHandle, newNode.pageNum, newNode.isLeafNode, newNode.isDeleted, newNode.curKeyNum+1, newNode.rightNode);
                }
                // insert last child
                childPos = node.getChild(node.curKeyNum);
                newNode.insertChild(childPos, newNode.curKeyNum);
                newNode.updateMetaToDisk(ixFileHandle, newNode.pageNum, newNode.isLeafNode, newNode.isDeleted, newNode.curKeyNum+1, newNode.rightNode);

               

                hasSplit = true;
                upPageNum = newNodePageNum;
                delete [] keyBuffer;

                return 0;
            }
        }        
    }
}

RC BTree::recSearch(IXFileHandle &ixFileHandle, const char *key, uint32_t pageNum) {
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
    attrLength = data[ATTRLENGTH];
    order = data[ORDER];
    
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
    rc |= bTree.insertEntry(ixFileHandle, attribute, (char*)key, rid);
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
    BTreeNode root;
    std::cerr << "totalPageNum = " << bTree.totalPageNum << std::endl;
    std::cerr << "rootPageNum = " << bTree.rootPageNum << std::endl;
    root.readNode(ixFileHandle, bTree.rootPageNum);
    std::cerr << "curKeyNum = " << root.curKeyNum << std::endl;

    root.printKey();
    root.printRID();
}

IX_ScanIterator::IX_ScanIterator() {


    curNodePageNum = -1;
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
    else if(curNodePageNum == 0) {

    }
    else {

    }

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

