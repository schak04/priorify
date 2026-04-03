#ifndef TASK_H // new learning: header guards prevent multiple inclusions
#define TASK_H

#include <string>

class Task {
public:
    int priority;
    std::string taskName;
    std::string taskDesc;
    std::string date;
    bool completed;
};

struct CompareTask {
    bool operator()(const Task& a, const Task& b) const {
        return a.priority > b.priority;
    }
};

#endif