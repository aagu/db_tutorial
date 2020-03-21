//
// Created by aagu on 20-3-19.
//

#ifndef SQLMINI_UTILS_H
#define SQLMINI_UTILS_H

#include "constants.h"

void apply_where(char* where_clause, Statement* statement);

uint8_t where_constrain_satisfied(Row* row, WhereClause clause);
#endif //SQLMINI_UTILS_H
