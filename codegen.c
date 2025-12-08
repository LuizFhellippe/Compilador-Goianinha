#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "symtab.h"

static FILE *out;
static int label_count = 0;
static int current_frame_offset = 0;
#define FRAME_OFFSET_STEP 4

// Protótipos
void gen_mips_for_node(AstNode* node);

int new_label() {
    return label_count++;
}

void generate_code(AstNode *ast_root, FILE *output_file) {
    out = output_file;
    
    // =======================================================
    // PASSO 1: GERAÇÃO DA SEÇÃO .data PARA VARIÁVEIS GLOBAIS
    // =======================================================
    fprintf(out, ".data\n");
    fprintf(out, "newline: .asciiz \"\\n\"\n");

    AstNode* decl = ast_root->child1;
    while (decl) {
        if (decl->type == NODE_VAR_DECL) {
            AstNode* id_node = decl->child2;
            while (id_node) {
                fprintf(out, "%s: .word 0\n", id_node->str_val);
                id_node = id_node->next;
            }
        }
        decl = decl->next;
    }

    fprintf(out, "\n.text\n");
    fprintf(out, ".globl main\n");

    // =======================================================
    // PASSO 2: GERAÇÃO DE CÓDIGO PARA TODAS AS FUNÇÕES E MAIN
    // =======================================================
    gen_mips_for_node(ast_root);
    
    // Adiciona a finalização após o código de main ter sido gerado
    fprintf(out, "\n# Fim do programa, sai\n");
    fprintf(out, "li $v0, 10\n");
    fprintf(out, "syscall\n");
}

void gen_prologue(int frame_size) {
    fprintf(out, "addiu $sp, $sp, -%d\n", frame_size); // Aloca espaço no frame
    fprintf(out, "sw $ra, %d($sp)\n", frame_size - 4); // Salva o endereço de retorno
    fprintf(out, "sw $fp, %d($sp)\n", frame_size - 8); // Salva o frame pointer antigo
    fprintf(out, "addiu $fp, $sp, %d\n", frame_size);   // $fp aponta para o topo do novo frame
}

void gen_epilogue(int frame_size) {
    fprintf(out, "lw $ra, -4($fp)\n");     // Restaura $ra
    fprintf(out, "lw $fp, -8($fp)\n");     // Restaura $fp
    fprintf(out, "addiu $sp, $sp, %d\n", frame_size);  // Desaloca o frame
    fprintf(out, "jr $ra\n\n");                        // Retorna
}

