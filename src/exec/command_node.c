#include <errno.h>

#include "exec.h"
#include "utils.h"

// char **args = [ "test", "salut", NULL ]
struct ast_node *new_command_node(char **args)
{
    struct ast_node *ret = xmalloc(sizeof(struct ast_node));
    size_t arr_len = array_len(args);
    ret->type = NODE_COMMAND;
    ret->data.ast_command.args = xcalloc(arr_len + 1, sizeof(char *));
    for (size_t i = 0; args[i] != NULL; ++i)
    {
        ret->data.ast_command.args[i] = strdup(args[i]);
        free(args[i]);
    }
    free(args);
    ret->data.ast_command.args[arr_len] = NULL;
    return ret;
}

void free_command_node(struct ast_node *node)
{
    if (node->type != NODE_COMMAND)
    {
        fprintf(stderr, "trying to free node of wrong type (command)");
        return;
    }

    if (node != NULL)
    {
        if (node->data.ast_command.args != NULL)
        {
            for (size_t i = 0; node->data.ast_command.args[i] != NULL; ++i)
                free(node->data.ast_command.args[i]);
            free(node->data.ast_command.args);
        }
        free(node);
    }
}

int exec_command_node(struct ast_node *node)
{
    if (node->type != NODE_COMMAND)
    {
        fprintf(stderr, "trying to exec node of wrong type (command)");
        return -1;
    }
    int status;

    if (strcmp(node->data.ast_command.args[0], "echo") == 0)
    {
        status = echo(node);
        return status;
    }
    if (strcmp(node->data.ast_command.args[0], "exit") == 0)
    {
        int return_status;
        if (node->data.ast_command.args[1] == NULL)
            return_status = 0;
        else
            return_status = atoi(node->data.ast_command.args[1]);
        exit(return_status);
    }

    pid_t child = fork();
    if (child == -1)
    {
        fprintf(stderr, "Could not fork process\n");
        abort();
    }

    if (child == 0)
    {
        // child process
        execvp(node->data.ast_command.args[0], node->data.ast_command.args);
        fprintf(stderr, "Could not execute %s\n",
                node->data.ast_command.args[0]);
        exit(-1);
        // return -1;
    }
    else
    {
        // parent process
        wait(&status); // wait for child
    }

    return status;
}
