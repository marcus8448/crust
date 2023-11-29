#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "ast.h"
#include "list.h"
#include "preprocess.h"
#include "register.h"
#include "token.h"

// strdup is fine in C23
#ifdef _MSC_VER
#define strdup _strdup
#endif

typedef struct
{
    const char *filename;
    char* contents;
    size_t len;
} FileData;

int filedata_init(FileData *data, const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL) return 2;
    fseek(file, 0, SEEK_END);
    const long len = ftell(file);
    if (len < 0 || len > 1024 * 64)
    {
        fclose(file);
        puts("file too large");
        return 1;
    }
    char* contents = malloc(len + 1);
    rewind(file);
    fread(contents, len, 1, file);
    fclose(file);
    contents[len] = '\0';

    data->filename = filename;
    data->contents = contents;
    data->len = len;
    return 0;
}

bool parse_arguments(const TokenList* list, StrList *args, size_t* index, bool open);
Result parse_root(const FileData *data, const TokenList *list, VarList *globals, FunctionList *functions, FILE* output);

void print_error(const char* section, const Result result, const FileData file)
{
    int line = 1;
    int lineStart = 0;
    for (int j = 0; j < result.failure->index; j++)
    {
        if (j < result.failure->index && file.contents[j] == '\n')
        {
            lineStart = j + 1;
            line++;
        }
    }
    int lineLen = 0;
    for (int j = lineStart; j < file.len && file.contents[j] != '\n'; j++)
    {
        lineLen = j - lineStart + 1;
    }
    printf("%s error at %s[%i:%i]\n", section, file.filename, line, result.failure->index - lineStart);
    printf("%.*s\n"
           "%*s%.*s\n", lineLen, file.contents + lineStart, result.failure->index - lineStart, "", result.failure->len,
           "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    printf("%*s'%.*s': %s\n", result.failure->index - lineStart, "", result.failure->len, file.contents + result.failure->index,
           result.reason);
}

int main(const int argc, char** argv)
{
    if (argc < 2)
    {
        if (argv[0] != NULL)
        {
            printf("Usage: %s <filenames...>\n", argv[0]);
        }
        else
        {
            puts("Usage: clamor <filenames..>");
        }
        return 1;
    }

    FileData *files = malloc(sizeof(FileData) * (argc - 1));
    TokenList *tokens = malloc(sizeof(TokenList) * (argc - 1));
    StrList *strLiterals = malloc(sizeof(StrList) * (argc - 1));

    for (int i = 1; i < argc; i++)
    {
        if (filedata_init(&files[i - 1], argv[i]) != 0)
        {
            exit(-1);
        }
        tokens_init(&tokens[i - 1]);
        strlist_init(&strLiterals[i - 1], 1);
    }

    for (int i = 0; i < argc - 1; i++)
    {
        if (!tokenize(files[i].contents, files[i].len, &tokens[i]))
        {
            exit(3);
        }
    }

    FunctionList functions;
    VarList globals;
    functionlist_init(&functions, 2);
    varlist_init(&globals, 2);

    FILE* output = fopen("output.asm", "wb");
    for (int i = 0; i < argc - 1; i++)
    {
        const Token *token = tokens[i].head;
        while (token != NULL)
        {
            puts(token_name(token->type));
            token = token->next;
        }

        const Result result = preprocess_globals(files[i].contents, tokens[i].head, &strLiterals[i], &globals, &functions, output);
        if (!successful(result))
        {
            fflush(output);
            print_error("Preprocessing", result, files[i]);
            exit(1);
        }
    }

    fputc('\n', output);

    for (int i = 0; i < argc - 1; i++)
    {
        const Result result = parse_root(&files[i], &tokens[i], &globals, &functions, output);
        if (!successful(result))
        {
            fflush(output);
            print_error("Parsing", result, files[i]);
            exit(1);
        }
    }
    fclose(output);

    //fixme
    free(files);
    free(tokens);
    free(strLiterals);

    return 0;
}

Result parse_function(const char *contents, const Token **token, const Function *function, const VarList *globals, const FunctionList *functions, FILE* output)
{
    StackFrame frame;
    stackframe_init(&frame, NULL);
    stackframe_load_arguments(&frame, function);

    return success();
}

Result parse_scope(const char* contents, Token** token, StackFrame* frame, VarList* globals, FunctionList* functions, FILE* output);

Result parse_root(const FileData *data, const TokenList *list, VarList *globals, FunctionList *functions, FILE* output)
{
    const Token *token = list->head;
    while (token != NULL)
    {
        switch (token->type)
        {
        case keyword_extern:
            token = token->next;
            token_seek_until(token, semicolon); // already processed
            break;
        case keyword_fn:
            token = token->next;
            const Token* id = token;
            token_matches(id, identifier);
            char* copy = token_copy(token, data->contents);
            int indexof = functionlist_indexof(functions, copy);
            free(copy);
            assert(indexof != -1);
            Function *function = &functions->array[indexof];
            token_seek_until(token, opening_curly_brace);

            StackFrame frame;
            stackframe_init(&frame, NULL);

            fprintf(output, "%.*s:\n", id->len, data->contents + id->index);
            fputs("pushq %rbp # save frame pointer\n"
                  "movq %rsp, %rbp # update frame pointer for this function\n", output);

            stackframe_load_arguments(&frame, function);
            forward_err(parse_scope(data->contents, &token, &frame, globals, functions, output));

            fputs("popq %rbp # restore frame pointer\n"
                  "retq\n", output);

            stackframe_free(&frame, output);
            break;
        case keyword_var:
            token = token->next;
            token_matches(token, identifier);
            token_seek_until(token, semicolon); // already processed
            break;
        case semicolon:
            break;
        case eof:
            return success();
        default:
            return failure(token, "expected global or function");
        }
        token = token->next;
    }
    // unreachable
    abort();
}

