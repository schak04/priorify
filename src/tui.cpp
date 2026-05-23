#include <string>
#include <cctype>
#include <vector>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include "task.h"
#include "task_manager.h"
#include "tui.h"

/*
---------- Global State ----------
*/

ftxui::ScreenInteractive priorify = ftxui::ScreenInteractive::Fullscreen();

enum class ScreenState {
    DASHBOARD,
    ADD_TASK,
    CONFIRM_DELETE,
    CONFIRM_CLEAR_ALL,
    EDIT_TASK
};

ScreenState currentState = ScreenState::DASHBOARD;

// input buffers for task creation + editing
std::string taskNameBuffer = "";
std::string taskDescBuffer = "";
std::string dateBuffer = ""; // dd-mm-yyyy; using string for now, will migrate to date obj later
std::string statusBuffer = ""; // Pending, Ongoing, Completed/Done -> TODO: radio
std::string priorityBuffer = ""; // High/Medium/Low (any case) or 1/2/3 (0 otherwise)

// state updates
std::vector<Task> cachedTasks;
void refreshTasks() {
    TaskManager manager;
    cachedTasks = manager.getTasks();
}

enum class Field {
    Name,        // 0
    Description, // 1
    DueDate,     // 2
    Status,      // 3
    Priority,    // 4
    Count        // 5; not a real field, just for focus switching help
};

int selectedTaskIndex = 0;

Field activeField = Field::Name;

Field& operator++(Field& field) {
    field = static_cast<Field>(
        (static_cast<int>(field) + 1) % static_cast<int>(Field::Count)
    );
    return field;
}
Field& operator--(Field& field) {
    field = static_cast<Field>(
        (static_cast<int>(field) + static_cast<int>(Field::Count) - 1) % static_cast<int>(Field::Count)
    );
    return field;
}

// Postfix overloads kept for future use. Not going to need these anytime soon though.
// Field operator++(Field& field, int) {
//     Field old = field;
//     ++field;
//     return old;
// }
// Field operator--(Field& field, int) {
//     Field old = field;
//     --field;
//     return old;
// }

/*
---------- Helper Functions ----------
*/

// strings to lowercase
std::string to_lower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

// reset input buffers + active field + current screen state
void resetBuffnFieldnState() {
    taskNameBuffer = "";
    taskDescBuffer = "";
    dateBuffer = "";
    statusBuffer = "";
    priorityBuffer = "";
    activeField = Field::Name;
    currentState = ScreenState::DASHBOARD;
}

bool handleTyping(ftxui::Event& event) {
    std::string* currentBuffer = nullptr;
    if (activeField == Field::Name) currentBuffer = &taskNameBuffer;
    else if (activeField == Field::Description) currentBuffer = &taskDescBuffer;
    else if (activeField == Field::DueDate) currentBuffer = &dateBuffer;
    else if (activeField == Field::Status) currentBuffer = &statusBuffer;
    else if (activeField == Field::Priority) currentBuffer = &priorityBuffer;

    if (currentBuffer) {
        if (event == ftxui::Event::Backspace) {
            if (!currentBuffer->empty()) currentBuffer->pop_back();
            return true;
        }
        if (event.is_character()) {
            *currentBuffer += event.character();
            return true;
        }
    }

    return false;
}

Task parseTaskFromBuffers() {
    Task task;
    task.taskName = taskNameBuffer;
    task.taskDesc = taskDescBuffer;
    task.date = dateBuffer;
    // status translation to true/false (completed or not)
    if (to_lower(statusBuffer) == "completed" || to_lower(statusBuffer) == "done") {
        task.completed = true;
    }
    else {
        task.completed = false;
    }
    
    try {
        if (!priorityBuffer.empty() && !std::isdigit(priorityBuffer[0])) {
            std::string p = to_lower(priorityBuffer);
            if (p == "high") task.priority = 1;
            else if (p == "medium") task.priority = 2;
            else if (p == "low") task.priority = 3;
            else task.priority = 0;
        }
        else if (!priorityBuffer.empty() && std::isdigit(priorityBuffer[0])) {
            int p = stoi(priorityBuffer);
            if (p <= 3 && p >= 1) task.priority = p;
            else if (p > 3) task.priority = 3;
            else task.priority = 0;
        }
        else task.priority = 0;
    } catch (...) {
        task.priority = 0;
    }
    return task;
}

/*
---------- TUI Rendering ----------
*/

