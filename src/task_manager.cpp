#include "task_manager.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

void TaskManager::addTask(Task task) {
    if (addTaskToDB(task)) {
        cout << "Task added successfully.\n";
    } else {
        cout << "Failed to add task to database.\n";
    }
}

void TaskManager::showAllTasks() {
    vector<Task> tasksFromDB = getAllTasksFromDB();
    int idx = 1;
    if (tasksFromDB.empty()) {
        cout << "No tasks to show.\n";
        return;
    }
    for (const auto& t : tasksFromDB) {
        cout << "--------------------------\n";
        cout << "Task " << idx++ << "\n";
        cout << "Priority: " << t.priority << "\n";
        cout << "Task Name: " << t.taskName << "\n";
        cout << "Description: " << t.taskDesc << "\n";
        cout << "Due date: " << t.date << "\n";
        cout << "Completed: " << (t.completed ? "Yes" : "No") << "\n";
    }
    cout << "--------------------------\n";
}

void TaskManager::updateTask() {
    vector<Task> tasksFromDB = getAllTasksFromDB();
    if (tasksFromDB.empty()) {
        cout << "No tasks to update.\n";
        return;
    }

    showAllTasks();
    cout << "Which task do you want to update? (Enter task no.): ";
    int tn; cin >> tn; cin.ignore(); tn--;
    if (tn < 0 || tn >= tasksFromDB.size()) {
        cout << "Invalid task number.\n";
        return;
    }

    Task original = tasksFromDB[tn];
    Task updated = original;

    cout << "Alright. Updating Task " << tn + 1 << "...\n";
    cout << "Enter a new name (Current: \"" << original.taskName << "\"): ";
    string input;
    getline(cin, input);
    if (!input.empty()) updated.taskName = input;

    cout << "Enter a new description (Current: \"" << original.taskDesc << "\"): ";
    getline(cin, input);
    if (!input.empty()) updated.taskDesc = input;

    cout << "Enter a new due date (Current: \"" << original.date << "\"): ";
    getline(cin, input);
    if (!input.empty()) updated.date = input;

    cout << "Enter a new priority (Current: " << original.priority << "): ";
    if (cin >> updated.priority) cin.ignore();

    if (updateTaskInDB(original, updated)) {
        cout << "Task updated successfully.\n";
    }
}

void TaskManager::markAsCompleted() {
    vector<Task> tasksFromDB = getAllTasksFromDB();
    if (tasksFromDB.empty()) {
        cout << "No tasks to mark as completed.\n";
        return;
    }

    showAllTasks();
    cout << "Which task would you like to mark as completed? (Enter task no.): ";
    int tn; cin >> tn; cin.ignore(); tn--;
    if (tn < 0 || tn >= tasksFromDB.size()) {
        cout << "Invalid task number.\n";
        return;
    }

    Task& t = tasksFromDB[tn];
    if (t.completed) {
        cout << "Task is already completed.\n";
        return;
    }

    if (markTaskCompletedInDB(t)) {
        cout << "Selected task marked as completed.\n";
    }
}

void TaskManager::removeTask() {
    vector<Task> tasksFromDB = getAllTasksFromDB();
    if (tasksFromDB.empty()) {
        cout << "No tasks to remove.\n";
        return;
    }

    showAllTasks();
    cout << "Which task would you like to remove? (Enter task no.): ";
    int tn; cin >> tn; cin.ignore(); tn--;
    if (tn < 0 || tn >= tasksFromDB.size()) {
        cout << "Invalid task number.\n";
        return;
    }

    if (deleteTaskFromDB(tasksFromDB[tn])) {
        cout << "Task removed successfully.\n";
    }
}

void TaskManager::clearAllTasks() {
    if (clearAllTasksFromDB()) {
        cout << "All tasks cleared successfully.\n";
    }
}