Result invoke_function(const char* contents, Token** token, StackFrame* frame, VarList* globals, FunctionList* functions, Function function, FILE* file);

StackAllocation *parse_statement(const char* contents, Token** token, StackFrame* frame, VarList* globals, FunctionList* functions, FILE* file);

Result parse_scope(const char* contents, Token** token, StackFrame* frame, VarList* globals, FunctionList* functions, FILE* output)
{
    *token = (*token)->next;
    token_matches(*token, opening_curly_brace);

    while ((*token)->next != NULL)
    {
        switch ((*token = (*token)->next)->type)
        {
        case closing_curly_brace:
            return success();
        case keyword_var:
            {
                Variable variable;
                *token = (*token)->next;
                token_matches(*token, identifier);
                variable.name = token_copy(*token, contents);

                *token = (*token)->next;
                token_matches(*token, colon);

                Type type;
                forward_err(parse_type(contents, token, &type));
                variable.type = type;

                *token = (*token)->next;
                if ((*token)->type == semicolon)
                {
                    typekind_size(type.kind);
                    stackframe_allocate(frame, variable);
                } else
                {
                    token_matches(*token, equals_assign);
                    const char reg = parse_statement(contents, token, frame, globals, functions, output);
                    stackframe_claim_or_copy_from(frame, variable, reg, output);
                }
                break;
            }
        case identifier:
            char* name = token_copy(*token, contents);

            *token = (*token)->next;

            switch ((*token)->type)
            {
            case equals_assign:
                {
                    const char reg = parse_statement(contents, token, frame, globals, functions, output);
                    StackAllocation* ref = stackframe_get_name(frame, name);
                    stackframe_set_or_copy_register(frame, ref, reg, output);
                }
                break;
            case opening_paren:
                {
                    const int indexof = functionlist_indexof(functions, name);
                    if (indexof == -1)
                    {
                        return failure(*token, "unknown function");
                    }
                    forward_err(invoke_function(contents, token, frame, globals, functions, functions->array[indexof], output));
                }
                break;
            default:
                token_matches_ext(*token, eof, "invalid id sx");
                break;
            }
            free(name);
            break;
        case cf_if:
            // token = list->array[(*index)++];
            // if (token->type != opening_paren)
            // {
            //     printf("expected (, found %s", token_name(token->type));
            //     return false;
            // }
            //
            // int reg = register_claim(registers, "@@IF!", s64);
            // parse_statement(list, index, registers, reg, output);
            //
            // register_release(registers, reg);
            break;
        case cf_else:
            // *token = (*token)->next;
            // token_matches(*token, opening_curly_brace);

            break;
        case cf_return:
            // const int reg = register_claim(registers, "@@RETURN!", s64);
            // parse_statement(list, index, registers, reg, output);
            // fprintf(output, "mov %s, %s # store return value in correct register\n", "rbx", register_get_mnemonic(registers, reg));
            // fprintf(output, "pop ebp # restore frame\n"
            //                 "ret # return\n");
            // register_release(registers, reg);
            break;
        case semicolon:
            continue;
        default:
            return failure(*token, "expected start of statement");
        }
    }
    // unreachable
    abort();
}

Result invoke_function(const char* contents, Token** token, StackFrame* frame, VarList* globals, FunctionList* functions, const Function function,
                       FILE* file)
{
    token_matches(*token, opening_paren);
    *token = (*token)->next;
    for (int i = 0; i < frame->allocations.len; ++i)
    {
        switch (frame->allocations.array[i].reg)
        {
        case rax:
        case rdi:
        case rsi:
        case rdx:
        case rcx:
        case r8:
        case r9:
        case r10:
        case r11:
            stackframe_moveto_stack(frame, &frame->allocations.array[i], file);
            break;
        default:
            break;
        }
    }

    for (int i = 0; i < function.arguments.len; i++)
    {
        char statement = parse_statement(contents, token, frame, globals, functions, file);
        if (i < 6)
        {

        } else
        {

        }

        if (i + 1 == function.arguments.len)
        {
            token_matches(*token, closing_paren);
        } else
        {
            token_matches(*token, comma);
        }
    }
    fprintf(file, "call %s\n", function.name);
    return success();
}

StackAllocation* parse_statement(const char* contents, Token** token, StackFrame* frame, VarList* globals,
                                 FunctionList* functions, FILE* file)
{
    *token = (*token)->next;
    return r8;
}
