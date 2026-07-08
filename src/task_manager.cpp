#include "../headers/task_manager.h"
#include <iostream>
#include <string>
#include <vector>

void TaskManager::addTask(const Task& task) {
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

void TaskManager::updateTask(const Task& og, const Task& edited) {
    updateTaskInDB(og, edited);
}

void TaskManager::toggleCompletionStatus(const Task& t) {
    toggleCompletionStatusInDB(t);
}

void TaskManager::removeTask(const Task& t) {
    deleteTaskFromDB(t);
}

void TaskManager::clearAllTasks() {
    if (clearAllTasksFromDB()) {
        std::cout << "All tasks cleared successfully.\n";
    }
}

std::vector<Task> TaskManager::getTasks() {
    return getAllTasksFromDB();
}
