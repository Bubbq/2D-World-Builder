#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdlib.h>

typedef void (*print_content_funct)(void*);
typedef void (*content_free_funct)(void*);

typedef struct Node 
{
    size_t size;
    void* content;
    struct Node* prev;
    struct Node* next;
    content_free_funct free_funct;
    print_content_funct print_funct;
} Node;

Node* node_init(void* data, const size_t data_size, const print_content_funct print_funct, const content_free_funct free_funct);
void node_free(Node* node);
void node_print(const Node* node);

typedef struct
{
    size_t count;
    Node* head; 
    Node* tail; 
} List;

List list_init();
void list_free(List* list);
void list_print(const List* list);
Node* list_dequeue(List* list);
void list_append(List* list, Node* node);

#endif