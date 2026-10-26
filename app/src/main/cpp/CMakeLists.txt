cmake_minimum_required(VERSION 3.10.2)
project(motogp08)

add_subdirectory(SDL)

add_library(motogp08 SHARED
    main.cpp
)

target_include_directories(motogp08 PRIVATE SDL/include)

target_link_libraries(motogp08 GLESv3  GLESv3 
    SDL2
    SDL2main
    android
    log
)
