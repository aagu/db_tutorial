//
// Created by aagu on 20-3-17.
//

#ifndef SQLMINI_CONSTANTS_H
#define SQLMINI_CONSTANTS_H

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_DELETE
} StatementType;

typedef enum {
    NO_CONSTRAIN,
    LESS,
    LESS_OR_EQUAL,
    EQUAL,
    EQUAL_OR_LARGER,
    LARGER
} EqualType;

typedef struct {
    EqualType type;
    uint32_t id;
} WhereClause;

typedef struct {
    StatementType type;
    Row row_to_manipulate; // only used by insert statement
    WhereClause clause;
} Statement;

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

static const uint32_t ID_SIZE = size_of_attribute(Row, id);
static const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
static const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
static const uint32_t ID_OFFSET = 0;
static const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
static const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
static const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

static const uint32_t PAGE_SIZE = 4096;

#endif //SQLMINI_CONSTANTS_H
