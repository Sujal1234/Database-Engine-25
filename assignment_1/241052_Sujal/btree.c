#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "btree.h"

static void print_entry(BtreeNode* node, size_t i);

static void free_subtree(BtreeNode* root);
static void free_node(BtreeNode* node);

static BtreeNode* btree_make_node(Btree* tree, int key, char* val);

static char* search(BtreeNode* root, int key);
static void move_entry(BtreeNode* src, size_t src_idx, BtreeNode* dest, size_t dest_idx);
static void node_split(BtreeNode* root, Btree* tree, BtreeNode* new_node, size_t idx);
static BtreeNode* node_insert(BtreeNode* root, Btree* tree, int key, char* val);

static BtreeNode* get_previous_key(BtreeNode* root, int idx); //Gets largest key smaller than given key

static void node_rebalance(Btree* tree, BtreeNode* root, int key);
static int recursive_delete(Btree* tree, BtreeNode* root, int key);


static void print_entry(BtreeNode* node, size_t i){
    printf("[%d,%s] ", node->keys[i], node->vals[i]);
}

void print_node(BtreeNode* node){
    if(!node){
        return;
    }
    printf("{ ");
    for (size_t i = 0; i < node->n_keys; i++)
    {
        print_entry(node, i);
    }
    printf("} ");
}

//Returns 0 if the depth is greater than the height of the tree 
static int print_depth(BtreeNode* root, size_t depth){
    if(depth == 0){
        print_node(root);
        return 1;
    }

    if(root->n_children == 0){
        return 0;
    }

    for (size_t i = 0; i < root->n_children; i++)
    {
        if(!print_depth(root->children[i], depth - 1)){
            return 0;
        }
    }
    return 1;
}

void print_tree(Btree* tree){
    size_t depth = 0;
    while(print_depth(tree->root, depth)){
        printf("\n");
        depth++;
    }
}

Btree make_btree(size_t max_keys){
    if(max_keys < 1){
        max_keys = 1;
    }
    Btree tree = {
        .max_keys = max_keys,
        .root = NULL
    };
    return tree;
}

static BtreeNode* btree_make_node(Btree* tree, int key, char* val) {
    //Number of children = tree->max_keys + 1
    BtreeNode* node = malloc(sizeof(BtreeNode) + sizeof(BtreeNode*) * (tree->max_keys+1));
    if(!node){
        perror("Node creation");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < tree->max_keys+1; i++)
    {
        node->children[i] = NULL;
    }
    
    node->keys = malloc(sizeof(int) * tree->max_keys);
    node->vals = malloc(sizeof(char*) * tree->max_keys);
    node->n_keys = 1;
    node->n_children = 0;

    node->keys[0] = key;
    node->vals[0] = val;
    return node;
}

void btree_free(Btree* tree){
    free_subtree(tree->root);
}

static void free_subtree(BtreeNode* root){
    if(!root){
        return;
    }
    free(root->keys);
    free(root->vals);
    for (size_t i = 0; i < root->n_children; i++)
    {
        free_subtree(root->children[i]);
    }
    free(root);
    //root->children is a FAM so it need not be freed
}

static void free_node(BtreeNode* node){
    free(node->keys);
    free(node->vals);
    free(node);
}

char* btree_search(Btree* tree, int key){
    return search(tree->root, key);
}

static char* search(BtreeNode* root, int key){
    if(!root) return NULL;
    size_t i = 0;
    for(; i < root->n_keys && key >= root->keys[i]; i++){
        if(root->keys[i] == key){
            return root->vals[i];
        }
    }

    if(root->n_children){
        return search(root->children[i], key);
    }
    else{
        //Not found
        return NULL;
    }
}

void btree_insert(Btree* tree, int key, char* val){
    if(!tree->root){
        tree->root = btree_make_node(tree, key, val);
        return;
    }
    BtreeNode* new_node = node_insert(tree->root, tree, key, val);
    if(new_node){
        tree->root = new_node;
    }
}

static void move_entry(BtreeNode* src, size_t src_idx, BtreeNode* dest, size_t dest_idx){
    dest->keys[dest_idx] = src->keys[src_idx];
    dest->vals[dest_idx] = src->vals[src_idx];
}

