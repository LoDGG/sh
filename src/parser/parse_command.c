#include "parser.h"

// command grammar:
//   simple_command
// | shell_command
enum parser_status parse_command(struct ast_node **ast, struct lexer *input)
{
    struct token *tok = lexer_peek(input);
    if (tok->type == TOKEN_ERROR)
        return PARSER_ERROR;

    enum parser_status stat = parser_simple_command(ast, input);
    if (stat != PARSER_OK) // either propagate error
        return stat; // or we found a simple command and stop there

    struct ast_node *com = NULL;
    stat = parse_shell_command(&com, input);
    if (stat != PARSER_FOUND)
        return stat; // we did not find anything, and parse_command did not find
                     // anything

    // now parse redirections
    *ast = new_redirec_list_node();
    (*ast)->data.ast_redirec_list.child = com;

    while (stat == PARSER_FOUND)
    {
        struct redirection *red = NULL;
        stat = parse_redirection(&red, input);
        if (stat == PARSER_ERROR)
        {
            free_node(*ast);
            return PARSER_ERROR;
        }
        if (stat == PARSER_OK) // no more redirections
            return PARSER_FOUND; // we had a command and maybe some redirections
                                 // : we found something

        push_redirec_list_node(*ast, red);
    }

    return PARSER_FOUND; // default, shouldn't come here
}

enum parser_status parse_assignment(struct ast_node **ast, struct lexer *input)
{
    struct token *tok = lexer_peek(input);
    if (tok->type == TOKEN_ERROR)
        return PARSER_ERROR;

    *ast = new_assignment_node(tok->value);
    tok = lexer_pop(input);
    if (tok->type == TOKEN_ERROR)
    {
        free_node(*ast);
        return PARSER_ERROR;
    }
    if (tok->type == TOKEN_WORDS || tok->type == TOKEN_EXPAND
        || tok->type == TOKEN_SINGLEQUOTE)
    {
        (*ast)->data.ast_assignment.tok = token_dup(tok);
        lexer_pop(input);
        return PARSER_FOUND;
    }
    else
    {
        free_node(*ast);
        return PARSER_ERROR;
    }
}

// Grammar :
//   (prefix)+
// | (prefix)* (element)+
// For us, is there is a assignment, ignore the rest, else proceed as normal
enum parser_status parser_simple_command(struct ast_node **ast,
                                         struct lexer *input)
{
    struct token *tok = lexer_peek(input);
    if (tok->type == TOKEN_ERROR)
        return PARSER_ERROR;

    if (tok->type == TOKEN_ASSIGNMENT_WORD)
    {
        return parse_assignment(ast, input);
    }
    *ast = new_redirec_list_node();

    size_t capacity = 5;
    size_t len = 0;
    struct token **args = xmalloc(sizeof(struct token *) * capacity);

    while (1)
    {
        if (tok->type == TOKEN_WORDS || tok->type == TOKEN_EXPAND)
        {
            if (len == capacity)
            {
                capacity *= 2;
                args = xrealloc(args, sizeof(struct token *) * capacity);
            }
            args[len] = token_dup(tok);
            len++;
        }
        else
        {
            struct redirection *red = NULL;
            enum parser_status stat = parse_redirection(&red, input);

            if (stat == PARSER_ERROR)
            {
                // free args
                for (size_t i = 0; i < len; ++i)
                    token_free(args[i]);
                free(args);
                // free redirection node
                free_node(*ast);
                return PARSER_ERROR;
            }
            if (stat == PARSER_FOUND)
            {
                // add redirection to redirection list
                push_redirec_list_node(*ast, red);
            }
            else // PARSER_OK
            {
                // we had no TOKEN_WORD and no redirection, we're out of here
                if (len == 0)
                {
                    // no word and no redirection at all
                    free_node(*ast);
                    *ast = NULL;
                    free(args);
                    return PARSER_OK;
                }

                struct ast_node *c = NULL;

                capacity = len + 1;
                args = xrealloc(args, sizeof(struct token *) * capacity);
                args[len] = NULL;
                // new_command_node frees args array, don't need to do it
                // ourselves
                c = new_command_node(args);

                (*ast)->data.ast_redirec_list.child = c;
                return PARSER_FOUND;
            }
        }
        tok = lexer_pop_ignore_keyword(input);
        if (tok->type == TOKEN_ERROR)
        {
            for (size_t i = 0; i < len; ++i)
                token_free(args[i]);
            free(args);
            // free redirection node
            free_node(*ast);
            return PARSER_ERROR;
        }
    }

    return PARSER_OK;
}

enum parser_status parse_shell_command(struct ast_node **ast,
                                       struct lexer *input)
{
    struct token *tok = lexer_peek(input);
    if (tok->type == TOKEN_ERROR)
        return PARSER_ERROR;

    if (tok->type == TOKEN_IF)
    {
        return parse_rule_if(ast, input);
    }
    if (tok->type == TOKEN_WHILE)
    {
        return parse_rule_while(ast, input);
    }
    if (tok->type == TOKEN_UNTIL)
    {
        return parse_rule_until(ast, input);
    }

    // printf(stderr, "Unable to parse shell command\n");
    return PARSER_OK;
}
