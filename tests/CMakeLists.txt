# EH3D-NODE-HMI-GUI host tests: the settings file, the gate between the operator and the machine, and the voice, built with any C++17
# compiler: cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host -C Debug
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
cmake_minimum_required(VERSION 3.16)
project(eh3d_node_hmi_gui_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/W4 /WX /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(test_hmi test_hmi.cpp)
add_executable(test_workflow test_workflow.cpp)
add_executable(test_keyboard test_keyboard.cpp)
add_executable(test_notices test_notices.cpp)
add_executable(test_machine test_machine.cpp)

enable_testing()
add_test(NAME hmi COMMAND test_hmi)
add_test(NAME workflow COMMAND test_workflow)
add_test(NAME keyboard COMMAND test_keyboard)
add_test(NAME notices COMMAND test_notices)
add_test(NAME machine COMMAND test_machine)