//new_node contains just the key and value to be inserted while splitting
//idx is the index where the new_node belongs before splitting
//Sets new_node as the pushed up node
static void node_split(BtreeNode* root, Btree* tree, BtreeNode* new_node, size_t idx){
    size_t min_keys = tree->max_keys / 2;

    BtreeNode* temp = btree_make_node(tree, -1, NULL);
    temp->children[0] = new_node->children[0];
    temp->children[1] = new_node->children[1];
    
    if(idx == min_keys){
        //New key itself is pushed up
        move_entry(new_node, 0, temp, 0);
    }
    else if(idx < min_keys){
        move_entry(root, min_keys - 1, temp, 0); //Move median into temp

        //Shift entries to the right
        for (size_t i = min_keys - 1; i > idx; i--)
        {
            move_entry(root, i - 1, root, i);
        }
        //Insert new node
        move_entry(new_node, 0, root, idx);
    }
    else{
        move_entry(root, min_keys, temp, 0); //Move median into temp
        
        //Shift entries to the left
        for (size_t i = idx - 1; i > min_keys; i--)
        {
            move_entry(root, i, root, i-1);
        }
        //Insert new node
        move_entry(new_node, 0, root, idx - 1); //The new entry ends up at idx-1 since the median to the left is gone
    }

    move_entry(temp, 0, new_node, 0); //Set new node to the pushed up node

    //Split old node into two children
    new_node->children[0] = root;
    new_node->children[1] = btree_make_node(tree, INT_MAX, NULL);

    //Shift the right half of the entries into the second child
    for (size_t i = min_keys; i < tree->max_keys; i++)
    {
        move_entry(root, i, new_node->children[1], i - min_keys);
    }
    
    if(root->n_children){
        if(idx < min_keys){
            //Move children to the right node
            for (size_t i = min_keys; i < tree->max_keys + 1; i++)
            {
                new_node->children[1]->children[i - min_keys] = root->children[i];
            }
            //Create gap at idx+1 to insert the actual left child of new_node
            for (size_t i = min_keys; i < idx+1; i--)
            {
                root->children[i] = root->children[i-1];
            }

            //Insert last child of left node
            root->children[idx + 1] = temp->children[1]; // > new_node but < pushed up node
        }
        // idx >= min_keys
        else{
            //Move children to the right node
            for (size_t i = min_keys + 1; i < tree->max_keys + 1; i++)
            {
                new_node->children[1]->children[i - min_keys - 1] = root->children[i];
            }

            //Create a gap at idx - min_keys + 1
            for (size_t i = (tree->max_keys + 1) / 2; i > idx - min_keys; --i)
            {
                new_node->children[1]->children[i] = new_node->children[1]->children[i-1];
            }
            //Insert the child in the gap
            new_node->children[1]->children[idx - min_keys + 1] = temp->children[1];
        }

        //Update all counts
        root->n_children = min_keys + 1;
        new_node->children[1]->n_children = tree->max_keys - min_keys + 1; //ceil(n/2) + 1
    }
    root->n_keys = min_keys;
    new_node->children[1]->n_keys = tree->max_keys - min_keys;
    new_node->n_keys = 1;
    new_node->n_children = 2;

    free_node(temp);
}

//Returns NULL if no splitting was needed and the key was inserted into a node
//which was not full
static BtreeNode* node_insert(BtreeNode* root, Btree* tree, int key, char* val){
    size_t i = 0;
    while(i < root->n_keys && key > root->keys[i]){
        i++;
    }
    //Now i is the index where the new node is to be inserted
    if(i < root->n_keys && key == root->keys[i]){
        //Already exists so don't insert
        return NULL;
    }

    BtreeNode* node = NULL;
    if(!root->n_children){
        //Leaf node
        if(root->n_keys == tree->max_keys){
            //Split node and push median up to parent
            node = btree_make_node(tree, key, val);

            node_split(root, tree, node, i); //This will set `node` to the pushed up node
        }
        else{
            //Insert normally
            assert(root->n_keys < tree->max_keys);
            //Insert the key at index i
            for(size_t idx = i; idx < root->n_keys; idx++){
                //Shift one ahead to make space for key
                root->keys[idx+1] = root->keys[idx];
                root->vals[idx+1] = root->vals[idx];
                move_entry(root, idx, root, idx+1);
            }
            root->keys[i] = key;
            root->vals[i] = val;
            root->n_keys++;
        }
    }
    else{
        //root is not a leaf node
        node = node_insert(root->children[i], tree, key, val); //Insert at index i branch
        if(node){
            //There was some splitting and a node was pushed back to parent (root)
            if(root->n_keys == tree->max_keys){
                node_split(root, tree, node, i); //Insert at i because `node` came from branch i
            }
            else{
                assert(root->n_keys < tree->max_keys);
                //Insert normally
                for (size_t idx = i; idx < root->n_keys; idx++)
                {
                    root->keys[idx+1] = root->keys[idx];
                    root->vals[idx+1] = root->vals[idx];
                }
                root->keys[i] = node->keys[0]; //node will have only one key
                root->vals[i] = node->vals[0];
                root->n_keys++;

                //Also handle children
                assert(node->n_children == 2);
                assert(node->n_keys == 1);

                for (size_t idx = i+1; idx < root->n_children; idx++)
                {
                    root->children[idx+1] = root->children[idx];
                }
                root->children[i] = node->children[0];
                root->children[i+1] = node->children[1];
                root->n_children++;

                free(node->keys);
                free(node->vals);
                free(node);
                node = NULL; //Now node = NULL will be returned
            }
        }
    }
    return node;
}

static BtreeNode* get_previous_key(BtreeNode* root, int idx){
    if(root->n_children == 0){
        return NULL;
    }
    BtreeNode* node = root->children[idx];
    while(node->n_children > 0){
        node = node->children[node->n_children-1];
    }
    return node; //node contains the previous key
}

