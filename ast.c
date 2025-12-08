#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

AstNode* create_node(NodeType type, AstNode *c1, AstNode *c2, AstNode *c3, int lineno) {
    AstNode *node = (AstNode*) malloc(sizeof(AstNode));
    if (!node) {
        fprintf(stderr, "Erro: falha ao alocar nó da AST.\n");
        exit(1);
    }
    node->type = type;
    node->child1 = c1;
    node->child2 = c2;
    node->child3 = c3;
    node->next   = NULL;
    node->lineno = lineno;
    node->data_type = TYPE_UNDEFINED;
    node->str_val = NULL; // por segurança
    node->int_val = 0;
    node->char_val = 0;
    return node;
}

AstNode* create_id_node(char* id, int lineno) {
    AstNode *node = create_node(NODE_ID, NULL, NULL, NULL, lineno);
    node->str_val = strdup(id);
    return node;
}

AstNode* create_int_node(int val, int lineno) {
    AstNode *node = create_node(NODE_INTCONST, NULL, NULL, NULL, lineno);
    node->int_val = val;
    return node;
}

AstNode* create_str_node(char* str, int lineno) {
    AstNode *node = create_node(NODE_STRCONST, NULL, NULL, NULL, lineno);
    node->str_val = strdup(str);
    return node;
}

AstNode* create_char_node(char val, int lineno) {
    AstNode *node = create_node(NODE_CARCONST, NULL, NULL, NULL, lineno);
    node->char_val = val;
    return node;
}

AstNode* append_to_list(AstNode* head, AstNode* newItem) {
    if (!newItem) return head;
    if (!head) return newItem;
    AstNode* cur = head;
    while (cur->next) cur = cur->next;
    cur->next = newItem;
    return head;
}

void free_ast(AstNode *node) {
    if (!node) return;
    free_ast(node->child1);
    free_ast(node->child2);
    free_ast(node->child3);
    free_ast(node->next);
    if (node->type == NODE_ID || node->type == NODE_STRCONST) {
        free(node->str_val);
    }
    free(node);
}

// opcional debug
static const char* node_type_name(NodeType t) {
    switch(t) {
        case NODE_PROGRAM: return "PROGRAM";
        case NODE_VAR_DECL: return "VAR_DECL";
        case NODE_FUNC_DECL: return "FUNC_DECL";
        case NODE_BLOCK: return "BLOCK";
        case NODE_ASSIGN: return "ASSIGN";
        case NODE_IF: return "IF";
        case NODE_WHILE: return "WHILE";
        case NODE_RETURN: return "RETURN";
        case NODE_READ: return "READ";
        case NODE_WRITE: return "WRITE";
        case NODE_NEWLINE: return "NEWLINE";
        case NODE_CALL: return "CALL";
        case NODE_ADD: return "ADD";
        case NODE_SUB: return "SUB";
        case NODE_MUL: return "MUL";
        case NODE_DIV: return "DIV";
        case NODE_EQ: return "EQ";
        case NODE_NEQ: return "NEQ";
        case NODE_LT: return "LT";
        case NODE_GT: return "GT";
        case NODE_LTE: return "LTE";
        case NODE_GTE: return "GTE";
        case NODE_OR: return "OR";
        case NODE_AND: return "AND";
        case NODE_NOT: return "NOT";
        case NODE_UNARY_MINUS: return "UNARY_MINUS";
        case NODE_TYPE: return "TYPE";
        case NODE_ID: return "ID";
        case NODE_INTCONST: return "INTCONST";
        case NODE_CARCONST: return "CARCONST";
        case NODE_STRCONST: return "STRCONST";
        case NODE_STMT_LIST: return "STMT_LIST";
        case NODE_DECL_LIST: return "DECL_LIST";
        case NODE_PARAM_LIST: return "PARAM_LIST";
        case NODE_ARG_LIST: return "ARG_LIST";
        default: return "UNKNOWN";
    }
}

void print_ast(AstNode *node, int level) {
    if (!node) return;
    for (int i = 0; i < level; i++) printf("  ");
    printf("%s (linha %d)\n", node_type_name(node->type), node->lineno);
    print_ast(node->child1, level + 1);
    print_ast(node->child2, level + 1);
    print_ast(node->child3, level + 1);
    print_ast(node->next, level);
}