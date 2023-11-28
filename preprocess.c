#include "preprocess.h"

#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "list.h"
#include "types.h"

Result parse_function_declaration(const char* contents, const Token *&token, Function *function)
{
    token = token->next;
    token_matches(token, identifier);
    function->name = token_copy(token, contents);

    token = token->next;
    token_matches(token, opening_paren);

    while ((token = token->next)->type != closing_paren)
    {
        if (token->type == identifier)
        {
            Variable argument;
            argument.name = token_copy(token, contents);

            token = token->next;
            token_matches(token, colon);

            forward_err(parse_type(contents, &token, &argument.type));
            varlist_add(&function->arguments, argument);
        }

        token = token->next;
        if (token->type == closing_paren) break;
        token_matches_ext(token, comma, ", or )");
    }
    return success();
}

Result preprocess_globals(char* contents, const Token *token, StrList *strLiterals, VarList *variables, FunctionList *functions, FILE* output)
{
    while (token != NULL)
    {
        switch (token->type)
        {
        case keyword_fn:
            {
                Function function;
                function_init(&function);
                forward_err(parse_function_declaration(contents, token, &function));

                if (functionlist_indexof(functions, function.name) != -1)
                {
                    return failure(token, "redefinition of function");
                }

                functionlist_add(functions, function);
                break;
            }
        case keyword_var:
            {
                Variable variable;
                token = token->next;
                token_matches(token, identifier);
                variable.name = token_copy(token, contents);

                if (varlist_indexof(variables, variable.name) != -1)
                {
                    return failure(token, "redefinition of global variable");
                }

                token = token->next;
                token_matches(token, colon);

                Type type;
                forward_err(parse_type(contents, &token, &type));
                variable.type = type;

                varlist_add(variables, variable);

                token = token->next;
                if (token->type == semicolon)
                {
                    typekind_size(type.kind);
                    fprintf(output, "%s:\n", variable.name);
                    const int bytes = size_bytes(typekind_size(type.kind));
                    fprintf(output, "\t.zero\t%i\n", bytes);
                    fprintf(output,  "\t.size\t%s, %i\n", variable.name, bytes);
                } else
                {
                    token_matches(token, equals_assign);
                    token = token->next;
                    const Token *value = token;
                    token_matches(token, constant);
                    token = token->next;
                    token_matches(token, semicolon);
                    const int bytes = size_bytes(typekind_size(type.kind));
                    fprintf(output, "%s:\n", variable.name);
                    fprintf(output, "\t.%s\t%.*s\n", size_mnemonic(typekind_size(type.kind)), value->len, contents + value->index);
                    fprintf(output, "\t.size\t%s, %i\n", variable.name, bytes);
                }
                break;
                case string:
                    char *buf = malloc(token->len + 1);
                memcpy(buf, contents + token->index, token->len);
                buf[token->len] = '\0';
                const int index = strlist_indexof(strLiterals, buf);
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
        case keyword_extern:
            {
                token = token->next;
                switch (token->type)
                {
                case keyword_fn:
                    token = token->next;
                    token_matches(token, identifier);

                    break;
                case keyword_var:
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
        token = token->next;
    }
    return success();
}
