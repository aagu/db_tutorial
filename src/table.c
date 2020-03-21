//
// Created by aagu on 20-3-17.
//

#include "table.h"
#include "btree.h"

Cursor* table_start(Table* table) {
    Cursor* cursor = table_find(table, 0);

    void* node = get_page(table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

/*
 * Return the position of the give key.
 * If the key is not present, return the position where it should be inserted.
 */
Cursor* table_find(Table* table, uint32_t key) {
    uint32_t root_page_num = table->root_page_num;
    void* root_node = get_page(table->pager, root_page_num);

    if (get_node_type(root_node) == NODE_LEAF) {
        return leaf_node_find(table, root_page_num, key);
    } else {
        return internal_node_find(table, root_page_num, key);
    }
}

//Cursor* table_remove(Table *table, uint32_t key) {
//    uint32_t root_page_num = table->root_page_num;
//    void* root_node = get_page(table->pager, root_page_num);
//
//    if (get_node_type(root_node) == NODE_LEAF) {
//        return leaf_node_delete(table, root_page_num, key);
//    } else {
//        return internal_node_delete(table, root_page_num, key);
//    }
//}

void* cursor_value(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* page = get_page(cursor->table->pager, page_num);
    return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor* cursor) {
    uint32_t page_num = cursor->page_num;
    void* node = get_page(cursor->table->pager, page_num);

    cursor->cell_num += 1;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        /*
         * Advance to next leaf node
         */
        uint32_t next_page_num = *leaf_node_next_leaf(node);
        if (next_page_num == 0) {
            // This was rightmost leaf
            cursor->end_of_table = true;
        } else {
            cursor->page_num = next_page_num;
            cursor->cell_num = 0;
        }
    }
}
