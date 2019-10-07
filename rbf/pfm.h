#ifndef _pfm_h_
#define _pfm_h_

typedef unsigned PageNum;
typedef int RC;
typedef unsigned char byte;

#define PAGE_SIZE 4096

#include <string>
#include <cstring>
#include <climits>
#include <fstream>
#include <vector>
#include <utility>

class FileHandle;
class HiddenPage;

#define HIDDEN_PAGE_VAR_NUM 4

class PagedFileManager {
public:
    static PagedFileManager &instance();                                // Access to the _pf_manager instance

    RC createFile(const std::string &fileName);                         // Create a new file
    RC destroyFile(const std::string &fileName);                        // Destroy a file
    RC openFile(const std::string &fileName, FileHandle &fileHandle);   // Open a file
    RC closeFile(FileHandle &fileHandle);                               // Close a file

protected:
    PagedFileManager();                                                 // Prevent construction
    ~PagedFileManager();                                                // Prevent unwanted destruction
    PagedFileManager(const PagedFileManager &);                         // Prevent construction by copying
    PagedFileManager &operator=(const PagedFileManager &);              // Prevent assignment

private:
    static PagedFileManager *_pf_manager;
};

class FileHandle {
public:
    // variables to keep the counter for each operation
    unsigned readPageCounter;
    unsigned writePageCounter;
    unsigned appendPageCounter;
    static const int pageOffset = 1;

    FileHandle();                                                       // Default constructor
    ~FileHandle();                                                      // Destructor

    RC readPage(PageNum pageNum, void *data);     // TODO: read datapage                      // Get a specific page
    RC writePage(PageNum pageNum, const void *data);                    // Write a specific page
    RC appendPage(const void *data);                                    // Append a specific page

    RC openFile(const std::string &fileName);   // TODO: read hiddenpage
    RC closeFile();                             // TODO: wirte hiddenpage
    
    unsigned getNumberOfPages();                                        // Get the number of pages in the file
    RC collectCounterValues(unsigned &readPageCount, unsigned &writePageCount,
                            unsigned &appendPageCount);                 // Put current counter values into variables
private:
    
    bool checkPageNum(int);

    std::fstream file;
    HiddenPage* hiddenPage;
    //DataPage* currentpage;
};

class HiddenPage {
public:

    unsigned readPageCounter;
    unsigned writePageCounter;
    unsigned appendPageCounter;

    //unsigned size;
    unsigned pageNum;
    // unsigned pageNum;


    HiddenPage();
    ~HiddenPage();

    void readHiddenPage(std::fstream& file);
    void writeHiddenPage(std::fstream& file);
    bool isFull();

private:
    HiddenPage * hiddenPage;
};

class DataPage {
public:

    DataPage();
    ~DataPage();


    // insertRecord();
    // isFull();

private:


};
#endif