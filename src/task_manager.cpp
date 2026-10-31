#include "../headers/task_manager.h"
#include <iostream>
#include <vector>

void TaskManager::addTask(const Task& task) {
    if (addTaskToDB(task)) {
        std::cout << "Task added successfully.\n";
    } else {
        std::cout << "Failed to add task to database.\n";
    }
}

void TaskManager::updateTask(const Task& og, const Task& edited) {
    updateTaskInDB(og, edited);
}

void TaskManager::toggleCompletionStatus(const Task& t) {
    toggleCompletionStatusInDB(t);
}

void TaskManager::swapTaskPositions(const Task& a, const Task& b) {
    swapTaskPositionsInDB(a, b);
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
