#ifndef UI_H
#define UI_H

#include <string>
#include "task_manager.h"

class UI {
public:
    void printWelcomePage();
    void printMenu();
    void run();

private:
    std::string fname;
    TaskManager tm;
};

#endif
