// CONTEÚDO CORRETO PARA symtab.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

static Scope *current_scope = NULL;
static int current_level = -1;

static unsigned int hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_TABLE_SIZE;
}

void symtab_init() {
    current_scope = NULL;
    current_level = -1;
    symtab_enter_scope();
}

void symtab_enter_scope() {
    Scope *new_scope = (Scope*) malloc(sizeof(Scope));
    if (!new_scope) {
        fprintf(stderr, "Fatal: Out of memory for new scope.\n");
        exit(1);
    }
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        new_scope->hash_table[i] = NULL;
    }
    new_scope->enclosing_scope = current_scope;
    current_scope = new_scope;
    current_level++;
}

void symtab_leave_scope() {
    if (!current_scope) return;
    Scope *scope_to_free = current_scope;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        SymbolNode *node = scope_to_free->hash_table[i];
        while (node) {
            SymbolNode *temp = node;
            node = node->next;
            free(temp->id_name);
            free(temp);
        }
    }
    current_scope = scope_to_free->enclosing_scope;
    free(scope_to_free);
    current_level--;
}

SymbolNode* symtab_insert(const char *name, SymbolCategory category, SymbolType type, int pos) {
    if (!current_scope) return NULL;
    unsigned int index = hash(name);
    SymbolNode *node = current_scope->hash_table[index];
    while(node) {
        if(strcmp(node->id_name, name) == 0) {
            return NULL; 
        }
        node = node->next;
    }
    SymbolNode *new_node = (SymbolNode*) malloc(sizeof(SymbolNode));
    new_node->id_name = strdup(name);
    new_node->category = category;
    new_node->type = type;
    new_node->scope_level = current_level;
    new_node->declaration_pos = pos;
    new_node->arg_count = 0;
    new_node->param_list = NULL;
    new_node->next = current_scope->hash_table[index];
    current_scope->hash_table[index] = new_node;
    return new_node;
}

SymbolNode* symtab_lookup(const char *name) {
    Scope *scope = current_scope;
    unsigned int index = hash(name);
    while (scope) {
        SymbolNode *node = scope->hash_table[index];
        while (node) {
            if (strcmp(node->id_name, name) == 0) {
                return node;
            }
            node = node->next;
        }
        scope = scope->enclosing_scope;
    }
    return NULL;
}

void symtab_destroy() {
    while (current_scope) {
        symtab_leave_scope();
    }
}