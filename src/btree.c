//
// Created by aagu on 20-3-17.
//

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"
#include "btree.h"
#include "pager.h"

NodeType get_node_type(void* node) {
    uint8_t value = *((uint8_t*)(node + NODE_TYPE_OFFSET));
    return (NodeType) value;
}

void set_node_type(void* node, NodeType type) {
    uint8_t value = type;
    *((uint8_t*) (node + NODE_TYPE_OFFSET)) = value;
}

bool is_node_root(void* node) {
    uint8_t value = *((uint8_t*) (node + IS_ROOT_OFFSET));
    return (bool)value;
}

void set_node_root(void* node, bool is_root) {
    uint8_t value = is_root;
    *((uint8_t*) (node + IS_ROOT_OFFSET)) = value;
}

uint32_t* node_parent(void* node) {
    return node + PARENT_POINTER_OFFSET;
}

// These methods return a pointer to the value in question, so they can be used both as a getter and a setter.
uint32_t* leaf_node_num_cells(void* node) {
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leaf_node_cell(void* node, uint32_t cell_num) {
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num) {
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void* node) {
    set_node_type(node, NODE_LEAF);
    set_node_root(node, false);
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0; // 0 represents no sibling
}

uint32_t leaf_node_binary_search(void* node, uint32_t key, uint32_t num_cells) {
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index) {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *leaf_node_key(node, index);
        if (key == key_at_index) {
            return index;
        }
        if (key < key_at_index) {
            one_past_max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    return min_index;
}

/*
 * this will return
 * 1. the position of the key
 * 2. the position of another key that need to be moved for insertion
 * 3. the position one past the last key
 */
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);
    uint32_t next_leaf = *leaf_node_next_leaf(node);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = page_num;

    uint32_t index =  leaf_node_binary_search(node, key, num_cells);
    cursor->cell_num = index;
    cursor->end_of_table = (next_leaf == 0) && (index == num_cells);
    return cursor;
}

Cursor* leaf_node_delete(Cursor* cursor, uint32_t key) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    uint32_t old_max_key = get_node_max_key(node);

    for (int32_t i = cursor->cell_num; i < num_cells; i++) {
        memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i + 1), LEAF_NODE_CELL_SIZE);
    }

    *leaf_node_num_cells(node) = num_cells - 1;


    if (!is_node_root(node)) {
        uint32_t parent_page_num = *node_parent(node);
        internal_node_delete(cursor->table, parent_page_num, cursor->page_num, old_max_key);
    }

    return cursor;
}

/**
 * Find the index of the correspond key in node or the index it should be placed
 * @param node
 * @param key
 * @return index of the key or index it should be placed
 */
uint32_t internal_node_find_child(void* node, uint32_t key) {
    /*
     * Return the index of the child which should contain
     * the given key
     */

    uint32_t num_keys = *internal_node_num_keys(node);

    /*
     * Binary search
     */
    uint32_t min_index = 0;
    uint32_t max_index = num_keys; /* there is one more child than key */

    while (min_index != max_index) {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = *internal_node_key(node, index);
        if (key_to_right >= key) {
            max_index = index;
        } else {
            min_index = index + 1;
        }
    }

    return min_index;
}

void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
    uint32_t old_child_index = internal_node_find_child(node, old_key);
    *internal_node_key(node, old_child_index) = new_key;
}

uint32_t* internal_node_cell(void* node, uint32_t cell_num) {
    return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}

void internal_node_delete_cell(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* parent = get_page(table->pager, parent_page_num);
    uint32_t right_child = *internal_node_right_child(parent);
    uint32_t num_keys = *internal_node_num_keys(parent);

    if (right_child == child_page_num) {
        if (num_keys == 1) {
            uint32_t parent_page = *node_parent(parent);
            internal_node_merge(table, parent_page, parent_page_num);
        } else {
            *internal_node_num_keys(parent) = num_keys - 1;
            uint32_t left_sibling_cell_page = *internal_node_child(parent, num_keys - 1);
            *internal_node_right_child(parent) = left_sibling_cell_page;
        }
    } else {
        void* child_node = get_page(table->pager, child_page_num);
        uint32_t key = get_node_max_key(child_node);
        uint32_t index = internal_node_find_child(parent, key);

        for (uint32_t i = num_keys - 1; i > index; i--) {
            memcpy(internal_node_cell(parent, i - 1), internal_node_cell(parent, i), INTERNAL_NODE_CELL_SIZE);
        }
    }
}

