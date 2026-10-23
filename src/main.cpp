#include "../headers/db.h"
#include "../headers/tui.h"
#include <iostream>

int main() {
    if (!initDB()) {
        std::cerr << "Database initialization failed. Exiting...\n";
        return 1;
    }

    runTUI();

    closeDB();
    return 0;
}
