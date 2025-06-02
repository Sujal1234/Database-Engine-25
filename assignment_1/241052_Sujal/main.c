#include <stdio.h>
#include <stdarg.h>

#include "btree.h"

int main(){
    Btree tree = make_btree(2);
    
    while(1){
        int key;
        printf("Enter key to be inserted: ");
        scanf("%d", &key);
        if(key == -1){
            break;
        }
        btree_insert(&tree, key, "");
    }
    print_tree(&tree);
    // print_node(node_found);
    btree_free(&tree);
}