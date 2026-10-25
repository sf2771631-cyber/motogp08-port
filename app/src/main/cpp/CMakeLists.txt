cmake_minimum_required(VERSION 3.10.2)
project("motogp08")

add_library(
    motogp08
    SHARED
    main.cpp
)

find_library(
    log-lib
    log
)

target_link_libraries(
    motogp08
    ${log-lib}
)
