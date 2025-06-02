#ifndef BTREE_H
#define BTREE_H

#include <stddef.h>

typedef struct BtreeNode {
    size_t n_keys;
    size_t n_children;
    int* keys;
    char** vals;
    struct BtreeNode* children[];
} BtreeNode;

typedef struct Btree {
    size_t max_keys;
    struct BtreeNode* root;
} Btree;

void print_node(BtreeNode* node); //For debugging
void print_tree(Btree* tree);

Btree make_btree(size_t max_keys);

void btree_free(Btree* tree);

char* btree_search(Btree* tree, int key);
void btree_insert(Btree* tree, int key, char* val);
void btree_delete(Btree* tree, int key);

// void btree_print(Btree* tree);

#endif //BTREE_H