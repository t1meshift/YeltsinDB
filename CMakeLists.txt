cmake_minimum_required(VERSION 3.15)
project(YeltsinDB C)

set(CMAKE_C_STANDARD 11)

include_directories(inc)

add_library(YeltsinDB STATIC
        src/ydb.c inc/YeltsinDB/ydb.h
        inc/YeltsinDB/error_code.h
        src/table_page.c inc/YeltsinDB/table_page.h
        inc/YeltsinDB/constants.h
        inc/YeltsinDB/types.h
        inc/YeltsinDB/macro.h
        )