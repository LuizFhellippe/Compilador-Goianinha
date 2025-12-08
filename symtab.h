#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"  // para usar DataType

typedef enum {
    SYMBOL_VAR,
    SYMBOL_PARAM,
    SYMBOL_FUNC
} SymbolCategory;

// Reutiliza DataType para o tipo do símbolo
typedef DataType SymbolType;

typedef struct SymbolNode {
    char *id_name;
    SymbolCategory category;
    SymbolType type;
    int scope_level;
    int declaration_pos;
    int arg_count;
    struct AstNode *param_list;   // <<< NOVO: lista de parâmetros (NODE_PARAM_LIST)
    struct SymbolNode *next;
} SymbolNode;

#define HASH_TABLE_SIZE 101

typedef struct Scope {
    SymbolNode* hash_table[HASH_TABLE_SIZE];
    struct Scope *enclosing_scope;
} Scope;

void symtab_init();
void symtab_enter_scope();
void symtab_leave_scope();

SymbolNode* symtab_insert(const char *name, SymbolCategory category, SymbolType type, int pos);
SymbolNode* symtab_lookup(const char *name);

void symtab_destroy();
void symtab_print();

#endif