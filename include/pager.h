//
// Created by aagu on 20-3-17.
//

#ifndef SQLMINI_PAGER_H
#define SQLMINI_PAGER_H

#define TABLE_MAX_PAGES 100

#include "constants.h"

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES];
} Pager;

void* get_page(Pager* pager, uint32_t page_num);

void serialize_row(Row* source, void* destination);

uint32_t get_unused_page_num(Pager* pager);

void deserialize_row(void* source, Row* destination);

Pager *pager_open(const char *filename);

void pager_flush(Pager* pager, uint32_t page_num);
#endif //SQLMINI_PAGER_H
