//
// Created by aagu on 20-3-17.
//

#ifndef SQLMINI_BTREE_H
#define SQLMINI_BTREE_H

#include "constants.h"

/*
 * Common Node Header Layout
 */
static const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
static const uint32_t NODE_TYPE_OFFSET = 0;
static const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
static const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
static const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
static const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
static const uint8_t COMMON_NODE_HEADER_SIZE =
        NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

/*
 * Leaf Node Header Layout
 */
static const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
static const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
static const uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
static const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
static const uint32_t LEAF_NODE_HEADER_SIZE =
        COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE;

/*
 * Leaf Node Body Layout
 */
static const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
static const uint32_t LEAF_NODE_KEY_OFFSET = 0;
static const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
static const uint32_t LEAF_NODE_VALUE_OFFSET =
        LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
static const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
static const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
static const uint32_t LEAF_NODE_MAX_CELLS =
        LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;
static const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
static const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT =
        (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

/*
 * Internal Node Header Layout
 */
static const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
static const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
static const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
static const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
        INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
static const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                           INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

/*
 * Internal Node Body Layout
 */
static const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
static const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
static const uint32_t INTERNAL_NODE_NUM_CELLS_OFFSET =
        INTERNAL_NODE_KEY_SIZE + INTERNAL_NODE_CHILD_SIZE;
static const uint32_t INTERNAL_NODE_CELL_SIZE =
        INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;
static const uint32_t INTERNAL_NODE_MAX_CELLS = 3;
static const uint32_t INTERNAL_NODE_RIGHT_SPILT_COUNT = (INTERNAL_NODE_MAX_CELLS + 1) / 2;
static const uint32_t INTERNAL_NODE_LEFT_SPILT_COUNT =
        (INTERNAL_NODE_MAX_CELLS + 1) - INTERNAL_NODE_RIGHT_SPILT_COUNT;

typedef enum {
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

NodeType get_node_type(void* node);

void set_node_type(void* node, NodeType type);

bool is_node_root(void* node);

void set_node_root(void* node, bool is_root);

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key);

void internal_node_delete(Table* table, uint32_t page_num, uint32_t key, uint32_t old_max_key);

void initialize_internal_node(void* node);

uint32_t* internal_node_num_keys(void* node);

uint32_t* internal_node_child(void* node, uint32_t child_num);

uint32_t get_node_max_key(void* node);

uint32_t* internal_node_key(void* node, uint32_t key_num);

uint32_t* internal_node_right_child(void* node);

//void internal_node_delete(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

void internal_node_merge(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

void internal_node_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

void initialize_leaf_node(void* node);

Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key);

Cursor* leaf_node_delete(Cursor* cursor, uint32_t key);

void* leaf_node_cell(void* node, uint32_t cell_num);

uint32_t* leaf_node_num_cells(void* node);

uint32_t* leaf_node_key(void* node, uint32_t cell_num);

void* leaf_node_value(void* node, uint32_t cell_num);

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value);

uint32_t* leaf_node_next_leaf(void* node);

void leaf_node_merge(Table* table, uint32_t left_node_page, uint32_t right_node_page);

void create_new_root(Table* table, uint32_t right_child_page_num);

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* row);

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level);
#endif //SQLMINI_BTREE_H
