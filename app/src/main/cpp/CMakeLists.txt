cmake_minimum_required(VERSION 3.10.2)
project(motogp08)

# Add SDL2 source subdirectory
add_subdirectory(SDL)

add_library(motogp08 SHARED
    main.cpp
)

target_include_directories(motogp08 PRIVATE SDL/include)

target_link_libraries(motogp08
    SDL2
    android
    log
)
