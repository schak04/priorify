#include "db.h"
#include <iostream>

sqlite3* db = nullptr;

bool initDB() {
    int rc = sqlite3_open("../data/tasks.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT,
            due_date TEXT,
            priority INTEGER,
            completed INTEGER
        );
    )";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool addTaskToDB(const Task& t) {
    const char* sql = "INSERT INTO tasks (name, description, due_date, priority, completed) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, t.taskName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.taskDesc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, t.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, t.priority);
    sqlite3_bind_int(stmt, 5, t.completed ? 1 : 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to insert task: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

std::vector<Task> getAllTasksFromDB() {
    std::vector<Task> tasks;
    const char* sql = "SELECT name, description, due_date, priority, completed FROM tasks ORDER BY priority ASC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to fetch tasks: " << sqlite3_errmsg(db) << std::endl;
        return tasks;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Task t;
        t.taskName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        t.taskDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        t.priority = sqlite3_column_int(stmt, 3);
        t.completed = sqlite3_column_int(stmt, 4);
        tasks.push_back(t);
    }
    sqlite3_finalize(stmt);
    return tasks;
}

bool updateTaskInDB(const Task& original, const Task& updated) {
    const char* sql = R"(
        UPDATE tasks
        SET name = ?, description = ?, due_date = ?, priority = ?
        WHERE name = ? AND description = ? AND due_date = ? AND priority = ? AND completed = ?
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, updated.taskName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, updated.taskDesc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, updated.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, updated.priority);
    sqlite3_bind_text(stmt, 5, original.taskName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, original.taskDesc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, original.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, original.priority);
    sqlite3_bind_int(stmt, 9, original.completed ? 1 : 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to update task in database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool markTaskCompletedInDB(const Task& t) {
    const char* sql = "UPDATE tasks SET completed = 1 WHERE name = ? AND description = ? AND due_date = ? AND priority = ? AND completed = 0;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, t.taskName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.taskDesc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, t.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, t.priority);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to update task: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool deleteTaskFromDB(const Task& t) {
    const char* sql = "DELETE FROM tasks WHERE name = ? AND description = ? AND due_date = ? AND priority = ? AND completed = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare delete statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, t.taskName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.taskDesc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, t.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, t.priority);
    sqlite3_bind_int(stmt, 5, t.completed ? 1 : 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to delete task: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool clearAllTasksFromDB() {
    const char* sql = "DELETE FROM tasks;";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to clear tasks from database: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

void closeDB() {
    if (db) {
        sqlite3_close(db);
    }
}