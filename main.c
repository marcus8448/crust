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

// https://stackoverflow.com/questions/1597007/creating-c-macro-with-and-line-token-concatenation-with-positioning-macr
#define __preprocess_concat_internal(x, y) x ## y
#define __preprocess_concat(x, y) __preprocess_concat_internal(x, y)

#define forward_err(function) Result __preprocess_concat(result, __LINE__) = function; if (!successful(__preprocess_concat(result, __LINE__))) { return __preprocess_concat(result, __LINE__); }

typedef struct
{
    const char *filename;
    char* contents;
    size_t len;
} FileData;

char* data_copy(const FileData *data, const Token *token)
{
    char *alloc = malloc(token->len + 1);
    memcpy(alloc, data->contents + token->index, token->len);
    alloc[token->len] = '\0';
    return alloc;
}

int filedata_init(FileData *data, const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL) return 2;
    fseek(file, 0, SEEK_END);
    const long len = ftell(file);
    if (len < 0 || len > 1024 * 64)
    {
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

Result parse_function_declaration(const FileData *data, StrList *args, const Token **token);
bool parse_arguments(const TokenList* list, StrList *args, size_t* index, bool open);
Result parse_root(const FileData *data, const TokenList *list, FILE* output);

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
        filedata_init(&files[i - 1], argv[i]);
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

    FILE* output = fopen("output.asm", "wb");
    for (int i = 0; i < argc - 1; i++)
    {
        const Token *token = tokens[i].head;
        while (token != NULL)
        {
            puts(token_name(token->type));
            token = token->next;
        }

        const Result result = preprocess_globals(files[i].contents, tokens[i].head, &strLiterals[i], output);
        if (!successful(result))
        {
            fflush(output);
            print_error("Preprocessing", result, files[i]);
            exit(1);
        }
    }

    for (int i = 0; i < argc - 1; i++)
    {
        const Result result = parse_root(&files[i], &tokens[i], output);
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

Result parse_scope(const FileData* data, const Token **token, Registers *registers, FILE* output);

Result parse_function_declaration(const FileData *data, StrList* args, const Token** token)
{
    *token = (*token)->next;
    token_matches(*token, opening_paren);

    while ((*token = (*token)->next)->type != closing_paren)
    {
        if ((*token)->type == identifier)
        {
            strlist_add(args, data_copy(data, *token));
        }

        if ((*token = (*token)->next)->type == closing_paren) return success();
        token_matches_ext(*token, comma, ", or )");
    }
    return success();
}

Result parse_root(const FileData *data, const TokenList *list, FILE* output)
{
    Registers registers;
    register_init(&registers);
    const Token *token = list->head;
    while (token != NULL)
    {
        switch (token->type)
        {
        case keyword_define:
            token = token->next;
            const Token* id = token;
            token_matches(id, identifier);

            StrList arguments;
            strlist_init(&arguments, 2);

            forward_err(parse_function_declaration(data, &arguments, &token));

            fprintf(output, "%.*s", id->len, data->contents + id->index);
            fputs(":\npush rbp # save frame pointer\n"
                  "mov rbp, rsp # update frame pointer for this function\n", output);
            for (int i = 0; i < arguments.len; i++)
            {
                fprintf(output, "pop %s # get variable %s\n", register_get_mnemonic(&registers, register_claim(&registers, arguments.array[i], s64)), arguments.array[i]);
            }
            parse_scope(data, &token, &registers, output);
            fputs("pop rbp # restore frame pointer\n"
                  "ret\n", output);
            break;
        case keyword_var:
            token = token->next;
            token_matches(token, identifier);
            token_seek_until(token, semicolon); // already processed
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

void parse_statement(const TokenList* list, size_t* index, const Registers* registers, int reg, FILE* output);
void call_function(char* name, const StrList *args, const Registers* registers, FILE* output);

void call_function(char* name, const StrList *args, const Registers* registers, FILE* output)
{
    register_push_all(registers, output);
    for (int i = args->len - 1; i >= 0;--i)
    {
        const char* mnemonic = register_get_mnemonic_v(registers, args->array[i]);
        fprintf(output, "push %s # push argument %s onto stack\n", mnemonic, args->array[i]);
    }
    fprintf(output, "call %s # call function %s\n", name, name);
    register_pop_all(registers, output);
}

Result parse_scope(const FileData* data, const Token **token, Registers *registers, FILE* output)
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
            // *token = (*token)->next;
            // token_matches(*token, identifier);
            //
            // switch ((*token = (*token)->next)->type)
            // {
            // case equals_assign:
            //     int reg = register_claim(registers, name, s64);
            //     parse_statement(list, index, registers, reg, output);
            //     break;
            // case opening_paren:
            //     StrList args;
            //     strlist_init(&args, 2);
            //     if (!parse_arguments(list, &args, index, true))
            //     {
            //         puts("Failed to parse method arguments");
            //         return false;
            //     }
            //     call_function(name, &args, registers, output);
            //     break;
            // default:
            //     return failure(*token, "expected assignment or function call");
            // }
        case identifier:
            // char* name = token.identifier;
            //
            // token = list->array[(*index)++];
            // if (equals_assign == token->type)
            // {
            //     int reg = register_claim(registers, name, s64);
            //     parse_statement(list, index, registers, reg, output);
            // } else if (token->type == opening_paren)
            // {
            //     StrList args;
            //     strlist_init(&args, 2);
            //     if (!parse_arguments(list, &args, index, true))
            //     {
            //         puts("Failed to parse method arguments");
            //         return false;
            //     }
            //     call_function(name, &args, registers, output);
            //
            //     token = list->array[(*index)++];
            //     if (token->type != semicolon)
            //     {
            //         printf("expected statment end, found %s", token_name(token->type));
            //     }
            // } else
            // {
            //     printf("Expected identifier, found %s!\n", token_name(token->type));
            //     return false;
            // }
            // break;
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

// void parse_statement(const TokenList* list, size_t* index, const Registers* registers, int reg, FILE* output)
// {
//     Token left = {.token = eof};
//     Token op = {.token = eof};
//     Token token = list->array[(*index)++];
//
//     if (token.token == constant)
//     {
//         if (left.token == eof)
//         {
//             left = token;
//         }
//     } else if (token.token == identifier)
//     {
//         char* identifier = token.identifier;
//         token = list->array[(*index)++];
//         if (token.token == opening_paren)
//         {
//             StrList args;
//             strlist_init(&args, 2);
//             if (!parse_arguments(list, &args, index, true))
//             {
//                 printf("failed to parse args");
//                 return;
//             }
//             call_function(identifier, &args, registers, output);
//             printf("mov %s, %s", register_get_mnemonic(registers, reg), "rbx");
//         }
//     } else if (token.token == semicolon)
//     {
//         return;
//     } else
//     {
//         printf("invalid ststnebt: %s\n", token_name(token->token));
//         return;
//     }
// }
//
// bool parse_arguments(const TokenList* list, StrList *args, size_t* index, bool open)
// {
//     if (!open)
//     {
//         Token peek = list->array[(*index)++];
//         if (peek.token != opening_paren)
//         {
//             printf("Expected '(', found %s!\n", token_names[peek.token]);
//             return false;
//         }
//     }
//
//     while (*index < list->len)
//     {
//         Token token = list->array[(*index)++];
//         if (token.token == closing_paren)
//         {
//             return true;
//         }
//
//         if (token.token == identifier)
//         {
//             strlist_add(args, token.identifier);
//         } else
//         {
//             printf("Expected identifier, found %s!\n", token_name(token->token));
//             return false;
//         }
//
//         token = list->array[(*index)++];
//         if (token.token == closing_paren)
//         {
//             return true;
//         }
//
//         if (token.token != comma)
//         {
//             printf("Expected ',' or ')', found %s!\n", token_name(token->token));
//             return false;
//         }
//     }
//     assert(false);
//     return false;
// }
