#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <vector>
#include "task.h"
#include "db.h"

class TaskManager {
public:
    void addTask(const Task& task);
    void showAllTasks();
    void updateTask(const Task& og, const Task& edited);
    void toggleCompletionStatus(const Task& t);
    void removeTask(const Task& t);
    void clearAllTasks();
    std::vector<Task> getTasks();
};

#endif