static void node_rebalance(Btree* tree, BtreeNode* root, int key){
    size_t min_keys = tree->max_keys / 2;
    
    if(root->n_children == 0){
        return;
    }

    size_t i = 0;
    while(i < root->n_keys && key > root->keys[i]){
        i++;
    }

    BtreeNode* child = root->children[i];

    //Rebalance starting as deep as possible (immediate parent of a leaf)
    node_rebalance(tree, child, key);

    if(child->n_keys >= min_keys){
        return;
    }
    
    if(i < root->n_children - 1 && root->children[i+1]->n_keys > min_keys){
        //Right sibling exists and has > min keys
        BtreeNode* right = root->children[i+1];

        //Send a key from root down to the end of the deleted-from-node
        move_entry(root, i, child, child->n_keys);
        child->n_keys++;

        //Replace the sent key with the smallest key from right sibling
        move_entry(right, 0, root, i);
        
        //Left shift keys of right sibling back into place
        for (size_t j = 0; j < right->n_keys-1; j++)
        {
            move_entry(right, j+1, right, j);
        }
        right->n_keys--;
        
        if(child->n_children > 0){
            //Internal node => Need to move children around

            //Copy first child of right sibling to the end of deleted-from-node
            child->children[child->n_children] 
                = right->children[0];
            child->n_children++;

            //Remove that child from the right sibling
            for (size_t j = 0; j < right->n_children-1; j++)
            {
                right->children[j] = right->children[j+1];
            }
            right->n_children--;
        }
    }


    else if(i > 0 && root->children[i-1]->n_keys > min_keys){
        //Left sibling exists and has > min keys
        
        BtreeNode* left = root->children[i-1];

        //Send key from root down to the beginning of child
        //Shift keys to the right to make space
        for (size_t j = child->n_keys-1; j > 0; j--)
        {
            move_entry(child, j-1, child, j);
        }

        //Copy key from root to the child
        move_entry(root, i-1, child, 0);
        child->n_keys++;

        //Replace the key sent with largest key from left sibling
        move_entry(left, left->n_keys-1, root, i-1);
        left->n_keys--;

        if(child->n_children > 0){
            //Right shift children of `child` to make space for the rightmost child of `left`
            for (size_t j = child->n_children; j > 0; j--)
            {
                child->children[j] = child->children[j-1];
            }
            
            child->children[0] = left->children[left->n_children-1];
            child->n_children++;
            left->n_children--;
        }
    }

    else{
        //Immediate sibling(s) has/have min keys so merge a sibling with the child
        size_t leftIdx = (i > 0) ? i - 1 : i;

        BtreeNode* left = root->children[leftIdx];
        BtreeNode* right = root->children[leftIdx + 1];
        
        //Copy key from root to the end of `left`
        move_entry(root, leftIdx, left, left->n_keys);
        left->n_keys++;

        //Left shift keys in root
        for (size_t j = leftIdx; j < root->n_keys-1; j++)
        {
            move_entry(root, j+1, root, j);
        }
        root->n_keys--;

        //Copy elements of the right (out of the 2 being merged) node to the left
        for (size_t j = 0; j < right->n_keys; j++)
        {
            move_entry(right, j, left, j + left->n_keys);
        }
        left->n_keys += right->n_keys;

        //Copy children from right to left
        for (size_t j = 0; j < right->n_children; j++)
        {
            left->children[left->n_children + j] = right->children[j];
        }
        left->n_children += right->n_children;
        
        free_node(right);

        for (size_t j = leftIdx+1; j < root->n_children-1; j++)
        {
            root->children[j] = root->children[j+1];
        }
        root->n_children--;
        
    }
}

static int recursive_delete(Btree* tree, BtreeNode* root, int key){
    size_t i = 0;
    while(i < root->n_keys && key > root->keys[i]){
        i++;
    }
    if(i < root->n_keys && root->keys[i] == key){
        if(root->n_children == 0){
            //key found in leaf

            //Left shift keys to delete that key
            for (size_t j = i; j < root->n_keys-1; j++)
            {
                move_entry(root, j+1, root, j); 
            }
            root->n_keys--;

            if(root->n_keys < tree->max_keys / 2){
                node_rebalance(tree, tree->root, key);
            }
        }
        else{
            //Internal node
            BtreeNode* prev = get_previous_key(root, i);
            
            //Replace the key to be deleted with the largest key smaller than it
            move_entry(prev, prev->n_keys-1, root, i);
            prev->n_keys--;

            if(prev->n_keys < tree->max_keys / 2){
                node_rebalance(tree, tree->root, prev->keys[0]);
            }
        }
    }
    else{
        //key not found in this node
        if(root->n_children > 0){
            return recursive_delete(tree, root->children[i], key);
        }
        else{
            //key is not present in the tree
            return -1;
        }
    }

    return 0;
}


void btree_delete(Btree* tree, int key){
    if(tree->root){
        recursive_delete(tree, tree->root, key);
        if(tree->root->n_keys == 0){

            BtreeNode* temp = tree->root;

            if(temp->n_children > 0){
                tree->root = temp->children[0];
            }
            else{
                tree->root = NULL;
            }
            free_node(temp);
        }
    }
}