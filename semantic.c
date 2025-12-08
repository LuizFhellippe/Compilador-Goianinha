#include <stdio.h>
#include <stdlib.h>
#include "semantic.h"
#include "symtab.h"

static int semantic_errors = 0;

void report_error(const char* message, int lineno) {
    fprintf(stderr, "ERRO SEMANTICO (linha %d): %s\n", lineno, message);
    semantic_errors++;
}

void check_node(AstNode *node) {
    if (!node) return;

    // A recursão nos filhos é feita dentro de cada case.
    // A recursão nos nós da lista (next) é feita no final.

    switch (node->type) {
        case NODE_PROGRAM:
            check_node(node->child1); // Lista de declarações globais
            check_node(node->child2); // Bloco 'programa'
            break;

        case NODE_BLOCK:
            symtab_enter_scope();
            check_node(node->child1); // Lista de declarações locais
            check_node(node->child2); // Lista de comandos
            symtab_leave_scope();
            break;

        case NODE_VAR_DECL: {
            DataType type = node->child1->int_val;
            AstNode* id_node = node->child2;
            while (id_node) {
                if (!symtab_insert(id_node->str_val, SYMBOL_VAR, type, 0)) {
                    report_error("Variavel redeclarada no mesmo escopo.", id_node->lineno);
                }
                id_node = id_node->next;
            }
            break;
        }
        
        case NODE_FUNC_DECL: {
            DataType return_type = node->child1->int_val;
            char* func_name = node->child2->str_val;
            AstNode* func_block = node->child3;

            SymbolNode* func_symbol = symtab_insert(func_name, SYMBOL_FUNC, return_type, 0);
            if (!func_symbol) {
                report_error("Funcao redeclarada.", node->lineno);
            }
            node->data_type = return_type;
            
            symtab_enter_scope();

            // --- parâmetros estão em func_block->child3
            AstNode* param_list = func_block->child3;   // NODE_PARAM_LIST encadeado
            if (param_list) {
                // registra na função
                if (func_symbol) {
                    func_symbol->param_list = param_list;       // <<< NOVO
                }
                // checa declarações de parâmetros
                check_node(param_list);
            }

            // conta parâmetros
            int arg_count = 0;
            AstNode* param_node = param_list;
            while(param_node) { arg_count++; param_node = param_node->next; }
            if(func_symbol) func_symbol->arg_count = arg_count;

            // declarações locais
            check_node(func_block->child1);
            // comandos
            check_node(func_block->child2);
            
            symtab_leave_scope();
            break;
        }
        
        case NODE_PARAM_LIST: {
            DataType type = node->child1->int_val;
            char* name = node->child2->str_val;
            if(!symtab_insert(name, SYMBOL_PARAM, type, 0)) {
                report_error("Parametro com nome duplicado.", node->lineno);
            }
            break;
        }

       case NODE_CALL: {
            // child1 = ID da função
            // child2 = lista de argumentos (NODE_ARG_LIST encadeado)
            check_node(node->child2); // checa tipos dos argumentos

            SymbolNode* symbol = symtab_lookup(node->child1->str_val);
            if (!symbol) {
                report_error("Funcao ou variavel nao declarada.", node->lineno);
                node->data_type = TYPE_UNDEFINED;
            } else if (symbol->category != SYMBOL_FUNC) {
                report_error("Identificador nao e uma funcao.", node->lineno);
                node->data_type = TYPE_UNDEFINED;
            } else {
                // compara quantidade
                int passed_args = 0;
                AstNode* arg = node->child2;
                while (arg) { passed_args++; arg = arg->next; }
                if (passed_args != symbol->arg_count) {
                    report_error("Numero incorreto de argumentos para a funcao.", node->lineno);
                }

                // compara tipos formais x reais
                AstNode* formal = symbol->param_list;  // NODE_PARAM_LIST: child1=Tipo, child2=ID
                AstNode* actual = node->child2;        // NODE_ARG_LIST: child1=Expr

                while (formal && actual) {
                    DataType formal_type = formal->child1->int_val;      // Tipo do parâmetro
                    DataType actual_type = actual->child1->data_type;    // Tipo da expressão-argumento

                    if (formal_type != TYPE_UNDEFINED &&
                        actual_type != TYPE_UNDEFINED &&
                        formal_type != actual_type) {

                        report_error("Parametro de chamada possui tipo diferente do parametro formal.", 
                                    actual->child1->lineno);
                    }

                    formal = formal->next;
                    actual = actual->next;
                }

                node->data_type = symbol->type;
            }
            break;
        }
        case NODE_ID: {
            SymbolNode* sym = symtab_lookup(node->str_val);
            if (!sym) {
                report_error("Identificador nao declarado.", node->lineno);
                node->data_type = TYPE_UNDEFINED;
            } else {
                node->data_type = sym->type;
            }
            break;
        }
        case NODE_ASSIGN:
            check_node(node->child1);
            check_node(node->child2);
            if (node->child1->data_type != TYPE_UNDEFINED && node->child2->data_type != TYPE_UNDEFINED &&
                node->child1->data_type != node->child2->data_type) {
                report_error("Tipos incompativeis na atribuicao.", node->lineno);
            }
            node->data_type = node->child1->data_type;
            break;
        case NODE_SUB: case NODE_MUL: case NODE_DIV: case NODE_ADD:
            check_node(node->child1);
            check_node(node->child2);
            if (node->child1->data_type != TYPE_INT || node->child2->data_type != TYPE_INT) {
                report_error("Operadores aritmeticos exigem operandos do tipo int.", node->lineno);
                node->data_type = TYPE_UNDEFINED;
            } else {
                node->data_type = TYPE_INT;
            }
            break;
        case NODE_GT: case NODE_LT: case NODE_EQ: case NODE_NEQ: case NODE_GTE: case NODE_LTE:
            check_node(node->child1);
            check_node(node->child2);
            if (node->child1->data_type != TYPE_UNDEFINED && node->child2->data_type != TYPE_UNDEFINED &&
                node->child1->data_type != node->child2->data_type) {
                report_error("Operadores relacionais exigem operandos do mesmo tipo.", node->lineno);
            }
            node->data_type = TYPE_INT;
            break;
        case NODE_IF:
        case NODE_WHILE:
            check_node(node->child1);
            if (node->child1->data_type != TYPE_INT && node->child1->data_type != TYPE_UNDEFINED) {
                report_error("Condicao deve ser do tipo int.", node->child1->lineno);
            }
            check_node(node->child2);
            check_node(node->child3);
            break;
        case NODE_INTCONST: node->data_type = TYPE_INT; break;
        case NODE_CARCONST: node->data_type = TYPE_CAR; break;
        case NODE_ARG_LIST:
             check_node(node->child1);
             if (node->child1) node->data_type = node->child1->data_type;
             break;
        default:
            check_node(node->child1);
            check_node(node->child2);
            check_node(node->child3);
            break;
    }

    // A recursão para o próximo item da lista é feita aqui no final.
    if (node->next) {
        check_node(node->next);
    }
}

int check_semantics(AstNode *root) {
    symtab_init();
    check_node(root);
    symtab_destroy();
    return semantic_errors > 0;
}