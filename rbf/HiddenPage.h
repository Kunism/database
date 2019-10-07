#ifndef CS222_FALL19_HIDDENPAGE_H
#define CS222_FALL19_HIDDENPAGE_H
const unsigned HIDDEN_PAGE_VAR_NUM = 3;

class HiddenPage {
public:

    unsigned readPageCounter;
    unsigned writePageCounter;
    unsigned appendPageCounter;

    // unsigned size;
    // unsigned pageNum;


    HiddenPage();
    ~HiddenPage();

    bool isFull();

private:

};
#endif //CS222_FALL19_HIDDENPAGE_H
