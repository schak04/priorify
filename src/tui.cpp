#include <string>
#include <cctype>
#include <vector>
#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include "task.h"
#include "task_manager.h"
#include "tui.h"

/*
---------- Custom Colours ----------
(to ensure terminal emulators using some theme
still render the original colours of the app)
*/

const auto COL_CYAN    = ftxui::Color::RGB(86, 210, 227);
const auto COL_YELLOW  = ftxui::Color::RGB(250, 189, 47);
const auto COL_RED     = ftxui::Color::RGB(234, 74, 74);
const auto COL_GREEN   = ftxui::Color::RGB(100, 200, 100);
const auto COL_GRAY    = ftxui::Color::RGB(100, 100, 100);
const auto COL_BLACK   = ftxui::Color::RGB(30, 30, 30);

/*
---------- Global State ----------
*/

ftxui::ScreenInteractive priorify = ftxui::ScreenInteractive::Fullscreen();

enum class ScreenState {
    DASHBOARD,
    ADD_TASK,
    CONFIRM_DELETE,
    CONFIRM_CLEAR_ALL,
    EDIT_TASK,
    CALENDAR_PICKER,
    ABOUT
};

ScreenState currentState = ScreenState::DASHBOARD;
ScreenState previousState = ScreenState::DASHBOARD;

// input buffers for task creation + editing
std::string taskNameBuffer = "";
std::string taskDescBuffer = "";
std::string dateBuffer = "";
// status and priority states
const std::vector<std::string> statusOptions = {"TODO", "DONE"};
int selectedStatusIndex = 0;
const std::vector<std::string> priorityOptions = {"High", "Medium", "Low", "None"};
int selectedPriorityIndex = 3;
// due date -> calendar states
// month/day are stored as unsigned to match std::chrono::month/day's underlying value type
int calViewingYear = 2026;
unsigned calViewingMonth = 1;
unsigned calCursorDay = 1;
bool calInitialised = false;

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

ftxui::Element keyHint(const std::string& key, const std::string& purpose) {
    return ftxui::hbox({
        ftxui::text(" " + key + ": ") | ftxui::bold | ftxui::color(COL_YELLOW),
        ftxui::text(purpose + "  ") | ftxui::dim
    });
}

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
    selectedStatusIndex = 0;
    selectedPriorityIndex = 3;
    activeField = Field::Name;
    currentState = ScreenState::DASHBOARD;
}

bool handleTypingAndSelection(ftxui::Event& event) {
    if (activeField == Field::Status) {
        if (event == ftxui::Event::ArrowLeft || event == ftxui::Event::Character('h') ||
            event == ftxui::Event::ArrowRight || event == ftxui::Event::Character('l')) {
            selectedStatusIndex = (selectedStatusIndex + 1) % 2;
            return true;
        }
        return false;
    }

    if (activeField == Field::Priority) {
        if (event == ftxui::Event::ArrowLeft || event == ftxui::Event::Character('h')) {
            selectedPriorityIndex = (selectedPriorityIndex + 3) % 4;
            return true;
        }
        if (event == ftxui::Event::ArrowRight || event == ftxui::Event::Character('l')) {
            selectedPriorityIndex = (selectedPriorityIndex + 1) % 4;
            return true;
        }
        return false;
    }

    std::string* currentBuffer = nullptr;
    if (activeField == Field::Name) currentBuffer = &taskNameBuffer;
    else if (activeField == Field::Description) currentBuffer = &taskDescBuffer;
    else if (activeField == Field::DueDate) currentBuffer = &dateBuffer;

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
    task.completed = (selectedStatusIndex == 1);
    
    if (selectedPriorityIndex == 0) task.priority = 1;
    else if (selectedPriorityIndex == 1) task.priority = 2;
    else if (selectedPriorityIndex == 2) task.priority = 3;
    else task.priority = 0;
    
    return task;
}

