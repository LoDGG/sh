#include "exec.h"
#include "utils.h"

struct ast_node *new_if_node()
{
    struct ast_node *ret = xmalloc(sizeof(struct ast_node));
    ret->type = NODE_IF;
    ret->data.ast_if.body_list = NULL;
    ret->data.ast_if.condition = NULL;
    ret->data.ast_if.elif = NULL;

    return ret;
}

void free_if_node(struct ast_node *node)
{
    if (node->type != NODE_IF)
    {
        fprintf(stderr, "trying to free non if node\n");
        return;
    }

    if (node != NULL)
    {
        if (node->data.ast_if.body_list != NULL)
            free_node(node->data.ast_if.body_list);
        if (node->data.ast_if.condition != NULL)
            free_node(node->data.ast_if.condition);
        if (node->data.ast_if.elif != NULL)
            free_node(node->data.ast_if.elif);
        free(node);
    }
}

int exec_if_node(struct ast_node *node)
{
    if (node->type != NODE_IF)
    {
        fprintf(stderr, "trying to execute non if node\n");
        return -1;
    }

    if (node->data.ast_if.body_list == NULL)
    {
        fprintf(stderr, "if/elif/else missing body\n");
        return -1;
    }

    if (node->data.ast_if.condition != NULL)
    {
        // if or elif case
        int return_status = exec_node(node->data.ast_if.condition);
        if (return_status == 0) // condition was true
        {
            int status = exec_node(node->data.ast_if.body_list);
            return status;
        }
        if (node->data.ast_if.elif != NULL)
        {
            int status = exec_node(node->data.ast_if.elif);
            return status;
        }
        return 0;
    }
    // else case
    int status = exec_node(node->data.ast_if.body_list);
    return status;
}
