#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include "task.h"
#include "db.h"
using namespace std;

class TaskManager {
    public:
    priority_queue<Task, vector<Task>, CompareTask> tasks;
    vector<Task> tasksVec;

    void addTask(Task task) {
        tasks.push(task);
        cout << "Task added successfully.\n";
    }

    void showAllTasks() {
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

    void makeTasksVec() {
        tasksVec.clear();
        priority_queue<Task, vector<Task>, CompareTask> temp = tasks;
        while (!temp.empty()) {
            tasksVec.push_back(temp.top());
            temp.pop();
        }
    }

    void rebuildPriorityQueue() {
        tasks = priority_queue<Task, vector<Task>, CompareTask>();
        for (Task& t : tasksVec) tasks.push(t);
    }

    void updateTask() {
        showAllTasks();
        makeTasksVec();
        cout << "Which task do you want to update? (Enter task no.): ";
        int tn; cin >> tn; cin.ignore(); tn--;
        if (tn < 0 || tn >= tasksVec.size()) {
            cout << "Invalid task number.\n";
            return;
        }

        Task original = tasksVec[tn];

        cout << "Alright. Updating Task " << tn + 1 << "...\n";
        cout << "Enter a new name for the task \"" << tasksVec[tn].taskName << "\": ";
        getline(cin, tasksVec[tn].taskName);
        cout << "Enter a new description for the task \"" << tasksVec[tn].taskName << "\": ";
        getline(cin, tasksVec[tn].taskDesc);
        cout << "Enter a new due date for the task \"" << tasksVec[tn].taskName << "\": ";
        getline(cin, tasksVec[tn].date);
        cout << "Enter a new priority for the task \"" << tasksVec[tn].taskName << "\": ";
        cin >> tasksVec[tn].priority;

        if (!updateTaskInDB(original, tasksVec[tn])) {
            return;
        }

        rebuildPriorityQueue();
        cout << "Task updated successfully.\n";
    }

    void markAsCompleted() {
        showAllTasks();
        makeTasksVec();
        cout << "Which task would you like to mark as completed? (Enter task no.): ";
        int tn;
        cin >> tn;
        tn--;
        if (tn < 0 || tn >= tasksVec.size()) {
            cout << "Invalid task number.\n";
            return;
        }

        Task& t = tasksVec[tn];
        t.completed = true;

        if (!markTaskCompletedInDB(t)) {
            return;
        }
        
        rebuildPriorityQueue();

        cout << "Selected task marked as completed.\n";
    }

    void removeTask() {
        showAllTasks();
        makeTasksVec();

        cout << "Which task would you like to remove? (Enter task no.): ";
        int tn;
        cin >> tn;
        tn--;

        if (tn < 0 || tn >= tasksVec.size()) {
            cout << "Invalid task number.\n";
            return;
        }

        Task taskToDelete = tasksVec[tn];

        if (!deleteTaskFromDB(taskToDelete)) {
            return;
        }

        tasksVec.erase(tasksVec.begin() + tn);

        rebuildPriorityQueue();

        cout << "Task removed successfully.\n";
    }

    void clearAllTasks() {
        if (!clearAllTasksFromDB()) {
            return;
        }

        while (!tasks.empty()) tasks.pop();   
        cout << "Clearing all the tasks...\n";
        cout << "All tasks cleared successfully.\n";
    }
};

string fname;

void printWelcomePage() {
    cout << "=================================================================\n";
    cout << "| ~ ~ ~ ~ ~  Priorify - A Priority-Based Task Manager ~ ~ ~ ~ ~ |\n";
    cout << "=================================================================\n";
    cout << "| ~ ~ ~ ~ ~ ~ ~ ~ Manage your tasks efficiently ~ ~ ~ ~ ~ ~ ~ ~ |\n";
    cout << "=================================================================\n";
    cout << "What's your first name? "; cin>>fname; cout<<"\n";
}

void printMenu() {
    cout << "=================================================================\n";
    cout << "What would you like to do, " << fname << "?\n";
    cout << "1. Add Task\n";
    cout << "2. Update Task\n";
    cout << "3. Mark Task as Completed\n";
    cout << "4. Show All Tasks\n";
    cout << "5. Remove Task\n";
    cout << "6. Clear All Tasks\n";
    cout << "0. Exit\n";
    cout << "=================================================================\n";
}


int main() {
    if (!initDB()) {
        cerr << "Database initialization failed. Exiting...\n";
        return 1;
    }

    printWelcomePage();
    TaskManager tm;
    int choice;
    do {
        printMenu();
        cout << "Enter your choice: ";
        cin >> choice;
        switch (choice) {
        case 1: {
            Task t;
            cout << "Enter task name: "; cin.ignore(); getline(cin, t.taskName);
            cout << "Enter task description: "; getline(cin, t.taskDesc);
            cout << "Enter due date (DD-MM-YYYY): "; getline(cin, t.date);
            cout << "Enter priority (1 = highest priority): ";
            while (true) {
                cin >> t.priority;
                if (t.priority >= 1) break;
                cout << "Invalid input. Please enter a positive integer (1 = highest priority): ";
            }
            t.completed = false;
            if (tm.addTaskToDB(t)) {
                tm.addTask(t);
            } else {
                cout << "Failed to add task to database.\n";
            }
            break;
        }
        case 2: {
            tm.updateTask();
            break;
        }
        case 3: {
            tm.markAsCompleted();
            break;
        }
        case 4: {
            tm.showAllTasks();
            break;
        }
        case 5: {
            tm.removeTask();
            break;
        }
        case 6: {
            tm.clearAllTasks();
            break;
        }
        case 0:
            cout << "Exiting...\n";
            break;
        default:
            cout << "Invalid input. Please enter a valid option.\n";
            break;
        }
    } while (choice != 0);

    closeDB();
    return 0;
}