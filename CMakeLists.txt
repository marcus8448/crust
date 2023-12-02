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
        linkedlist.c
        linkedlist.h
        stack.c
        stack.h
        preprocess.h
        preprocess.c
        register.c
        register.h
        types.c
        types.h
        ir.c
        ir.h
)

target_compile_definitions(crust PRIVATE _CRT_SECURE_NO_WARNINGS)