void leaf_node_merge(Table* table, uint32_t left_node_page, uint32_t right_node_page) {
    void* right_node = get_page(table->pager, right_node_page);
    void* left_node = get_page(table->pager, left_node_page);
    uint32_t left_node_num_cells = *leaf_node_num_cells(left_node);
    uint32_t right_node_num_cells = *leaf_node_num_cells(right_node);
    uint32_t old_max_key = get_node_max_key(left_node);

    *leaf_node_next_leaf(left_node) = *leaf_node_next_leaf(right_node);

    for (uint32_t index = 0; index < right_node_num_cells; index++) {
        void* source_cell = leaf_node_cell(right_node, index);
        void* destination_cell = leaf_node_cell(left_node, index + left_node_num_cells);

        memcpy(destination_cell, source_cell, LEAF_NODE_CELL_SIZE);
    }

    *leaf_node_num_cells(left_node) += right_node_num_cells;

    uint32_t left_parent_page = *node_parent(left_node);
    uint32_t right_parent_page = *node_parent(right_node);

    void* parent = get_page(table->pager, left_parent_page);

    if (left_node_page != *internal_node_right_child(parent)) {
        uint32_t new_max_key = get_node_max_key(left_node);
        update_internal_node_key(parent, old_max_key, new_max_key);
    }

    internal_node_delete_cell(table, right_parent_page, right_node_page);

    free(right_node);
    table->pager->pages[right_node_page] = NULL;
}

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = get_page(table->pager, page_num);

    uint32_t child_index = internal_node_find_child(node, key);
    uint32_t child_num = *internal_node_child(node, child_index);
    void* child = get_page(table->pager, child_num);
    switch (get_node_type(child)) {
        case NODE_INTERNAL:
            return internal_node_find(table, child_num, key);
        case NODE_LEAF:
            return leaf_node_find(table, child_num, key);
    }
}

uint32_t internal_node_find_left_leaf(Table* table, uint32_t page_num, uint32_t target_page) {
    void* this_page = get_page(table->pager, page_num);

    if (get_node_type(this_page) == NODE_LEAF) return 0;

    uint32_t num_keys = *internal_node_num_keys(this_page);
    for (uint32_t i = 0; i <= num_keys; i++) {
        uint32_t child_page = *internal_node_cell(this_page, i);
        void* child_node = get_page(table->pager, child_page);

        if (get_node_type(child_node) == NODE_INTERNAL) {
            uint32_t res = internal_node_find_left_leaf(table, child_page, target_page);
            if (res != 0) return res;
        } else {
            if (*leaf_node_next_leaf(child_node) == target_page) return child_page;
        }
    }

    uint32_t parent_num = *node_parent(this_page);
    return internal_node_find_left_leaf(table, parent_num, target_page);
}

uint32_t leaf_node_find_left_sibling(Table* table, uint32_t child_page_num) {
    void* child = get_page(table->pager, child_page_num);
    uint32_t parent_page = *node_parent(child);
    return internal_node_find_left_leaf(table, parent_page, child_page_num);
}

