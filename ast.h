#ifndef AST_H
#define AST_H

// Tipos de nós da AST
typedef enum {
    // Nós de comando
    NODE_PROGRAM,
    NODE_VAR_DECL,
    NODE_FUNC_DECL,
    NODE_BLOCK,
    NODE_ASSIGN,
    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,
    NODE_READ,
    NODE_WRITE,
    NODE_NEWLINE,
    NODE_CALL,
    
    // Nós de expressão
    NODE_ADD, NODE_SUB, NODE_MUL, NODE_DIV,
    NODE_EQ, NODE_NEQ, NODE_LT, NODE_GT, NODE_LTE, NODE_GTE,
    NODE_OR, NODE_AND, NODE_NOT,
    NODE_UNARY_MINUS,

    // Nós de tipo e literais
    NODE_TYPE,
    NODE_ID,
    NODE_INTCONST,
    NODE_CARCONST,
    NODE_STRCONST,

    // Nós para listas
    NODE_STMT_LIST,
    NODE_DECL_LIST,
    NODE_PARAM_LIST,
    NODE_ARG_LIST
} NodeType;

// Tipos de dados da linguagem Goianinha (para análise semântica)
typedef enum {
    TYPE_UNDEFINED,
    TYPE_INT,
    TYPE_CAR,
    TYPE_VOID // Para funções que não retornam valor (embora Goianinha exija retorno)
} DataType;

// Estrutura de um nó da AST
typedef struct AstNode {
    NodeType type;
    int lineno;
    DataType data_type; 

    struct AstNode *child1;
    struct AstNode *child2;
    struct AstNode *child3;

    union {
        int int_val;
        char char_val;
        char* str_val;
    };

    struct AstNode *next; 
} AstNode;

AstNode* create_node(NodeType type, AstNode *c1, AstNode *c2, AstNode *c3, int lineno);
AstNode* create_id_node(char* id, int lineno);
AstNode* create_int_node(int val, int lineno);
AstNode* create_str_node(char* str, int lineno);
AstNode* create_char_node(char val, int lineno);
AstNode* append_to_list(AstNode* head, AstNode* newItem);

void free_ast(AstNode *node);
void print_ast(AstNode *node, int level);

#endif