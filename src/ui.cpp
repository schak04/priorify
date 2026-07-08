#include <iostream>
#include <string>
#include "../headers/ui.h"
#include "../headers/task.h"
#include "../headers/db.h"

void UI::printWelcomePage() {
    std::cout << "=================================================================\n";
    std::cout << "| ~ ~ ~ ~ ~  Priorify - A Priority-Based Task Manager ~ ~ ~ ~ ~ |\n";
    std::cout << "=================================================================\n";
    std::cout << "| ~ ~ ~ ~ ~ ~ ~ ~ Manage your tasks efficiently ~ ~ ~ ~ ~ ~ ~ ~ |\n";
    std::cout << "=================================================================\n";
    std::cout << "What's your first name? "; std::cin >> fname; std::cout << "\n";
}

void UI::printMenu() {
    std::cout << "=================================================================\n";
    std::cout << "What would you like to do, " << fname << "?\n";
    std::cout << "1. Add Task\n";
    std::cout << "2. Update Task\n";
    std::cout << "3. Mark Task as Completed\n";
    std::cout << "4. Show All Tasks\n";
    std::cout << "5. Remove Task\n";
    std::cout << "6. Clear All Tasks\n";
    std::cout << "0. Exit\n";
    std::cout << "=================================================================\n";
}

void UI::run() {
    printWelcomePage();
    int choice;
    do {
        printMenu();
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        switch (choice) {
        case 1: {
            Task t;
            std::cout << "Enter task name: "; std::cin.ignore(); std::getline(std::cin, t.taskName);
            std::cout << "Enter task description: "; std::getline(std::cin, t.taskDesc);
            std::cout << "Enter due date (DD-MM-YYYY): "; std::getline(std::cin, t.date);
            std::cout << "Enter priority (1 = highest priority): ";
            while (true) {
                std::cin >> t.priority;
                if (t.priority >= 1) break;
                std::cout << "Invalid input. Please enter a positive integer (1 = highest priority): ";
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
            std::cout << "Exiting...\n";
            break;
        default:
            std::cout << "Invalid input. Please enter a valid option.\n";
            break;
        }
    } while (choice != 0);
}