void internal_node_delete(Table* table, uint32_t parent_page_num, uint32_t child_page_num, uint32_t old_max_key) {
    void* parent = get_page(table->pager, parent_page_num);
    void* child = get_page(table->pager, child_page_num);
    if (get_node_type(parent) == NODE_LEAF) return;

    uint32_t child_num_cells;
    uint32_t max_cells;

    if (get_node_type(child) == NODE_LEAF) {
        child_num_cells = *leaf_node_num_cells(child);
        max_cells = LEAF_NODE_MAX_CELLS;
    } else {
        printf("Need to implement internal key deletion");
        exit(EXIT_FAILURE);
    }

    if (child_num_cells < max_cells / 2) {
        uint32_t right_sibling_page = *leaf_node_next_leaf(child);
        if (right_sibling_page == 0) { // no right sibling, try to get left sibling
            uint32_t left_sibling_page = leaf_node_find_left_sibling(table, child_page_num);
            void* left_sibling = get_page(table->pager, left_sibling_page);
            uint32_t left_sibling_num_cells = *leaf_node_num_cells(left_sibling);
            if (left_sibling_num_cells < max_cells / 2) {
                leaf_node_merge(table, left_sibling_page, child_page_num);
            } else {
                uint32_t old_key = get_node_max_key(left_sibling);
                for (uint32_t i = 0; i < child_num_cells; i++) {
                    void* source_cell;
                    if (i == 0) {
                        source_cell = leaf_node_cell(left_sibling, left_sibling_num_cells - 1);
                    } else {
                        source_cell = leaf_node_cell(child, i - 1);
                    }

                    memcpy(leaf_node_cell(child, i), source_cell, LEAF_NODE_CELL_SIZE);
                }

                uint32_t left_sibling_parent_page = *node_parent(left_sibling);
                void* left_sibling_parent = get_page(table->pager, left_sibling_parent_page);

                if (left_sibling_page != *internal_node_right_child(left_sibling_parent)) {
                    uint32_t new_key = get_node_max_key(left_sibling);
                    update_internal_node_key(left_sibling_parent, old_key, new_key);
                }

                *leaf_node_num_cells(child) += 1;
                *leaf_node_num_cells(left_sibling) -= 1;
            }
        } else {
            void* right_sibling = get_page(table->pager, right_sibling_page);
            uint32_t right_sibling_num_cells = *leaf_node_num_cells(right_sibling);
            if (right_sibling_num_cells < max_cells / 2) {
                leaf_node_merge(table, child_page_num, right_sibling_page);
            } else {
                for (uint32_t i = 0; i < right_sibling_num_cells; i++) {
                    void* destination_cell;
                    if (i == 0) {
                        destination_cell = leaf_node_cell(child, child_num_cells);
                    } else {
                        destination_cell = leaf_node_cell(right_sibling, i - 1);
                    }

                    memcpy(destination_cell, leaf_node_cell(right_sibling, i), LEAF_NODE_CELL_SIZE);
                }

                *leaf_node_num_cells(child) += 1;
                *leaf_node_num_cells(right_sibling) -= 1;
            }
        }
    }

    if(is_node_root(child)) return;

    if (child_page_num != *internal_node_right_child(parent)) {
        uint32_t new_max_key = get_node_max_key(child);
        update_internal_node_key(parent, old_max_key, new_max_key);
    }
}

