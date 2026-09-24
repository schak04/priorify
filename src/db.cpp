#include "../headers/db.h"
#include <iostream>
#include <filesystem>

sqlite3* db = nullptr;

bool initDB() {
    std::filesystem::create_directories("data");

    int rc = sqlite3_open("data/tasks.db", &db);
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
            completed INTEGER,
            position INTEGER NOT NULL DEFAULT 0
        );
    )";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    // migrate older databases that lack the position column (for same-priority reordering)
    bool hasPositionColumn = false;
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, "PRAGMA table_info(tasks);", -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (colName && std::string(colName) == "position") {
                hasPositionColumn = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
    }
    if (!hasPositionColumn) {
        rc = sqlite3_exec(db, "ALTER TABLE tasks ADD COLUMN position INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        // seed positions by insert order so existing tasks keep their current visible order
        rc = sqlite3_exec(db, "UPDATE tasks SET position = id;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
    }

    return true;
}

bool addTaskToDB(const Task& t) {
    const char* sql = "INSERT INTO tasks (name, description, due_date, priority, completed, position) VALUES (?, ?, ?, ?, ?, (SELECT COALESCE(MAX(position), -1) + 1 FROM tasks WHERE priority = ?));";
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
    sqlite3_bind_int(stmt, 6, t.priority);
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
    const char* sql = "SELECT id, name, description, due_date, priority, completed, position FROM tasks ORDER BY priority ASC, position ASC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to fetch tasks: " << sqlite3_errmsg(db) << std::endl;
        return tasks;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Task t;
        t.id = sqlite3_column_int(stmt, 0);
        t.taskName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.taskDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        t.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t.priority = sqlite3_column_int(stmt, 4);
        t.completed = sqlite3_column_int(stmt, 5);
        t.position = sqlite3_column_int(stmt, 6);
        tasks.push_back(t);
    }
    sqlite3_finalize(stmt);
    return tasks;
}

bool updateTaskInDB(const Task& original, const Task& updated) {
    const char* sql = R"(
        UPDATE tasks
        SET name = ?, description = ?, due_date = ?, priority = ?, completed = ?
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
    sqlite3_bind_int(stmt, 5, updated.completed);
    sqlite3_bind_text(stmt, 6, original.taskName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, original.taskDesc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, original.date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, original.priority);
    sqlite3_bind_int(stmt, 10, original.completed ? 1 : 0);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to update task in database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool toggleCompletionStatusInDB(const Task& t) {
    const char* sql = "UPDATE tasks SET completed = NOT completed WHERE name = ? AND description = ? AND due_date = ? AND priority = ?;";
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

bool swapTaskPositionsInDB(const Task& a, const Task& b) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    const char* sql = "UPDATE tasks SET position = ? WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare position update: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    bool success = true;
    sqlite3_bind_int(stmt, 1, b.position);
    sqlite3_bind_int(stmt, 2, a.id);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to swap positions: " << sqlite3_errmsg(db) << std::endl;
        success = false;
    }
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, a.position);
    sqlite3_bind_int(stmt, 2, b.id);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to swap positions: " << sqlite3_errmsg(db) << std::endl;
        success = false;
    }
    sqlite3_finalize(stmt);

    sqlite3_exec(db, success ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    return success;
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
