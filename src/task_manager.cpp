#include "task_manager.h"
#include <iostream>
#include <string>
#include <vector>

void TaskManager::addTask(Task task) {
    if (addTaskToDB(task)) {
        std::cout << "Task added successfully.\n";
    } else {
        std::cout << "Failed to add task to database.\n";
    }
}

void TaskManager::showAllTasks() {
    std::vector<Task> tasksFromDB = getAllTasksFromDB();
    int idx = 1;
    if (tasksFromDB.empty()) {
        std::cout << "No tasks to show.\n";
        return;
    }
    for (const auto& t : tasksFromDB) {
        std::cout << "--------------------------\n";
        std::cout << "Task " << idx++ << "\n";
        std::cout << "Priority: " << t.priority << "\n";
        std::cout << "Task Name: " << t.taskName << "\n";
        std::cout << "Description: " << t.taskDesc << "\n";
        std::cout << "Due date: " << t.date << "\n";
        std::cout << "Completed: " << (t.completed ? "Yes" : "No") << "\n";
    }
    std::cout << "--------------------------\n";
}

void TaskManager::updateTask() {
    std::vector<Task> tasksFromDB = getAllTasksFromDB();
    if (tasksFromDB.empty()) {
        std::cout << "No tasks to update.\n";
        return;
    }

    showAllTasks();
    std::cout << "Which task do you want to update? (Enter task no.): ";
    int tn; std::cin >> tn; std::cin.ignore(); tn--;
    if (tn < 0 || tn >= (int)tasksFromDB.size()) {
        std::cout << "Invalid task number.\n";
        return;
    }

    Task original = tasksFromDB[tn];
    Task updated = original;

    std::cout << "Alright. Updating Task " << tn + 1 << "...\n";
    std::cout << "Enter a new name (Current: \"" << original.taskName << "\"): ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) updated.taskName = input;

    std::cout << "Enter a new description (Current: \"" << original.taskDesc << "\"): ";
    std::getline(std::cin, input);
    if (!input.empty()) updated.taskDesc = input;

    std::cout << "Enter a new due date (Current: \"" << original.date << "\"): ";
    std::getline(std::cin, input);
    if (!input.empty()) updated.date = input;

    std::cout << "Enter a new priority (Current: " << original.priority << "): ";
    if (std::cin >> updated.priority) std::cin.ignore();

    if (updateTaskInDB(original, updated)) {
        std::cout << "Task updated successfully.\n";
    }
}

void TaskManager::markAsCompleted() {
    std::vector<Task> tasksFromDB = getAllTasksFromDB();
    if (tasksFromDB.empty()) {
        std::cout << "No tasks to mark as completed.\n";
        return;
    }

    showAllTasks();
    std::cout << "Which task would you like to mark as completed? (Enter task no.): ";
    int tn; std::cin >> tn; std::cin.ignore(); tn--;
    if (tn < 0 || tn >= (int)tasksFromDB.size()) {
        std::cout << "Invalid task number.\n";
        return;
    }

    Task& t = tasksFromDB[tn];
    if (t.completed) {
        std::cout << "Task is already completed.\n";
        return;
    }

    if (markTaskCompletedInDB(t)) {
        std::cout << "Selected task marked as completed.\n";
    }
}

void TaskManager::removeTask() {
    std::vector<Task> tasksFromDB = getAllTasksFromDB();
    if (tasksFromDB.empty()) {
        std::cout << "No tasks to remove.\n";
        return;
    }

    showAllTasks();
    std::cout << "Which task would you like to remove? (Enter task no.): ";
    int tn; std::cin >> tn; std::cin.ignore(); tn--;
    if (tn < 0 || tn >= (int)tasksFromDB.size()) {
        std::cout << "Invalid task number.\n";
        return;
    }

    if (deleteTaskFromDB(tasksFromDB[tn])) {
        std::cout << "Task removed successfully.\n";
    }
}

void TaskManager::clearAllTasks() {
    if (clearAllTasksFromDB()) {
        std::cout << "All tasks cleared successfully.\n";
    }
}

std::vector<Task> TaskManager::getTasks() {
    return getAllTasksFromDB();
}