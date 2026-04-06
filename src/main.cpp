#include "db.h"
#include "ui.h"
#include <iostream>

int main() {
    if (!initDB()) {
        std::cerr << "Database initialization failed. Exiting...\n";
        return 1;
    }

    UI ui;
    ui.run();

    closeDB();
    return 0;
}