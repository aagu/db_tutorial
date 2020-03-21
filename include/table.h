//
// Created by aagu on 20-3-17.
//

#ifndef SQLMINI_TABLE_H
#define SQLMINI_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pager.h"

typedef struct {
    Pager* pager;
    uint32_t root_page_num;
} Table;

typedef struct {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates a position one past the last element
} Cursor;

Cursor* table_start(Table* table);

Cursor* table_find(Table* table, uint32_t key);

Cursor* table_remove(Table* table, uint32_t key);

void* cursor_value(Cursor* cursor);

void cursor_advance(Cursor* cursor);

void cursor_delete(Cursor* cursor);
#endif //SQLMINI_TABLE_H