void initCalendar() {
    // figure out today
    // now -> a std::chrono::time_point representing the current point in time. for future ref: https://en.cppreference.com/cpp/chrono/system_clock/now
    const auto now = std::chrono::system_clock::now(); 

    // calendar representation (cuz now is a time_point, not a calendar date) -> need to store data in yyyy-mm-dd format. for future ref: https://en.cppreference.com/cpp/chrono/year_month_day
    // round down now to day precision (example: 2026-05-29 18:52:11 -> 2026-05-29 00:00:00) because std::chrono::year_month_day cannot directly understand a high precision time_point with hours/mins/secs/nanosecs. It expects a sys_days. (sys_days -> time_point truncated to whole days). Note: sys_days is also basically days elapsed since Unix epoch.
    std::chrono::year_month_day today{std::chrono::floor<std::chrono::days>(now)};

    calViewingYear = static_cast<int>(today.year());
    calViewingMonth = static_cast<unsigned>(today.month());
    calCursorDay = static_cast<unsigned>(today.day());
    calInitialised = true;
}

// these two are needed by drawCalendarPicker()
int getDaysInMonth(int year, unsigned month) {
    std::chrono::year_month ym{std::chrono::year(year), std::chrono::month(month)}; // represent the given month/year as a calendar month
    std::chrono::year_month_day_last ymdl{ym/std::chrono::last}; // get the "last day of this month" (calendar rep)
    return static_cast<unsigned>(ymdl.day());
}
int getStartingWeekday(int year, unsigned month) { // 0 for Sunday, 1 for Monday, ..., 6 for Saturday
    std::chrono::year_month_day firstOfMonth{std::chrono::year(year), std::chrono::month(month), std::chrono::day(1)};
    std::chrono::sys_days firstSysDays = firstOfMonth; // (time_point) because weekday works with sys_days
    std::chrono::weekday firstWeekday{firstSysDays};
    // convert weekday into a number
    return firstWeekday.c_encoding();
}

/*
---------- TUI Rendering ----------
*/

ftxui::Element drawLogo() {
    return ftxui::vbox({
        ftxui::text("█▀█ █▀█ █ █▀█ █▀█ █ █▀▀ █▄█") | ftxui::center | ftxui::color(COL_CYAN),
        ftxui::text("█▀▀ █▀▄ █ █▄█ █▀▄ █ █▀   █ ") | ftxui::center | ftxui::color(COL_YELLOW)
    }) | ftxui::bold;
}

ftxui::Element drawDashboard() {
    ftxui::Elements taskElements;

    if (cachedTasks.empty()) {
        taskElements.push_back(ftxui::text("No tasks found. Press 'a' to add one!") | ftxui::center | ftxui::dim);
    } else {
        for (int i = 0; i < cachedTasks.size(); i++) {
            const auto& task = cachedTasks[i];
            ftxui::Color priorityColor = COL_GRAY; // priority 0
            if (task.priority == 1) priorityColor = COL_RED;
            else if (task.priority == 2) priorityColor = COL_YELLOW;
            else if (task.priority == 3) priorityColor = COL_GREEN;

            auto element = ftxui::hbox({
                ftxui::text("  ") | ftxui::color(priorityColor) | ftxui::bgcolor(priorityColor), // priority colour tag
                ftxui::paragraph(" " + task.taskName) | ftxui::bold | ftxui::flex,
                ftxui::filler(),
                ftxui::text(task.date + " ") | ftxui::dim,
                ftxui::text(task.completed ? "[DONE]" : "[TODO]") 
                    | ftxui::color(task.completed ? COL_GREEN : COL_YELLOW)
            });

            if (selectedTaskIndex == i) {
                element = element | ftxui::focus | ftxui::inverted;
            }

            taskElements.push_back(element | ftxui::borderRounded);
        }
    }

    return ftxui::vbox({
        drawLogo(),
        ftxui::separator(),
        ftxui::vbox(std::move(taskElements)) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex,
        ftxui::separator(),
        ftxui::hbox({
            keyHint("a", "Add"),
            keyHint("e", "Edit"),
            keyHint("d", "Delete"),
            keyHint("D", "Clear all"),
            keyHint("c", "Toggle completion status"),
            keyHint("q/Esc", "Quit"),
            ftxui::filler(),
            keyHint("?", "About"),
            ftxui::text("v2.0.0 ") | ftxui::color(COL_CYAN) | ftxui::bold
        })
    }) | ftxui::borderRounded;
}