void gen_mips_for_node(AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            // Gera código para as funções primeiro
            gen_mips_for_node(node->child1);
            // Depois, gera o código para o bloco 'programa' (main)
            fprintf(out, "\nmain:\n");
            // Vamos estimar um tamanho de frame para main. Uma análise real calcularia isso.
            gen_prologue(100); 
            current_frame_offset = 0; // Reset para o frame de main
            gen_mips_for_node(node->child2);
            break;

        case NODE_FUNC_DECL:
        {
            char* func_name = node->child2->str_val;
            fprintf(out, "\n%s:\n", func_name);
            // Tamanho do frame estimado. Uma análise real calcularia o espaço exato.
            int frame_size = 100;
            gen_prologue(frame_size);
            current_frame_offset = 0; // Reset para o frame da nova função
            gen_mips_for_node(node->child3); // Gera código para o corpo da função
            // Gera um retorno padrão caso não haja 'retorne' explícito no final
            gen_epilogue(frame_size);
            gen_mips_for_node(node->next); // Continua para a próxima função/declaração global
            break;
        }

        case NODE_BLOCK:
            // O escopo de variáveis já é tratado pela função que contém o bloco
            gen_mips_for_node(node->child1); // Declarações (para mapear offsets)
            gen_mips_for_node(node->child2); // Comandos
            break;

        case NODE_VAR_DECL:
        {
            // Mapeia offsets para variáveis LOCAIS
            AstNode* id_node = node->child2;
            while (id_node) {
                SymbolNode* symbol = symtab_lookup(id_node->str_val);
                if (symbol && symbol->scope_level > 0) {
                    current_frame_offset += FRAME_OFFSET_STEP;
                    symbol->declaration_pos = -current_frame_offset;
                }
                id_node = id_node->next;
            }
            gen_mips_for_node(node->next);
            break;
        }

        case NODE_ASSIGN:
        {
            gen_mips_for_node(node->child2);
            SymbolNode* symbol = symtab_lookup(node->child1->str_val);
            if (symbol) {
                if (symbol->scope_level == 0) {
                    fprintf(out, "sw $v0, %s\n", symbol->id_name);
                } else {
                    fprintf(out, "sw $v0, %d($fp)\n", symbol->declaration_pos);
                }
            }
            gen_mips_for_node(node->next); // <<< ADICIONE ESTA LINHA
            break;
        }

        case NODE_WRITE:
            if (node->child1->type == NODE_STRCONST) {
                // ... (lógica para string)
            } else {
                gen_mips_for_node(node->child1);
                // Precisa verificar o tipo para usar a syscall correta
                if (node->child1->data_type == TYPE_CAR) {
                    fprintf(out, "move $a0, $v0\n"); 
                    fprintf(out, "li $v0, 11\n"); // Syscall para imprimir char
                    fprintf(out, "syscall\n");
                } else { // Assume int
                    fprintf(out, "move $a0, $v0\n"); 
                    fprintf(out, "li $v0, 1\n");     // Syscall para imprimir int
                    fprintf(out, "syscall\n");
                }
            }
            gen_mips_for_node(node->next); // <<< ADICIONE ESTA LINHA
            break;
        
        case NODE_NEWLINE:
            fprintf(out, "la $a0, newline\n");
            fprintf(out, "li $v0, 4\n");
            fprintf(out, "syscall\n");
            gen_mips_for_node(node->next); // <<< ADICIONE ESTA LINHA
            break;
            
        case NODE_READ:
             // Syscall para ler inteiro
            fprintf(out, "li $v0, 5\n");
            fprintf(out, "syscall\n");
            // O valor lido está em $v0. Agora, armazene-o na variável.
            SymbolNode* symbol = symtab_lookup(node->child1->str_val);
            if (symbol) {
                if (symbol->scope_level == 0) {
                    fprintf(out, "sw $v0, %s\n", symbol->id_name);
                } else {
                    fprintf(out, "sw $v0, %d($fp)\n", symbol->declaration_pos);
                }
            }
            gen_mips_for_node(node->next); // <<< ADICIONE ESTA LINHA
            break;

        case NODE_IF: {
            int else_label = new_label();
            int end_if_label = new_label();

            gen_mips_for_node(node->child1); // Condição
            fprintf(out, "beq $v0, $zero, L%d\n", else_label);
            
            gen_mips_for_node(node->child2); // Bloco 'then'
            fprintf(out, "j L%d\n", end_if_label);
            
            fprintf(out, "L%d:\n", else_label);
            if (node->child3) {
                gen_mips_for_node(node->child3); // Bloco 'else'
            }
            
            fprintf(out, "L%d:\n", end_if_label);
            gen_mips_for_node(node->next); // <<< ADICIONE ESTA LINHA
            break;
        }

        // Também precisamos implementar os operadores de comparação
        case NODE_LT: // Less Than
            gen_mips_for_node(node->child1);
            fprintf(out, "sw $v0, 0($sp)\n");
            fprintf(out, "addiu $sp, $sp, -4\n");
            gen_mips_for_node(node->child2);
            fprintf(out, "lw $t1, 4($sp)\n");
            fprintf(out, "addiu $sp, $sp, 4\n");
            // slt (set if less than): se $t1 < $v0, $v0 = 1, senão $v0 = 0
            fprintf(out, "slt $v0, $t1, $v0\n"); 
            break;

        case NODE_ID:
        {
            SymbolNode* symbol = symtab_lookup(node->str_val);
            if (symbol) {
                if (symbol->scope_level == 0) { // Global
                    fprintf(out, "lw $v0, %s\n", symbol->id_name);
                } else { // Local
                    fprintf(out, "lw $v0, %d($fp)\n", symbol->declaration_pos);
                }
            }
            break;
        }
        
        case NODE_CALL:
        {
            // Lógica de chamada de função (simplificada)
            // 1. Salvar registradores $t0-$t9 se necessário (não implementado)
            // 2. Passar argumentos
            // TODO: Implementar passagem de argumentos na pilha
            
            // 3. Chamar a função
            fprintf(out, "jal %s\n", node->child1->str_val);
            
            // 4. Restaurar registradores (não implementado)
            // O valor de retorno já estará em $v0
            break;
        }
        
        case NODE_RETURN:
            gen_mips_for_node(node->child1); // Calcula o valor de retorno, que vai para $v0
            fprintf(out, "jr $ra\n");
            break;

        // ... Os outros cases (ADD, SUB, IF, WRITE, etc.) permanecem os mesmos ...
        // ... mas adicione gen_mips_for_node(node->next); para comandos em lista ...
        
        default:
            gen_mips_for_node(node->child1);
            gen_mips_for_node(node->child2);
            gen_mips_for_node(node->child3);
            gen_mips_for_node(node->next);
            break;
    }
}