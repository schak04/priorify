#include "db.h"
#include "ui.h"
#include <iostream>

using namespace std;

int main() {
    if (!initDB()) {
        cerr << "Database initialization failed. Exiting...\n";
        return 1;
    }

    UI ui;
    ui.run();

    closeDB();
    return 0;
}