// to draw task creation + editing forms
ftxui::Element drawTaskDetailsForm(const std::string& formTitle) {
    auto focusOnField = [&](Field field, const std::string& label, const std::string& buffer) {
        if (activeField == field) {
            return ftxui::hbox({
                ftxui::text(" > " + label + ": ") | ftxui::color(COL_CYAN) | ftxui::bold,
                ftxui::text(buffer) | ftxui::color(COL_YELLOW) | ftxui::inverted,
                ftxui::text(" ")
            }) | ftxui::borderRounded;
        } else {
            return ftxui::hbox({
                ftxui::text("   " + label + ": "),
                ftxui::text(buffer),
                ftxui::text(" ")
            }) | ftxui::borderRounded;
        }
    };
    auto drawPills = [&](Field field, const std::string& label, const std::vector<std::string>& options, int selectedIndex) {
        ftxui::Elements pills;
        pills.push_back(ftxui::text(activeField == field ? " > " + label + ": " : "   " + label + ": ") 
                        | (activeField == field ? ftxui::color(COL_CYAN) | ftxui::bold : ftxui::nothing));
        for (int i=0; i<options.size(); i++) {
            ftxui::Element optionText = ftxui::text("  " + options[i] + "  ");
            if (i == selectedIndex) {
                if (field == Field::Priority) {
                    if (i == 0) optionText |= ftxui::bgcolor(COL_RED);
                    else if (i == 1) optionText |= ftxui::bgcolor(COL_YELLOW);
                    else if (i == 2) optionText |= ftxui::bgcolor(COL_GREEN);
                    else optionText |= ftxui::bgcolor(COL_GRAY);
                } else if (field == Field::Status) {
                    if (i == 0) optionText |= ftxui::bgcolor(COL_YELLOW);
                    else optionText |= ftxui::bgcolor(COL_GREEN);
                }
                optionText |= ftxui::color(COL_BLACK) | ftxui::bold;
            } else {
                optionText |= ftxui::dim;
            }
            pills.push_back(optionText);
            pills.push_back(ftxui::text(" "));
        }
        return ftxui::hbox(std::move(pills)) | ftxui::borderRounded;
    };

    return ftxui::vbox({
        ftxui::text(formTitle) | ftxui::bold | ftxui::center,
        ftxui::separator(),
        ftxui::filler(),
        focusOnField(Field::Name,        "Name           ", taskNameBuffer),
        focusOnField(Field::Description, "Description    ", taskDescBuffer),
        focusOnField(Field::DueDate,     "Due Date       ", dateBuffer),
        drawPills(Field::Status,         "Status         ", statusOptions, selectedStatusIndex),
        drawPills(Field::Priority,       "Priority       ", priorityOptions, selectedPriorityIndex),
        ftxui::filler(),
        ftxui::separator(),
        ftxui::vbox({
            ftxui::hbox({
                keyHint("TAB/Up/Down", "Switch Field"),
                keyHint("Left/Right", "Change Options"),
                keyHint("Spacebar (on Due Date)", "Pick Date from Calendar")
            }) | ftxui::center,
            ftxui::hbox({
                keyHint("Enter", "Save"),
                keyHint("Esc", "Cancel")
            }) | ftxui::center
        })
    }) | ftxui::borderRounded;
}

ftxui::Element drawConfirmDelete() {
    return ftxui::vbox({
        ftxui::text(" ! WARNING ! ") | ftxui::bold | ftxui::center | ftxui::color(COL_RED),
        ftxui::separator(),
        ftxui::filler(),
        ftxui::text("Are you sure you want to delete this task?") | ftxui::center,
        ftxui::text("\"" + cachedTasks[selectedTaskIndex].taskName + "\"") | ftxui::bold | ftxui::center | ftxui::color(COL_YELLOW),
        ftxui::filler(),
        ftxui::separator(),
        ftxui::hbox({
            keyHint("y/Enter", "Yes, delete it"),
            keyHint("n/Esc", "No, cancel")
        }) | ftxui::center,
    }) | ftxui::borderRounded | ftxui::center;
}