uint32_t* internal_node_num_keys(void* node) {
    return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

uint32_t* internal_node_right_child(void* node) {
    return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

void internal_node_merge(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* parent = get_page(table->pager, parent_page_num);
    void* child = get_page(table->pager, child_page_num);

    if (is_node_root(child)) {
        uint32_t node_page = *internal_node_cell(child, 0);
        void* node = get_page(table->pager, node_page);
        set_node_root(node, true);
        set_node_root(child, false);
        free(child);
        table->pager->pages[child_page_num] = NULL;
        return;
    }
    uint32_t num_keys = *internal_node_num_keys(parent);
    uint32_t* pages_of_children = malloc(sizeof(uint32_t) * (num_keys + 1));
    for (uint32_t i = 0; i <= num_keys; i++) {
        pages_of_children[i] = *internal_node_child(parent, i);
    }
}

uint32_t* internal_node_child(void* node, uint32_t child_num) {
    uint32_t num_keys = *internal_node_num_keys(node);
    if (child_num > num_keys) {
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    } else if (child_num == num_keys) {
        return internal_node_right_child(node);
    } else {
        return internal_node_cell(node, child_num);
    }
}

uint32_t* internal_node_key(void* node, uint32_t key_num) {
    return (void*)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}

void initialize_internal_node(void* node) {
    set_node_type(node, NODE_INTERNAL);
    set_node_root(node, false);
    *internal_node_num_keys(node) = 0;
}

uint32_t get_node_max_key(void* node) {
    switch (get_node_type(node)) {
        case NODE_INTERNAL:
            return *internal_node_key(node, *internal_node_num_keys(node) - 1);
        case NODE_LEAF:
            return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
    }
}

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* row) {
    void* node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        // Node full
        leaf_node_split_and_insert(cursor, key, row);
        return;
    }

    if (cursor->cell_num < num_cells) {
        // Make room for new cell by shifting cells with larger cell num to right
        for (uint32_t i = num_cells; i > cursor->cell_num; --i) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(row, leaf_node_value(node, cursor->cell_num));
}

void create_new_root(Table* table, uint32_t right_child_page_num) {
    /*
     * Handle splitting the root.
     * Old root copied to new page, becomes left child.
     * Address of right child passed in.
     * Re-initialize root page to contain the new root node.
     * New root node points to two children;
     */

    void* root = get_page(table->pager, table->root_page_num);
    void* right_child = get_page(table->pager, right_child_page_num);
    uint32_t left_child_page_num = get_unused_page_num(table->pager);
    void* left_child = get_page(table->pager, left_child_page_num);

    /*
     * Left child has data copied from old root;
     */
    memcpy(left_child, root, PAGE_SIZE);
    set_node_root(left_child, false);

    /*
     * Root node is a new internal node with one key and two children
     */
    initialize_internal_node(root);
    set_node_root(root, true);
    *internal_node_num_keys(root) = 1;
    *internal_node_child(root, 0) = left_child_page_num;
    uint32_t left_child_max_key = get_node_max_key(left_child);
    *internal_node_key(root, 0) = left_child_max_key;
    *internal_node_right_child(root) = right_child_page_num;
    *node_parent(left_child) = table->root_page_num;
    *node_parent(right_child) = table->root_page_num;
}

void do_internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    /*
     * Add a new child/key pair to parent that corresponds to child
     */
    void* parent = get_page(table->pager, parent_page_num);
    void* child = get_page(table->pager, child_page_num);
    uint32_t child_max_key = get_node_max_key(child);
    uint32_t index = internal_node_find_child(parent, child_max_key);

    uint32_t original_num_keys = *internal_node_num_keys(parent);
    *internal_node_num_keys(parent) = original_num_keys + 1;

    uint32_t right_child_page_num = *internal_node_right_child(parent);
    if (right_child_page_num == 0) {
        // parent node is empty
        *internal_node_child(parent, original_num_keys) = child_page_num;
        *internal_node_right_child(parent) = child_page_num;
        *node_parent(child) = parent_page_num;
        return;
    }
    void* right_child = get_page(table->pager, right_child_page_num);

    uint32_t old_max_key = get_node_max_key(right_child);
    if (child_max_key > old_max_key) {
        // Replace right child
        *internal_node_child(parent, original_num_keys) = right_child_page_num;
        *internal_node_key(parent, original_num_keys) = get_node_max_key(right_child);
        *internal_node_right_child(parent) = child_page_num;
    } else {
        // Make room for the new cell
        for (uint32_t i = original_num_keys; i > index; i--) {
            void* destination = internal_node_cell(parent, i);
            void* source = internal_node_cell(parent, i - 1);
            memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
        }
        *internal_node_child(parent, index) = child_page_num;
        *internal_node_key(parent, index) = child_max_key;
    }

    *node_parent(child) = parent_page_num;
}

