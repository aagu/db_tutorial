cmake_minimum_required(VERSION 3.15)
project(sqlmini C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(db
        src/utils.c src/table.c src/btree.c src/pager.c src/db.c)