ftxui::Element drawDashboard() {
    ftxui::Elements taskElements;
    
    if (cachedTasks.empty()) {
        taskElements.push_back(ftxui::text("No tasks found. Press 'a' to add one!") | ftxui::center | ftxui::dim);
    } else {
        for (int i = 0; i < cachedTasks.size(); i++) {
            const auto& task = cachedTasks[i];
            ftxui::Color priorityColor = ftxui::Color::GrayDark; // priority 0
            if (task.priority == 1) priorityColor = ftxui::Color::Red;
            else if (task.priority == 2) priorityColor = ftxui::Color::Yellow;
            else if (task.priority == 3) priorityColor = ftxui::Color::Green;

            auto element = ftxui::hbox({
                ftxui::text("  ") | ftxui::color(priorityColor) | ftxui::bgcolor(priorityColor), // priority colour tag
                ftxui::paragraph(" " + task.taskName) | ftxui::bold | ftxui::flex,
                ftxui::filler(),
                ftxui::text(task.date + " ") | ftxui::dim,
                ftxui::text(task.completed ? "[DONE]" : "[TODO]") 
                    | ftxui::color(task.completed ? ftxui::Color::Green : ftxui::Color::Yellow)
            });

            if (selectedTaskIndex == i) {
                element = element | ftxui::focus | ftxui::inverted;
            }

            taskElements.push_back(element | ftxui::border);
        }
    }

    return ftxui::vbox({
        ftxui::text("PRIORIFY") | ftxui::bold | ftxui::center, // TODO: gotta make this ASCII-art-ish later
        ftxui::separator(),
        ftxui::vbox(std::move(taskElements)) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex,
        ftxui::separator(),
        ftxui::hbox({
            ftxui::text("a") | ftxui::bold, ftxui::text(":Add  "),
            ftxui::text("e") | ftxui::bold, ftxui::text(":Edit  "),
            ftxui::text("d") | ftxui::bold, ftxui::text(":Delete  "),
            ftxui::text("D") | ftxui::bold, ftxui::text(":Clear all  "),
            ftxui::text("c") | ftxui::bold, ftxui::text(":Toggle completion status  "),
            ftxui::text("q/esc") | ftxui::bold, ftxui::text(":Quit"),
            ftxui::filler(),
            ftxui::text("v2.0.0 ") | ftxui::dim,
        })
    }) | ftxui::border;
}

// to draw task creation + editing forms
ftxui::Element drawTaskDetailsForm(const std::string& formTitle) {
    auto focusOnField = [&](Field field, const std::string& label, const std::string& buffer) {
        if (activeField == field) {
            return ftxui::hbox({
                ftxui::text(" > " + label + ": ") | ftxui::color(ftxui::Color::Cyan) | ftxui::bold,
                ftxui::text(buffer) | ftxui::color(ftxui::Color::Yellow) | ftxui::inverted,
                ftxui::text(" ")
            }) | ftxui::border;
        } else {
            return ftxui::hbox({
                ftxui::text("   " + label + ": "),
                ftxui::text(buffer),
                ftxui::text(" ")
            }) | ftxui::border;
        }
    };
    return ftxui::vbox({
        ftxui::text(formTitle) | ftxui::bold | ftxui::center,
        ftxui::separator(),
        ftxui::filler(),
        focusOnField(Field::Name,        "Name           ", taskNameBuffer),
        focusOnField(Field::Description, "Description    ", taskDescBuffer),
        focusOnField(Field::DueDate,     "Due Date       ", dateBuffer),
        focusOnField(Field::Status,      "Status         ", statusBuffer),
        focusOnField(Field::Priority,    "Priority       ", priorityBuffer),
        ftxui::filler(),
        ftxui::separator(),
        ftxui::text("TAB/Up/Down: Switch Field  |  ENTER: Save  |  ESC: Cancel") | ftxui::dim | ftxui::center,
    }) | ftxui::border;
}

ftxui::Element drawConfirmDelete() {
    return ftxui::vbox({
        ftxui::text(" ! WARNING ! ") | ftxui::bold | ftxui::center | ftxui::color(ftxui::Color::Red),
        ftxui::separator(),
        ftxui::filler(),
        ftxui::text("Are you sure you want to delete this task?") | ftxui::center,
        ftxui::text("\"" + cachedTasks[selectedTaskIndex].taskName + "\"") | ftxui::bold | ftxui::center | ftxui::color(ftxui::Color::Yellow),
        ftxui::filler(),
        ftxui::separator(),
        ftxui::text(" [y/enter] Yes, I want to delete it.  |  [n/esc] No, that was a mistake. ") | ftxui::center,
    }) | ftxui::border | ftxui::center;
}

ftxui::Element drawConfirmClearAll() {
    return ftxui::vbox({
        ftxui::text(" !!! DANGER ZONE !!! ") | ftxui::bold | ftxui::center | ftxui::color(ftxui::Color::Red),
        ftxui::separator(),
        ftxui::filler(),
        ftxui::text("This will permanently REMOVE ALL TASKS.") | ftxui::center,
        ftxui::text("This action CANNOT be undone.") | ftxui::center | ftxui::color(ftxui::Color::Red),
        ftxui::filler(),
        ftxui::separator(),
        ftxui::text(" [y] Yes, NUKE.  |  [n/esc] No, abort. ") | ftxui::center,
    }) | ftxui::border | ftxui::center;
}

