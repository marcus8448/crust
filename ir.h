#ifndef IR_H
#define IR_H

typedef enum
{
    MOV,
    PUSH,
    POP,
    SUB,
    AND,
    OR,
    CALL
} OpType;

typedef enum
{
    // 8bit
    Byte,
    // 16bit
    Word,
    // 32bit
    Long,
    // 64bit
    Quad
} Size;

typedef struct
{
    Size size;
    OpType operation;
    char* values[3];
} Statement;

void statement_init(Statement* statement);

char get_suffix_s(Size size);
#endif //IR_H