void internal_node_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* old_node = get_page(table->pager, parent_page_num);
    uint32_t new_page_num = get_unused_page_num(table->pager);
    uint32_t old_parent_max_key = get_node_max_key(old_node);
    void* new_node = get_page(table->pager, new_page_num);
    initialize_internal_node(new_node);

    uint32_t old_right_child_page_num = *internal_node_right_child(old_node);
    void* old_right_child = get_page(table->pager, old_right_child_page_num);
    void* insert_child = get_page(table->pager, child_page_num);

    uint32_t old_max = get_node_max_key(old_right_child);
    uint32_t insert_max = get_node_max_key(insert_child);

    if (old_max > insert_max) {
        *internal_node_cell(new_node, 0) = child_page_num;
        *internal_node_num_keys(new_node) = 1;
        *internal_node_key(new_node, 0) = insert_max;
        *internal_node_cell(new_node, 1) = old_right_child_page_num;
        *internal_node_right_child(new_node) = old_right_child_page_num;
    } else {
        *internal_node_cell(new_node, 0) = old_right_child_page_num;
        *internal_node_num_keys(new_node) = 1;
        *internal_node_key(new_node, 0) = old_max;
        *internal_node_cell(new_node, 1) = child_page_num;
        *internal_node_right_child(new_node) = child_page_num;
    }

    // Split node
    for (int32_t i = INTERNAL_NODE_MAX_CELLS - 1; i > INTERNAL_NODE_LEFT_SPILT_COUNT; i--) {
        uint32_t child_page = *internal_node_child(old_node, i);

        do_internal_node_insert(table, new_page_num, child_page);
    }

    uint32_t child_page = *internal_node_child(old_node, INTERNAL_NODE_LEFT_SPILT_COUNT);
    *internal_node_right_child(old_node) = child_page;

    *internal_node_num_keys(old_node) = INTERNAL_NODE_LEFT_SPILT_COUNT;

    if (is_node_root(old_node)) {
        create_new_root(table, new_page_num);
    } else {
        uint32_t parent_parent_page_num = *node_parent(old_node);
        uint32_t new_max = get_node_max_key(old_node);
        void* parent_parent = get_page(table->pager, parent_parent_page_num);

        update_internal_node_key(parent_parent, old_parent_max_key, new_max);
        internal_node_insert(table, parent_parent_page_num, new_page_num);
    }
}

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
    void* parent = get_page(table->pager, parent_page_num);

    uint32_t original_num_keys = *internal_node_num_keys(parent);

    if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
        return internal_node_split_and_insert(table, parent_page_num, child_page_num);
    } else {
        return do_internal_node_insert(table, parent_page_num, child_page_num);
    }
}

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
    /*
     * Create a new node and move half the cells over.
     * Insert the new value in one of the two parts.
     * Update parent or create a new parent.
     */

    void* old_node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t old_max = get_node_max_key(old_node);
    uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
    void* new_node = get_page(cursor->table->pager, new_page_num);
    initialize_leaf_node(new_node);

    *node_parent(new_node) = *node_parent(old_node);
    uint32_t next_leaf_page_num = *leaf_node_next_leaf(old_node);
    *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
    *leaf_node_next_leaf(old_node) = new_page_num;

    /*
     * All existing keys plus new key should be divided evenly between
     * old (left) and new (right) nodes. Starting from the right, move
     * each key to correct position.
     */
    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
        void* destination_node;
        if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
            destination_node = new_node;
        } else {
            destination_node = old_node;
        }
        uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        void* destination_cell = leaf_node_cell(destination_node, index_within_node);

        if (i == cursor->cell_num) {
            // save the new key/value pair
            serialize_row(value,
                    leaf_node_value(destination_node, index_within_node));
            *leaf_node_key(destination_node, index_within_node) = key;
        } else if (i > cursor->cell_num) {
            // larger key move right
            memcpy(destination_cell, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        } else {
            // smaller key keep untouched
            memcpy(destination_cell, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
        }
    }

    *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
    *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;

    if (is_node_root(old_node)) {
        return create_new_root(cursor->table, new_page_num);
    } else {
        uint32_t parent_page_num = *node_parent(old_node);
        uint32_t new_max = get_node_max_key(old_node);
        void* parent = get_page(cursor->table->pager, parent_page_num);

        update_internal_node_key(parent, old_max, new_max);
        internal_node_insert(cursor->table, parent_page_num, new_page_num);
        return;
    }
}

uint32_t* leaf_node_next_leaf(void* node) {
    return node + LEAF_NODE_NEXT_LEAF_OFFSET;
}

void print_leaf_node(void* node) {
    uint32_t num_cells = *leaf_node_num_cells(node);
    printf("leaf (size %d)\n", num_cells);
    for (uint32_t i = 0; i < num_cells; i++) {
        uint32_t key = *leaf_node_key(node, i);
        printf("\t- %d : %d\n", i, key);
    }
}

void indent(uint32_t level) {
    for (uint32_t i = 0; i < level; i++) {
        printf("  ");
    }
}

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
    void* node = get_page(pager, page_num);
    uint32_t num_keys, child_page;

    switch (get_node_type(node)) {
        case NODE_INTERNAL:
            num_keys = *internal_node_num_keys(node);
            indent(indentation_level);
            printf("- internal (size %d)\n", num_keys);
            for (uint32_t i = 0; i < num_keys; i++) {
                child_page = *internal_node_cell(node, i);
                print_tree(pager, child_page, indentation_level + 1);

                indent(indentation_level + 1);
                printf("- key %d\n", *internal_node_key(node, i));
            }
            child_page = *internal_node_right_child(node);
            print_tree(pager, child_page, indentation_level + 1);
            break;
        case NODE_LEAF:
            num_keys = *leaf_node_num_cells(node);
            indent(indentation_level);
            printf("- leaf (size %d)\n", num_keys);
            for (uint32_t i = 0; i < num_keys; i++) {
                indent(indentation_level + 1);
                printf("- %d\n", *leaf_node_key(node, i));
            }
            break;
    }
}