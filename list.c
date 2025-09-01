#include "list.h"

Node* node_init(void* content, const size_t content_size, const print_content_funct print_funct, const content_free_funct free_funct)
{
    if (!content)
        return NULL;

    Node* node = malloc(sizeof(Node));
    if (!node) 
        return NULL;
    
    node->content = content;
    node->size = content_size;

    node->free_funct = free_funct;
    node->print_funct = print_funct;
    
    node->prev = node->next = NULL;
    
    return node;
}

void node_free(Node* node)
{
    if (!node)
        return;

    if (node->content && node->free_funct)
        node->free_funct(node->content);

    free(node); node = NULL;
}

void node_print(const Node* node)
{
    if (node && node->print_funct)
        node->print_funct(node->content);
}

List list_init()
{
    List list = {
        .count = 0,
        .head = NULL,
        .tail = NULL,
    };
    
    return list;
}

void list_free(List* list)
{
    if (!list)
        return;

    while (list->head) 
        node_free(list_dequeue(list));

    list->head = list->tail = NULL;
}

void list_print(const List* list)
{
    if (!list)
        return;

    for (Node* node = list->head; node; node = node->next)
        node_print(node);
}

void list_append(List* list, Node* node)
{
    if (!list || !node)
        return;

    node->prev = node->next = NULL;

    if (!list->head) 
        list->head = list->tail = node;

    else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }

    list->count++;
}

Node* list_dequeue(List* list)
{
    if (!list)
        return NULL;

    Node* detached = list->head;

    list->head = list->head->next;
    if (!list->head) 
        list->tail = NULL;

    detached->prev = detached->next = NULL;

    list->count--;
    
    return detached;
}