ftxui::Element makeTUI() {
    if (currentState == ScreenState::DASHBOARD) {
        return drawDashboard();
    } else if (currentState == ScreenState::ADD_TASK) {
        return drawTaskDetailsForm("ADD NEW TASK");
    } else if (currentState == ScreenState::EDIT_TASK) {
        return drawTaskDetailsForm("EDIT TASK");
    } else if (currentState == ScreenState::CONFIRM_DELETE) {
        return drawConfirmDelete();
    } else if (currentState == ScreenState::CONFIRM_CLEAR_ALL) {
        return drawConfirmClearAll();
    }
    return ftxui::text("How'd you even get here?") | ftxui::center;
}

/*
---------- Event Handling ----------
*/

bool handleEvent(ftxui::Event event) {
    if (currentState == ScreenState::DASHBOARD) {
        if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
            priorify.ExitLoopClosure()();
            return true;
        }
        if (event == ftxui::Event::Character('a')) {
            currentState = ScreenState::ADD_TASK;
            return true;
        }
        if (event == ftxui::Event::Character('e')) {
            Task taskToBeEdited = cachedTasks[selectedTaskIndex];
            taskNameBuffer = taskToBeEdited.taskName;
            taskDescBuffer = taskToBeEdited.taskDesc;
            dateBuffer = taskToBeEdited.date;
            statusBuffer = taskToBeEdited.completed ? "DONE" : "TODO";
            priorityBuffer = std::to_string(taskToBeEdited.priority);

            currentState = ScreenState::EDIT_TASK;
            return true;
        }
        if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('j')) {
            if (!cachedTasks.empty()) {
                selectedTaskIndex = (selectedTaskIndex + 1) % cachedTasks.size();
            }
            return true;
        }
        if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('k')) {
            if (!cachedTasks.empty()) {
                selectedTaskIndex = (selectedTaskIndex + cachedTasks.size() - 1) % cachedTasks.size();
            }
            return true;
        }
        if (event == ftxui::Event::Character('d')) {
            if (!cachedTasks.empty() && selectedTaskIndex >= 0 && selectedTaskIndex < cachedTasks.size()) {
                currentState = ScreenState::CONFIRM_DELETE;
            }
            return true;
        }
        if (event == ftxui::Event::Character('D')) {
            if (!cachedTasks.empty()) {
                currentState = ScreenState::CONFIRM_CLEAR_ALL;
            }
            return true;
        }
        if (event == ftxui::Event::Character('c')) {
            Task t = cachedTasks[selectedTaskIndex];
            TaskManager manager;
            manager.toggleCompletionStatus(t);
            refreshTasks();
            return true;
        }
    }
    else if (currentState == ScreenState::CONFIRM_DELETE) {
        if (event == ftxui::Event::Character('y') || event == ftxui::Event::Return) {
            TaskManager manager;
            manager.removeTask(cachedTasks[selectedTaskIndex]);
            refreshTasks();
            if (selectedTaskIndex >= cachedTasks.size() && !cachedTasks.empty()) {
                selectedTaskIndex = cachedTasks.size() - 1;
            }
            currentState = ScreenState::DASHBOARD;
            return true;
        }
        if (event == ftxui::Event::Character('n') || event == ftxui::Event::Escape) {
            currentState = ScreenState::DASHBOARD;
            return true;
        }
    }
    else if (currentState == ScreenState::CONFIRM_CLEAR_ALL) {
        if (event == ftxui::Event::Character('y')) {
            TaskManager manager;
            manager.clearAllTasks();
            refreshTasks();
            selectedTaskIndex = 0;
            currentState = ScreenState::DASHBOARD;
            return true;
        }
        if (event == ftxui::Event::Character('n') || event == ftxui::Event::Escape) {
            currentState = ScreenState::DASHBOARD;
            return true;
        }
    }
    else if (currentState == ScreenState::ADD_TASK) {
        if (event == ftxui::Event::Escape) {
            resetBuffnFieldnState();
            return true;
        }
        if (event == ftxui::Event::Tab || event == ftxui::Event::ArrowDown) {
            ++activeField;
            return true;
        }
        if (event == ftxui::Event::ArrowUp) {
            --activeField;
            return true;
        }
        if (event == ftxui::Event::Return) {
            Task task = parseTaskFromBuffers();
            TaskManager manager;
            manager.addTask(task);
            refreshTasks();
            resetBuffnFieldnState();
            return true;
        }

        handleTyping(event);
    }
    else if (currentState == ScreenState::EDIT_TASK) {
        if (event == ftxui::Event::Escape) {
            resetBuffnFieldnState();
            return true;
        }
        if (event == ftxui::Event::Tab || event == ftxui::Event::ArrowDown) {
            ++activeField;
            return true;
        }
        if (event == ftxui::Event::ArrowUp) {
            --activeField;
            return true;
        }
        if (event == ftxui::Event::Return) {
            Task task = parseTaskFromBuffers();
            TaskManager manager;
            manager.updateTask(cachedTasks[selectedTaskIndex], task);
            refreshTasks();
            resetBuffnFieldnState();
            return true;
        }

        handleTyping(event);
    }

    return false;
}

/*
---------- Entrypoint ----------
*/

void runTUI() {
    refreshTasks();
    auto display = ftxui::Renderer(makeTUI);
    display = ftxui::CatchEvent(display, handleEvent);

    priorify.Loop(display);
}