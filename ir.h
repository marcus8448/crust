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
    Byte,
    // 8bit
    Word,
    // 16bit
    Long,
    // 32bit
    Quad // 64bit
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
