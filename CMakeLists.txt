cmake_minimum_required(VERSION 3.16)

project(cpp_log_analyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(log_analyzer_core
    src/FileManager.cpp
    src/LogAnalyzer.cpp
    src/LogParser.cpp
    src/ReportGenerator.cpp
    src/Timestamp.cpp
)
target_include_directories(log_analyzer_core PUBLIC include)

if(MSVC)
    target_compile_options(log_analyzer_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(log_analyzer_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(log-analyzer src/main.cpp)
target_link_libraries(log-analyzer PRIVATE log_analyzer_core)

if(MSVC)
    target_compile_options(log-analyzer PRIVATE /W4 /permissive-)
else()
    target_compile_options(log-analyzer PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(log_analyzer_tests tests/LogAnalyzerTests.cpp)
    target_link_libraries(log_analyzer_tests PRIVATE log_analyzer_core)
    if(MSVC)
        target_compile_options(log_analyzer_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(log_analyzer_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
    add_test(NAME log_analyzer_tests COMMAND log_analyzer_tests)
    set_tests_properties(log_analyzer_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
