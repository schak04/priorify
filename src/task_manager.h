#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <vector>
#include "task.h"
#include "db.h"

class TaskManager {
public:
    void addTask(Task task);
    void showAllTasks();
    void updateTask(Task& og, Task& edited);
    void toggleCompletionStatus(Task& t);
    void removeTask(const Task& t);
    void clearAllTasks();
    std::vector<Task> getTasks();
};

#endif