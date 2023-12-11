cmake_minimum_required(VERSION 3.27)
project(crust C)

set(CMAKE_C_STANDARD 17)

add_executable(crust src/main.c
        src/struct/list.c
        src/struct/list.h
        src/token.h
        src/token.c
        src/ast.c
        src/ast.h
        src/preprocess.h
        src/preprocess.c
        src/register.c
        src/register.h
        src/types.c
        src/types.h
        src/ir.c
        src/ir.h
        src/util.h
)

get_target_property(SOURCE_FILES crust SOURCES)

find_program(ClangFormat clang-format)
if (ClangFormat)
    add_custom_target(FormatCode COMMAND ${ClangFormat} -i ${SOURCE_FILES} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMENT "Formatting code")
endif()

if (ClangTidy)
    add_custom_target(Tidy COMMAND ${ClangTidy} ${SOURCE_FILES} -- ${CMAKE_C_FLAGS} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMENT "Checking code")
endif()

find_program(ClangTidy clang-tidy)

if (WIN32)
    target_compile_definitions(crust PRIVATE _CRT_SECURE_NO_WARNINGS strdup=_strdup)
endif()