ftxui::Element drawConfirmClearAll() {
    return ftxui::vbox({
        ftxui::text(" !!! DANGER ZONE !!! ") | ftxui::bold | ftxui::center | ftxui::color(COL_RED),
        ftxui::separator(),
        ftxui::filler(),
        ftxui::text("This will permanently REMOVE ALL TASKS.") | ftxui::center,
        ftxui::text("This action CANNOT be undone.") | ftxui::center | ftxui::color(COL_RED),
        ftxui::filler(),
        ftxui::separator(),
        ftxui::hbox({
            keyHint("y", "Yes, NUKE"),
            keyHint("n/Esc", "No, abort")
        }) | ftxui::center,
    }) | ftxui::borderRounded | ftxui::center;
}

ftxui::Element drawCalendarPicker() {
    if (!calInitialised) initCalendar();

    int daysInMonth = getDaysInMonth(calViewingYear, calViewingMonth);
    int startingWeekday = getStartingWeekday(calViewingYear, calViewingMonth);

    std::vector<ftxui::Elements> calRows;

    auto headerCell = [](const char* t) {return ftxui::text(t) | ftxui::bold | ftxui::color(COL_YELLOW) | ftxui::center | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 6);};
    calRows.push_back({headerCell("Su"), headerCell("Mo"), headerCell("Tu"), headerCell("We"), headerCell("Th"), headerCell("Fr"), headerCell("Sa")});    

    ftxui::Elements currentRow;
    for (int i=0; i<startingWeekday; i++) {
        currentRow.push_back(ftxui::text("  ") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 6));
    }
    for (unsigned day=1; day<=daysInMonth; day++) {
        std::string dayStr = std::to_string(day);
        if (dayStr.length()==1) dayStr = " "+dayStr;
        auto dayText = ftxui::text(dayStr) | ftxui::center | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 6);
        if (day==calCursorDay) dayText |= ftxui::inverted;
        currentRow.push_back(dayText);
        if (currentRow.size()==7) {
            calRows.push_back(currentRow);
            currentRow.clear(); 
        }
    }
    if (!currentRow.empty()) {
        while (currentRow.size()<7) {
            currentRow.push_back(ftxui::text(" "));
        }
    }
    calRows.push_back(currentRow);

    auto calendar = ftxui::gridbox(calRows) | ftxui::center;

    static const char* const monthNames[] = {"January", "February", "March", "April", "May",
                                             "June", "July", "August", "September", "October",
                                             "November", "December"};
    std::string monthNameAndYear = std::string(monthNames[calViewingMonth-1]) + " " + std::to_string(calViewingYear);

    auto hints = ftxui::vbox({
        ftxui::hbox({
            keyHint("  ]/[", "Change Month"),
            keyHint("h/j/k/l/arrows", "Move Day Cursor")
        }) | ftxui::center,
        ftxui::hbox({
            keyHint("  Enter", "Select Date"),
            keyHint("Esc", "Cancel")
        }) | ftxui::center
    });

    return ftxui::vbox({
        ftxui::text(monthNameAndYear) | ftxui::bold | ftxui::center | ftxui::color(COL_CYAN),
        ftxui::separator(),
        ftxui::filler(),
        calendar,
        ftxui::filler(),
        ftxui::separator(),
        hints
    }) | ftxui::borderRounded | ftxui::center;
}

