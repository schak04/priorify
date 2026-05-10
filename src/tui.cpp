#include "tui.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

auto priorify = ScreenInteractive::Fullscreen();

enum class ScreenState {
    DASHBOARD,
    ADD_TASK
};

ScreenState currentState = ScreenState::DASHBOARD;

std::string newTaskNameBuffer = "";
std::string newTaskDescBuffer = "";
std::string newDateBuffer = ""; // dd-mm-yyyy; using string for now, will migrate to date obj later
std::string newStatusBuffer = ""; // Pending, Ongoing, Completed -> TODO: dropdown
std::string newPriorityBuffer = ""; // will soon make use of my old prioritization logic (min heap)

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

Element drawDashboard() {
    return vbox({
        text("PRIORIFY") | bold | center, // TODO: gotta make this ASCII-art-ish later
        separator(),
        filler(),
        text("Manage your tasks efficiently") | center,
        text("Press 'a' to add a new task") | color(Color::Cyan) | center,
        text("Press 'q' or 'esc' to exit") | dim | center,
        filler(),
        separator(),
        hbox({
            text("a") | bold, text(":Add  "),
            text("e") | bold, text(":Edit  "),
            text("d") | bold, text(":Delete  "),
            text("c") | bold, text(":Mark as completed"),
            filler(),
            text("Creator: Saptaparno Chakraborty (AKA Sapto/Sept) ") | dim,
        })
    }) | border;
}

Element drawAddTask() {
    auto focusOnField = [&](Field field, const std::string& label, const std::string& buffer) {
        if (activeField == field) {
            return hbox({
                text(" > " + label + ": ") | color(Color::Cyan) | bold,
                text(buffer) | color(Color::Yellow) | inverted,
                text(" ")
            }) | border;
        } else {
            return hbox({
                text("   " + label + ": "),
                text(buffer),
                text(" ")
            }) | border;
        }
    };

    return vbox({
        text("ADD NEW TASK") | bold | center,
        separator(),
        filler(),
        focusOnField(Field::Name,        "Name           ", newTaskNameBuffer),
        focusOnField(Field::Description, "Description    ", newTaskDescBuffer),
        focusOnField(Field::DueDate,     "Due Date       ", newDateBuffer),
        focusOnField(Field::Status,      "Status         ", newStatusBuffer),
        focusOnField(Field::Priority,    "Priority       ", newPriorityBuffer),
        filler(),
        separator(),
        text("TAB/Up/Down: Switch Field  |  ENTER: Save  |  ESC: Cancel") | dim | center,
    }) | border;
}

Element makeTUI() {
    if (currentState == ScreenState::DASHBOARD) {
        return drawDashboard();
    } else if (currentState == ScreenState::ADD_TASK) {
        return drawAddTask();
    }
    return text("How'd you even get here?") | center;
}

bool handleEvent(Event event) {
    if (currentState == ScreenState::DASHBOARD) {
        if (event == Event::Character('q') || event == Event::Escape) {
            priorify.ExitLoopClosure()();
            return true;
        }
        if (event == Event::Character('a')) {
            currentState = ScreenState::ADD_TASK;
            return true;
        }
    }
    else if (currentState == ScreenState::ADD_TASK) {
        if (event == Event::Escape) {
            newTaskNameBuffer = "";
            newTaskDescBuffer = "";
            newDateBuffer = "";
            newStatusBuffer = "";
            newPriorityBuffer = "";
            activeField = Field::Name;
            currentState = ScreenState::DASHBOARD;
            return true;
        }
        if (event == Event::Tab || event == Event::ArrowDown) {
            ++activeField;
            return true;
        }
        if (event == Event::ArrowUp) {
            --activeField;
            return true;
        }
        if (event == Event::Return) {
            // DB stuff soon
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
            if (event == Event::Backspace) {
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