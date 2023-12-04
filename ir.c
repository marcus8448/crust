#include "ir.h"

#include <stddef.h>

void statement_init(Statement* statement)
{
    statement->values[0] = NULL;
    statement->values[1] = NULL;
    statement->values[2] = NULL;
}

char get_suffix_s(const Width size)
{
    switch (size)
    {
    case Byte:
        return 'b';
    case Word:
        return 'w';
    case Long:
        return 'l';
    case Quad:
        return 'q';
    }
    return 0;
}
