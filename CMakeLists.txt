cmake_minimum_required(VERSION 3.27)
project(crust C)

set(CMAKE_C_STANDARD 17)

add_executable(crust main.c
        list.h
        token.h
        token.c
        list.c
        ast.c
        ast.h
)

target_compile_definitions(crust PRIVATE _CRT_SECURE_NO_WARNINGS)
