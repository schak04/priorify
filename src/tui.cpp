#include "tui.h"
#include <string>
#include <cctype>
#include <vector>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include "task_manager.h"

// --------- helpers ---------

// strings to lowercase
std::string to_lower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

// ----------------------------


ftxui::ScreenInteractive priorify = ftxui::ScreenInteractive::Fullscreen();

enum class ScreenState {
    DASHBOARD,
    ADD_TASK
};

ScreenState currentState = ScreenState::DASHBOARD;

std::string newTaskNameBuffer = "";
std::string newTaskDescBuffer = "";
std::string newDateBuffer = ""; // dd-mm-yyyy; using string for now, will migrate to date obj later
std::string newStatusBuffer = ""; // Pending, Ongoing, Completed -> TODO: dropdown
std::string newPriorityBuffer = ""; // High/Medium/Low (any case) or 1/2/3 (0 otherwise)

enum class Field {
    Name,        // 0
    Description, // 1
    DueDate,     // 2
    Status,      // 3
    Priority,    // 4
    Count        // 5; not a real field, just for focus switching help
};

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

ftxui::Element drawDashboard() {
    return ftxui::vbox({
        ftxui::text("PRIORIFY") | ftxui::bold | ftxui::center, // TODO: gotta make this ASCII-art-ish later
        ftxui::separator(),
        ftxui::filler(),
        ftxui::text("Manage your tasks efficiently") | ftxui::center,
        ftxui::text("Press 'a' to add a new task") | ftxui::color(ftxui::Color::Cyan) | ftxui::center,
        ftxui::text("Press 'q' or 'esc' to exit") | ftxui::dim | ftxui::center,
        ftxui::filler(),
        ftxui::separator(),
        ftxui::hbox({
            ftxui::text("a") | ftxui::bold, ftxui::text(":Add  "),
            ftxui::text("e") | ftxui::bold, ftxui::text(":Edit  "),
            ftxui::text("d") | ftxui::bold, ftxui::text(":Delete  "),
            ftxui::text("c") | ftxui::bold, ftxui::text(":Mark as completed"),
            ftxui::filler(),
            ftxui::text("Creator: Saptaparno Chakraborty (AKA Sapto/Sept) ") | ftxui::dim,
        })
    }) | ftxui::border;
}

ftxui::Element drawAddTask() {
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
        ftxui::text("ADD NEW TASK") | ftxui::bold | ftxui::center,
        ftxui::separator(),
        ftxui::filler(),
        focusOnField(Field::Name,        "Name           ", newTaskNameBuffer),
        focusOnField(Field::Description, "Description    ", newTaskDescBuffer),
        focusOnField(Field::DueDate,     "Due Date       ", newDateBuffer),
        focusOnField(Field::Status,      "Status         ", newStatusBuffer),
        focusOnField(Field::Priority,    "Priority       ", newPriorityBuffer),
        ftxui::filler(),
        ftxui::separator(),
        ftxui::text("TAB/Up/Down: Switch Field  |  ENTER: Save  |  ESC: Cancel") | ftxui::dim | ftxui::center,
    }) | ftxui::border;
}

ftxui::Element makeTUI() {
    if (currentState == ScreenState::DASHBOARD) {
        return drawDashboard();
    } else if (currentState == ScreenState::ADD_TASK) {
        return drawAddTask();
    }
    return ftxui::text("How'd you even get here?") | ftxui::center;
}

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
    }
    else if (currentState == ScreenState::ADD_TASK) {
        if (event == ftxui::Event::Escape) {
            newTaskNameBuffer = "";
            newTaskDescBuffer = "";
            newDateBuffer = "";
            newStatusBuffer = "";
            newPriorityBuffer = "";
            activeField = Field::Name;
            currentState = ScreenState::DASHBOARD;
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
            Task task;
            task.taskName = newTaskNameBuffer;
            task.taskDesc = newTaskDescBuffer;
            task.date = newDateBuffer;
            task.completed = false;
            
            try {
                if (!newPriorityBuffer.empty() && !std::isdigit(newPriorityBuffer[0])) {
                    std::string p = to_lower(newPriorityBuffer);
                    if (p == "high") task.priority = 1;
                    else if (p == "medium") task.priority = 2;
                    else if (p == "low") task.priority = 3;
                    else task.priority = 0;
                }
                else if (!newPriorityBuffer.empty() && std::isdigit(newPriorityBuffer[0])) {
                    int p = stoi(newPriorityBuffer);
                    if (p <= 3 && p >= 1) task.priority = p;
                    else if (p > 3) task.priority = 3;
                    else task.priority = 0;
                }
                else task.priority = 0;
            } catch (...) {
                task.priority = 0;
            }

            TaskManager manager;
            manager.addTask(task);

            newTaskNameBuffer = "";
            newTaskDescBuffer = "";
            newDateBuffer = "";
            newStatusBuffer = "";
            newPriorityBuffer = "";
            activeField = Field::Name;
            currentState = ScreenState::DASHBOARD;
            return true;
        }

        std::string* currentBuffer = nullptr;
        if (activeField == Field::Name) currentBuffer = &newTaskNameBuffer;
        else if (activeField == Field::Description) currentBuffer = &newTaskDescBuffer;
        else if (activeField == Field::DueDate) currentBuffer = &newDateBuffer;
        else if (activeField == Field::Status) currentBuffer = &newStatusBuffer;
        else if (activeField == Field::Priority) currentBuffer = &newPriorityBuffer;

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
    }
    return false;
}

void runTUI() {
    auto display = Renderer(makeTUI);
    display = CatchEvent(display, handleEvent);

    priorify.Loop(display);
}