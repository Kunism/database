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
        char* dummyPage = new char [ FileHandle::pageOffset * PAGE_SIZE];
        file.write(dummyPage,FileHandle::pageOffset * PAGE_SIZE);
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
    fileHandle.closeFile();
}

FileHandle::FileHandle() {
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
}

FileHandle::~FileHandle() = default;

RC FileHandle::readPage(PageNum pageNum, void *data) {
    if (!checkPageNum(pageNum)) {
        return -1;
    }
    else {
        file.seekg((pageOffset+pageNum)*PAGE_SIZE);
        file.read( reinterpret_cast<char*>(data) , PAGE_SIZE);
        readPageCounter++;
        return 0;
    }
}

RC FileHandle::writePage(PageNum pageNum, const void *data) {
    if (!checkPageNum(pageNum)) {
        return -1;
    }
    else {
        char* buffer = new char [PAGE_SIZE];
        std::memcpy(buffer,data, PAGE_SIZE);
        file.seekp((pageOffset+pageNum)*PAGE_SIZE);
        file.write( buffer, PAGE_SIZE);
        writePageCounter++;
        return 0;
    }
}

RC FileHandle::appendPage(const void *data) {
    char* buffer = new char [PAGE_SIZE];
    std::memcpy(buffer, data, PAGE_SIZE);
    file.seekp(0, std::ios_base::end);
    file.write(buffer, PAGE_SIZE);
    appendPageCounter++;
    return 0;
}

unsigned FileHandle::getNumberOfPages() {
    return appendPageCounter;
    // TODO
}

RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount) {
    readPageCount = readPageCounter;
    writePageCount = writePageCounter;
    appendPageCount = appendPageCounter;
    return 0;
}

RC FileHandle::openFile(const std::string &fileName) {
    file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
    if( !file.is_open()) {
        return -1;
    }
    else {
        // file >> readPageCounter;
        // file >> writePageCounter;
        // file >> appendPageCounter;
        return 0;
    }
}

RC FileHandle::closeFile() {
    file.seekp(0);
    file << readPageCounter;
    file << writePageCounter;
    file << appendPageCounter;
    file.close();
    return 0;
}

bool FileHandle::checkPageNum(int pageNum) {
    file.seekg(0, std::ios_base::end);
    auto fileEnd = file.tellg();
    auto pos = (pageOffset + pageNum) * PAGE_SIZE;
    if( pos < fileEnd) {
        return true;
    }
    else {
        return false;
    }
}

HiddenPage::HiddenPage() {
    this->readPageCounter = 0;
    this->writePageCounter = 0;
    this->appendPageCounter = 0;
    this->size = HIDDEN_PAGE_VAR_NUM * sizeof(unsigned);
}

HiddenPage::~HiddenPage() {

}

bool HiddenPage::isFull() {
    return size > PAGE_SIZE;
}

