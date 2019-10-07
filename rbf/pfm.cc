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
//        char* buffer = new char [FileHandle::PAGE_OFFSET * PAGE_SIZE];
//        memcpy(buffer, (char*) &hiddenPage, sizeof(hiddenPage));
//        file.write(buffer, FileHandle::PAGE_OFFSET * PAGE_SIZE);
        file.close();
        return 0;
    }
}

RC PagedFileManager::destroyFile(const std::string &fileName) {
    std::fstream file;
    file.open(fileName, std::ios::in | std::ios::binary);
    
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
    if (pageNum <= hiddenPage->var[PAGE_NUM]) {
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
    if (pageNum <= hiddenPage->var[PAGE_NUM]) {
        char* buffer = new char [PAGE_SIZE];
        std::memcpy(buffer,data, PAGE_SIZE);
        file.seekp((PAGE_OFFSET + pageNum) * PAGE_SIZE);
        file.write( buffer, PAGE_SIZE);
        hiddenPage->var[WRITE_PAGE_COUNTER]++;
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
    return 0;
}

unsigned FileHandle::getNumberOfPages() {
    return hiddenPage->var[PAGE_NUM];
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
    readPageCount = hiddenPage->var[READ_PAGE_COUNTER];
    writePageCount = hiddenPage->var[WRITE_PAGE_COUNTER];
    appendPageCount = hiddenPage->var[APPEND_PAGE_COUNTER];
    return 0;
}

RC FileHandle::openFile(const std::string &fileName) {
    file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
    if( !file.is_open()) {
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
    file.close();
    return 0;
}

HiddenPage::HiddenPage() {

    var[READ_PAGE_COUNTER] = 101;
    var[WRITE_PAGE_COUNTER] = 102;
    var[APPEND_PAGE_COUNTER] = 103;
    var[PAGE_NUM] = 0;
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
    var[PAGE_NUM] = ((unsigned*)buffer)[PAGE_SIZE];
}

void HiddenPage::writeHiddenPage(std::fstream& file) {

    char* buffer = new char [PAGE_SIZE];
    memcpy(buffer, (char*)var, sizeof(unsigned) * HIDDEN_PAGE_VAR_NUM);
    file.seekp(0, std::ios::beg);
    file.write(buffer, PAGE_SIZE);

    char* buf = new char[sizeof(unsigned) * 4];
    file.seekg(0, std::ios_base::beg);
    file.read(buf, sizeof(unsigned) * 4);

}

//bool HiddenPage::isFull() {
//    return size > PAGE_SIZE;
//}