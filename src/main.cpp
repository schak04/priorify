#include "../headers/db.h"
#include "../headers/tui.h"
// #include "../headers/ui.h"
#include <iostream>

int main() {
    if (!initDB()) {
        std::cerr << "Database initialization failed. Exiting...\n";
        return 1;
    }

    // UI ui;
    // ui.run();
    runTUI();

    closeDB();
    return 0;
}
