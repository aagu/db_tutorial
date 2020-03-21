//
// Created by aagu on 20-3-19.
//

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

void apply_where(char *where_clause, Statement* statement) {
    uint32_t str_len = strlen(where_clause);

    if (str_len < 4) return;
    if (strncmp(where_clause, "id", 2) == 0) {
        if (where_clause[3] == '=') {
            char* id = malloc(sizeof(char) * (str_len - 4));
            strncpy(id, where_clause + 4, str_len - 4);
            uint32_t id_num = atoi(id);
            if (where_clause[2] == '>') {
                statement->clause.type = EQUAL_OR_LARGER;
            } else if (where_clause[2] == '<') {
                statement->clause.type = LESS_OR_EQUAL;
            }
            statement->clause.id = id_num;
            free(id);
        } else if (where_clause[3] >= '0' && where_clause[3] <= '9') {
            char* id = malloc(sizeof(char) * (str_len - 3));
            strncpy(id, where_clause + 3, str_len - 3);
            uint32_t id_num = atoi(id);
            if (where_clause[2] == '>') {
                statement->clause.type = LARGER;
            } else if (where_clause[2] == '<') {
                statement->clause.type = LESS;
            } else if (where_clause[2] == '=') {
                statement->clause.type = EQUAL;
            }
            statement->clause.id = id_num;
            free(id);
        }
    }
}

uint8_t where_constrain_satisfied(Row *row, WhereClause clause) {
    int type = (int)clause.type;

    if (type == (int)NO_CONSTRAIN) return 1;

    if (type == (int)LESS) return row->id < clause.id;

    if (type == (int)LESS_OR_EQUAL) return row->id <= clause.id;

    if (type == (int)EQUAL) return row->id == clause.id;

    // > and >= are controlled by cursor itself
    if (type == (int)EQUAL_OR_LARGER || type == (int)LARGER) return 1;

    return 0;
}
