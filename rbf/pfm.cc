#include "pfm.h"
#include <iostream>
PagedFileManager *PagedFileManager::_pf_manager = nullptr;

PagedFileManager &PagedFileManager::instance() {
    static PagedFileManager _pf_manager = PagedFileManager();
    return _pf_manager;
}

PagedFileManager::PagedFileManager() = default;

PagedFileManager::~PagedFileManager() { delete _pf_manager; }

PagedFileManager::PagedFileManager(const PagedFileManager &) = default;

PagedFileManager &PagedFileManager::operator=(const PagedFileManager &) = default;

RC PagedFileManager::createFile(const std::string &fileName) {
    std::fstream file;
    file.open(fileName, std::ios::in | std::ios::binary);
    if( file.is_open()) {
        return -1;
    }
    else {
        file.clear();
        file.open(fileName, std::ios::out | std::ios::binary);
        HiddenPage hiddenPage;
        hiddenPage.writeHiddenPage(file);
        file.close();
        return 0;
    }
}

RC PagedFileManager::destroyFile(const std::string &fileName) {
    std::fstream file;
    file.open(fileName, std::ios::in | std::ios::binary);
    
    // TODO check the statment valid
    if( !file.is_open() || remove(fileName.c_str()) != 0) {
        return -1;
    }
    else{
        return 0;
    }
}

RC PagedFileManager::openFile(const std::string &fileName, FileHandle &fileHandle) {

    return fileHandle.openFile(fileName);
}

RC PagedFileManager::closeFile(FileHandle &fileHandle) {
    return fileHandle.closeFile();
}

FileHandle::FileHandle() {
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
}

FileHandle::~FileHandle() = default;

RC FileHandle::readPage(PageNum pageNum, void *data) {
    if (pageNum < hiddenPage->var[PAGE_NUM]) {
        file.seekg((PAGE_OFFSET + pageNum) * PAGE_SIZE);
        file.read( reinterpret_cast<char*>(data) , PAGE_SIZE);
        hiddenPage->var[READ_PAGE_COUNTER]++;
        return 0;
    }
    else {
        return -1;
    }
}

RC FileHandle::writePage(PageNum pageNum, const void *data) {
    if (pageNum < hiddenPage->var[PAGE_NUM]) {
        char* buffer = new char [PAGE_SIZE];
        std::memcpy(buffer,data, PAGE_SIZE);
        file.seekp((PAGE_OFFSET + pageNum) * PAGE_SIZE);
        file.write( buffer, PAGE_SIZE);
        hiddenPage->var[WRITE_PAGE_COUNTER]++;
        // TOOOOOOOOOOOOOOOOOODOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
        // write hidden page?
        delete[] buffer;
        return 0;
    }
    else {
        return -1;
    }
}

RC FileHandle::appendPage(const void *data) {
    char* buffer = new char [PAGE_SIZE];
    std::memcpy(buffer, data, PAGE_SIZE);
    file.seekp(0, std::ios::end);
    file.write(buffer, PAGE_SIZE);
    hiddenPage->var[APPEND_PAGE_COUNTER]++;
    hiddenPage->var[PAGE_NUM]++;
    // CHECK
    hiddenPage->writeHiddenPage(file);
    delete[] buffer;
    return 0;
}

unsigned FileHandle::getNumberOfPages() {
    return hiddenPage->var[PAGE_NUM];
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
    hiddenPage->readHiddenPage(file);
    readPageCount = hiddenPage->var[READ_PAGE_COUNTER];
    writePageCount = hiddenPage->var[WRITE_PAGE_COUNTER];
    appendPageCount = hiddenPage->var[APPEND_PAGE_COUNTER];
    return 0;
}

// it returns "unsigned var[HIDDEN_PAGE_VAR_NUM]"
RC FileHandle::readBTreeHiddenPage(void *data) {
    
    hiddenPage->readHiddenPage(file);

    memcpy(data, hiddenPage->var, sizeof(int) * HIDDEN_PAGE_VAR_NUM);
    return 0;
}


// it takes "unsigned var[HIDDEN_PAGE_VAR_NUM]"
RC FileHandle::writeBTreeHiddenPage(void *data) {
    
    memcpy(hiddenPage->var, data, sizeof(int) * HIDDEN_PAGE_VAR_NUM);

    hiddenPage->writeHiddenPage(file);
    return 0;
}

// RC FileHandle::appendBTreePage(void *data) {
//     // char* fakeHiddenPage = new char [PAGE_SIZE];
    
