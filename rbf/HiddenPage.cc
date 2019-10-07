#include "HiddenPage.h"
#include "pfm.h"

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