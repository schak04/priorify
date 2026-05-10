#include "tui.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

/*
--- drawn on the screen ---
*/
auto priorify = ScreenInteractive::Fullscreen();

enum class ScreenState {
    DASHBOARD,
    ADD_TASK
};

ScreenState currentState = ScreenState::DASHBOARD;

std::string newTaskNameBuffer = "";

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
    return vbox({
        text("ADD NEW TASK") | bold | center,
        separator(),
        filler(),
        hbox({
            text(" Task Name: "),
            text(newTaskNameBuffer) | color(Color::Yellow),
            text(" ")
        }) | border | center,
        filler(),
        separator(),
        text("Press 'Enter' to Save  |  'Esc' to Cancel") | dim | center,
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

/*
--- listening for key pressing events ---
*/

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
            currentState = ScreenState::DASHBOARD;
            return true;
        }
        if (event == Event::Return) {
            // DB stuff soon
            newTaskNameBuffer = "";
            currentState = ScreenState::DASHBOARD;
            return true;
        }
        if (event == Event::Backspace) {
            if (!newTaskNameBuffer.empty()) {
                newTaskNameBuffer.pop_back();
            }
            return true;
        }
        if (event.is_character()) {
            newTaskNameBuffer += event.character();
            return true;
        }
    }
    return false;
}

void runTUI() {
    auto display = Renderer(makeTUI);
    display = CatchEvent(display, handleEvent);

    priorify.Loop(display);
}