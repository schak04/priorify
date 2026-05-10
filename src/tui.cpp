#include "tui.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

/*
drawn on the screen
*/
auto priorify = ScreenInteractive::Fullscreen();

Element makeTUI() {
    Element mainUIBox = vbox({
        text("PRIORIFY") | bold | center, // TODO: gotta make this ASCII-art-ish later
        separator(),
        filler(),
        text("Manage your tasks efficiently") | center,
        text("Press q or esc to exit") | dim | center,
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

    return mainUIBox;
}

/*
listening for key pressing events
*/
bool handleEvent(Event event) {
    if (event == Event::Character('q') || event == Event::Escape) {
        auto fn = priorify.ExitLoopClosure();
        fn();
        return true;
    }
    return false;
}

void runTUI() {
    auto display = Renderer(makeTUI);
    display = CatchEvent(display, handleEvent);

    priorify.Loop(display);
}