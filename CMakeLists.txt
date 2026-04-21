cmake_minimum_required(VERSION 3.14)
project(priorify LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_library(sqlite3 STATIC ${LIB_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${LIB_DIR})

add_executable(priorify
    ${SRC_DIR}/db.cpp
    ${SRC_DIR}/task_manager.cpp
    ${SRC_DIR}/tui.cpp
    # ${SRC_DIR}/ui.cpp
    ${SRC_DIR}/main.cpp
)

target_link_libraries(priorify PRIVATE sqlite3 pthread dl) # since SQLite needs pthread and dl on Linux


# --- FTXUI Integration ---

include(FetchContent)

FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0
)

FetchContent_MakeAvailable(ftxui)

target_link_libraries(priorify PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
)

set_target_properties(priorify PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)