ftxui::Element drawAboutScreen() {
    return ftxui::vbox({
        drawLogo(),
        ftxui::separator(),
        ftxui::filler(),
        ftxui::text("A minimalist, terminal-based task manager made for absolute focus.") | ftxui::center | ftxui::bold,
        ftxui::text("This project is intended to help users manage and prioritise their tasks") | ftxui::center,
        ftxui::text("with zero distractions.") | ftxui::center,
        ftxui::text(""),
        ftxui::text("Designed for people who enjoy keyboard-driven, distraction-free tools.") | ftxui::center | ftxui::color(COL_YELLOW),
        ftxui::text(""),
        ftxui::text("I created this to make my own task tracking easier, as") | ftxui::center,
        ftxui::text("I spend a lot of time in the terminal and genuinely enjoy") | ftxui::center,
        ftxui::text("TUI apps (and C++). I hope it helps others too.") | ftxui::center,
        ftxui::text(""),
        ftxui::text("~ Saptaparno Chakraborty") | ftxui::center | ftxui::bold | ftxui::color(COL_CYAN),
        ftxui::text("GitHub/GitLab username: schak04") | ftxui::center | ftxui::dim,
        ftxui::filler(),
        ftxui::separator(),
        ftxui::hbox({
            keyHint("Esc/?", "Return to Dashboard")
        }) | ftxui::center
    }) | ftxui::borderRounded;
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
    } else if (currentState == ScreenState::CALENDAR_PICKER) {
        return drawCalendarPicker();
    } else if (currentState == ScreenState::ABOUT) {
        return drawAboutScreen();
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
            selectedStatusIndex = taskToBeEdited.completed ? 1 : 0;

            if (taskToBeEdited.priority == 1) selectedPriorityIndex = 0;
            else if (taskToBeEdited.priority == 2) selectedPriorityIndex = 1;
            else if (taskToBeEdited.priority == 3) selectedPriorityIndex = 2;
            else selectedPriorityIndex = 3;

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
        if (event == ftxui::Event::Character('?')) {
            currentState = ScreenState::ABOUT;
            return true;
        }
    }
    else if (currentState == ScreenState::ABOUT) {
        if (event == ftxui::Event::Escape || event == ftxui::Event::Character('q') || event == ftxui::Event::Character('?')) {
            currentState = ScreenState::DASHBOARD;
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
        if (activeField == Field::DueDate && event == ftxui::Event::Character(' ')) {
            previousState = currentState;
            currentState = ScreenState::CALENDAR_PICKER;
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

        handleTypingAndSelection(event);
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
        if (activeField == Field::DueDate && event == ftxui::Event::Character(' ')) {
            previousState = currentState;
            currentState = ScreenState::CALENDAR_PICKER;
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

        handleTypingAndSelection(event);
    }
    else if (currentState == ScreenState::CALENDAR_PICKER) {
        if (event == ftxui::Event::Escape) {
            currentState = previousState;
            return true;        
        }

        int daysInMonth = getDaysInMonth(calViewingYear, calViewingMonth);
 
        if (event == ftxui::Event::Character('l') || event == ftxui::Event::ArrowRight) {
            calCursorDay++;
            if (calCursorDay>daysInMonth) calCursorDay = daysInMonth;
            return true;
        }
        if (event == ftxui::Event::Character('h') || event == ftxui::Event::ArrowLeft) {
            if (calCursorDay>1) calCursorDay--;
            else calCursorDay = 1;
            return true;
        }
        if (event == ftxui::Event::Character('j') || event == ftxui::Event::ArrowDown) {
            calCursorDay += 7;
            if (calCursorDay>daysInMonth) calCursorDay = daysInMonth;
            return true;
        }
        if (event == ftxui::Event::Character('k') || event == ftxui::Event::ArrowUp) {
            if (calCursorDay>7) calCursorDay -= 7;
            else calCursorDay = 1;
            return true;
        }

        if (event == ftxui::Event::Character(']')) {
            calViewingMonth++;
            if (calViewingMonth>12) {
                calViewingMonth = 1;
                calViewingYear++;
            }
            int newDays = getDaysInMonth(calViewingYear, calViewingMonth);
            if (calCursorDay>newDays) calCursorDay = newDays;
            return true;
        }
        if (event == ftxui::Event::Character('[')) {
            calViewingMonth--;
            if (calViewingMonth<1) {
                calViewingMonth = 12;
                calViewingYear--;
            }
            int newDays = getDaysInMonth(calViewingYear, calViewingMonth);
            if (calCursorDay>newDays) calCursorDay = newDays;
            return true;
        }

        if (event == ftxui::Event::Return) {
            std::string d = (calCursorDay<10 ? "0" : "") + std::to_string(calCursorDay);
            std::string m = (calViewingMonth<10 ? "0" : "") + std::to_string(calViewingMonth);
            dateBuffer = d + "-" + m + "-" + std::to_string(calViewingYear);
            currentState = previousState;
            return true;
        }
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