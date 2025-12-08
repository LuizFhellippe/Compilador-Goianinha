#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"

static FILE *out;
static int label_count = 0;

// Gera um novo rótulo único
int new_label() {
    return label_count++;
}

// Função recursiva para gerar código para um nó
void gen_mips_for_node(AstNode* node);

void generate_code(AstNode *ast_root, FILE *output_file) {
    out = output_file;
    
    // Cabeçalho do arquivo assembly
    fprintf(out, ".data\n");
    // Aqui iriam as variáveis globais e strings
    fprintf(out, "newline: .asciiz \"\\n\"\n");

    fprintf(out, "\n.text\n");
    fprintf(out, ".globl main\n");

    // Gera código para o corpo do programa
    gen_mips_for_node(ast_root);
    
    // Finalização do programa
    fprintf(out, "\n# Fim do programa, sai\n");
    fprintf(out, "li $v0, 10\n");
    fprintf(out, "syscall\n");
}


void gen_mips_for_node(AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            // 'main' em MIPS é o nosso bloco 'programa'
            fprintf(out, "\nmain:\n");
            gen_mips_for_node(node->child2); // Gera o código do bloco 'programa'
            break;

        case NODE_BLOCK:
            // Gerenciamento de escopo e registradores (simplificado)
            gen_mips_for_node(node->child1); // Declarações (alocação na pilha)
            gen_mips_for_node(node->child2); // Comandos
            break;
        
        case NODE_STMT_LIST:
            gen_mips_for_node(node->child1);
            gen_mips_for_node(node->next);
            break;
            
        case NODE_WRITE:
            // Assumindo que a expressão a ser escrita é um inteiro
            gen_mips_for_node(node->child1);
            fprintf(out, "move $a0, $v0\n"); // Resultado da expressão já está em $v0
            fprintf(out, "li $v0, 1\n");     // Syscall para imprimir inteiro
            fprintf(out, "syscall\n");
            break;
            
        case NODE_NEWLINE:
            fprintf(out, "la $a0, newline\n");
            fprintf(out, "li $v0, 4\n");      // Syscall para imprimir string
            fprintf(out, "syscall\n");
            break;
            
        case NODE_INTCONST:
            fprintf(out, "li $v0, %d\n", node->int_val);
            break;
            
        case NODE_ADD:
            gen_mips_for_node(node->child1);
            fprintf(out, "sw $v0, 0($sp)\n"); // Empilha o primeiro operando
            fprintf(out, "addiu $sp, $sp, -4\n");
            gen_mips_for_node(node->child2);
            fprintf(out, "lw $t1, 4($sp)\n"); // Desempilha o primeiro operando em $t1
            fprintf(out, "addiu $sp, $sp, 4\n");
            fprintf(out, "add $v0, $t1, $v0\n"); // Soma e guarda em $v0
            break;
            
        case NODE_IF: {
            int else_label = new_label();
            int end_if_label = new_label();

            gen_mips_for_node(node->child1); // Condição
            fprintf(out, "beq $v0, $zero, L%d\n", else_label); // Se falso, pula para o else/fim
            
            gen_mips_for_node(node->child2); // Bloco 'then'
            fprintf(out, "j L%d\n", end_if_label);
            
            fprintf(out, "L%d:\n", else_label);
            if (node->child3) {
                gen_mips_for_node(node->child3); // Bloco 'else'
            }
            
            fprintf(out, "L%d:\n", end_if_label);
            break;
        }

        default:
            // Recursão genérica
            if (node->child1) gen_mips_for_node(node->child1);
            if (node->child2) gen_mips_for_node(node->child2);
            if (node->child3) gen_mips_for_node(node->child3);
            if (node->next) gen_mips_for_node(node->next);
            break;
    }
}