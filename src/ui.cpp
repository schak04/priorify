#include "ui.h"
#include <iostream>
#include <string>
#include "task.h"
#include "db.h"

using namespace std;

void UI::printWelcomePage() {
    cout << "=================================================================\n";
    cout << "| ~ ~ ~ ~ ~  Priorify - A Priority-Based Task Manager ~ ~ ~ ~ ~ |\n";
    cout << "=================================================================\n";
    cout << "| ~ ~ ~ ~ ~ ~ ~ ~ Manage your tasks efficiently ~ ~ ~ ~ ~ ~ ~ ~ |\n";
    cout << "=================================================================\n";
    cout << "What's your first name? "; cin >> fname; cout << "\n";
}

void UI::printMenu() {
    cout << "=================================================================\n";
    cout << "What would you like to do, " << fname << "?\n";
    cout << "1. Add Task\n";
    cout << "2. Update Task\n";
    cout << "3. Mark Task as Completed\n";
    cout << "4. Show All Tasks\n";
    cout << "5. Remove Task\n";
    cout << "6. Clear All Tasks\n";
    cout << "0. Exit\n";
    cout << "=================================================================\n";
}

void UI::run() {
    printWelcomePage();
    int choice;
    do {
        printMenu();
        cout << "Enter your choice: ";
        cin >> choice;
        switch (choice) {
        case 1: {
            Task t;
            cout << "Enter task name: "; cin.ignore(); getline(cin, t.taskName);
            cout << "Enter task description: "; getline(cin, t.taskDesc);
            cout << "Enter due date (DD-MM-YYYY): "; getline(cin, t.date);
            cout << "Enter priority (1 = highest priority): ";
            while (true) {
                cin >> t.priority;
                if (t.priority >= 1) break;
                cout << "Invalid input. Please enter a positive integer (1 = highest priority): ";
            }
            t.completed = false;
            tm.addTask(t);
            break;
        }
        case 2:
            tm.updateTask();
            break;
        case 3:
            tm.markAsCompleted();
            break;
        case 4:
            tm.showAllTasks();
            break;
        case 5:
            tm.removeTask();
            break;
        case 6:
            tm.clearAllTasks();
            break;
        case 0:
            cout << "Exiting...\n";
            break;
        default:
            cout << "Invalid input. Please enter a valid option.\n";
            break;
        }
    } while (choice != 0);
}
