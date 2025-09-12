cmake_minimum_required(VERSION 3.12)

project(emulator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    ${PROJECT_NAME}
    main.cpp
    cmd_parser.cpp
)

# Optional: Enable warnings for better code quality
# target_compile_options(${PROJECT_NAME} PRIVATE
#     -Wall
#     -Wextra
#     -Wconversion
#     -Wsign-conversion
#     -Wshadow
#     -Wpedantic
#     -Werror
#     -Wdeprecated
# )