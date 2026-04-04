#ifndef DB_H
#define DB_H

#include "task.h"
#include <vector>
#include <string>

extern "C" {
    #include "../lib/sqlite3.h"
}

extern sqlite3* db;

bool initDB();
bool addTaskToDB(const Task& t);
std::vector<Task> getAllTasksFromDB();
bool updateTaskInDB(const Task& original, const Task& updated);
bool markTaskCompletedInDB(const Task& t);
bool deleteTaskFromDB(const Task& t);
bool clearAllTasksFromDB();
void closeDB();

#endif