//     readBTreeHiddenPage(fakeHiddenPage);
//     file.seekp(0, std::ios::end);
//     file.write(reinterpret_cast<char*>(data), PAGE_SIZE);
//     // hiddenPage->var[]++
//     // hidde
//     // CHECK
//     hiddenPage->writeBTreeHiddenPage();
//     delete[] buffer;
//     return 0;
// }


RC FileHandle::readBTreePage(PageNum pageNum, void *data) {
    if (pageNum < hiddenPage->var[TOTAL_PAGE_NUM]) {
        file.seekg((PAGE_OFFSET + pageNum) * PAGE_SIZE);
        file.read( reinterpret_cast<char*>(data) , PAGE_SIZE);
        hiddenPage->var[READ_PAGE_COUNTER]++;
        hiddenPage->writeHiddenPage(file);
        return 0;
    }
    else {
        return -1;
    }
}

RC FileHandle::writeBTreePage(PageNum pageNum, const void *data) {
    if (pageNum < hiddenPage->var[TOTAL_PAGE_NUM]) {
        file.seekp((PAGE_OFFSET + pageNum) * PAGE_SIZE);
        file.write( reinterpret_cast<const char*>(data), PAGE_SIZE);
        hiddenPage->var[WRITE_PAGE_COUNTER]++;
        hiddenPage->writeHiddenPage(file);
        return 0;
    }
    else {
        return -1;
    }
}

RC FileHandle::createNodePage(void* data) {
    // read the newest data
    hiddenPage->readHiddenPage(file);

    hiddenPage->var[APPEND_PAGE_COUNTER]++;
    hiddenPage->var[TOTAL_PAGE_NUM]++;
    file.seekp(0, std::ios::end);
    file.write(reinterpret_cast<const char*>(data), PAGE_SIZE);
    hiddenPage->writeHiddenPage(file);
    return 0;
}

unsigned FileHandle::getNumberOfNodes() {
    return hiddenPage->var[TOTAL_PAGE_NUM];
}
//DELETE TODO
RC FileHandle::openFile(const std::string &fileName) {
    if(file.is_open()) {
        return -1;
    }
    file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
    if( !file.is_open()) {
        // std::cerr << "CANNOT OPEN FROM THE BOTTOM" <<std::endl;
        return -1;
    }
    else {
        hiddenPage = new HiddenPage();
        hiddenPage->readHiddenPage(file);
        return 0;
    }
}

RC FileHandle::closeFile() {
    hiddenPage->writeHiddenPage(file);
    delete(hiddenPage);
    file.close();
    return 0;
}

HiddenPage::HiddenPage() {

    var[READ_PAGE_COUNTER] = 0;
    var[WRITE_PAGE_COUNTER] = 0;
    var[APPEND_PAGE_COUNTER] = 0;
    var[PAGE_NUM] = 0;

    var[ROOT_PAGE_NUM] = -1;
    var[TOTAL_PAGE_NUM] = 0;
    var[ATTRTYPE] = 0;
    var[ATTRLENGTH] = 0;
    var[ORDER] = 0;
}

HiddenPage::~HiddenPage() {

}

void HiddenPage::readHiddenPage(std::fstream& file) {
    char* buffer = new char [sizeof(unsigned) * HIDDEN_PAGE_VAR_NUM];
    file.seekg(0, std::ios::beg);
    file.read(buffer, sizeof(unsigned) * HIDDEN_PAGE_VAR_NUM);
    var[READ_PAGE_COUNTER] = ((unsigned*)buffer)[READ_PAGE_COUNTER];
    var[WRITE_PAGE_COUNTER] = ((unsigned*)buffer)[WRITE_PAGE_COUNTER];
    var[APPEND_PAGE_COUNTER] = ((unsigned*)buffer)[APPEND_PAGE_COUNTER];
    var[PAGE_NUM] = ((unsigned*)buffer)[PAGE_NUM];

    var[ROOT_PAGE_NUM] = ((unsigned*)buffer)[ROOT_PAGE_NUM];
    var[TOTAL_PAGE_NUM] = ((unsigned*)buffer)[TOTAL_PAGE_NUM];
    var[ATTRTYPE] = ((unsigned*)buffer)[ATTRTYPE];
    var[ORDER] = ((unsigned*)buffer)[ORDER];

    delete[] buffer;
}

void HiddenPage::writeHiddenPage(std::fstream& file) {

    char* buffer = new char [PAGE_SIZE];
    memset(buffer, 0, PAGE_SIZE);
    memcpy(buffer, var, sizeof(unsigned) * HIDDEN_PAGE_VAR_NUM);
    file.seekp(0, std::ios::beg);
    file.write(buffer, PAGE_SIZE);

    delete[] buffer;
}

