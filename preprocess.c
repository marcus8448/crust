#include "preprocess.h"

#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "types.h"

Result preprocess_globals(char* contents, const Token *token, StrList *strLiterals, FILE* output)
{
    while (token != NULL)
    {
        switch (token->type)
        {
        case keyword_define:
            {
                token = token->next;
                token_matches(token, identifier);
                const Token *id = token;

                token = token->next;
                token_matches(token, opening_paren);

                while ((token = token->next)->type != closing_paren)
                {
                    if (token->type == identifier)
                    {
                        strlist_add(args, data_copy(data, *token));
                    }

                    token = token->next;
                    token_matches(token, colon);

                    Type type;
                    parse_type(contents, &token, &type);

                    if ((token = token->next)->type == closing_paren) break;
                    token_matches_ext(token, comma, ", or )");
                }


                return success();

                break;
            }
        case keyword_var:
            {
                token = token->next;
                token_matches(token, identifier);
                const Token *id = token;
                token = token->next;
                token_matches(token, colon);
                Type type;
                parse_type(contents, &token, &type);

                token = token->next;
                if (token->type == semicolon)
                {
                    variable_size(type.kind);
                    fprintf(output, "%.*s:\n", id->len, contents + id->index);
                    const int bytes = size_bytes(variable_size(type.kind));
                    fprintf(output, "\t.zero\t%i\n", bytes);
                    fprintf(output,  "\t.size\t%.*s, %i\n", id->len, contents + id->index, bytes);
                } else
                {
                    token_matches(token, equals_assign);
                    token = token->next;
                    const Token *value = token;
                    token_matches(token, constant);
                    token = token->next;
                    token_matches(token, semicolon);
                    const int bytes = size_bytes(variable_size(type.kind));
                    fprintf(output, "%.*s:\n", id->len, contents + id->index);
                    fprintf(output, "\t.%s\t%.*s\n", size_mnemonic(variable_size(type.kind)), value->len, contents + value->index);
                    fprintf(output, "\t.size\t%.*s, %i\n", id->len, contents + id->index, bytes);
                }
                break;
                case string:
                    char *buf = malloc(token->len + 1);
                memcpy(buf, contents + token->index, token->len);
                buf[token->len] = '\0';
                const int index = strlist_indexof_val(strLiterals, buf);
                if (index != -1)
                {
                    free(buf);
                } else
                {
                    fprintf(output, ".L.STR%i:\n\t.asciz\t%s\n\t.size\t.L.STR%i, %i\n", strLiterals->len, buf, strLiterals->len, token->len - 1);
                    strlist_add(strLiterals, buf);
                }
                break;
            }
        default:
            break;
        }
        token = token->next;
    }
    return success();
}
