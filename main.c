#include <stdio.h>
#include "ast.h"
#include "semantic.h"
#include "codegen.h"

// Declarações externas
extern FILE *yyin;
extern int yyparse(void);
extern AstNode *ast_root;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_entrada.g>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror(argv[1]);
        return 1;
    }

    printf("1. Iniciando analise sintatica...\n");
    if (yyparse() != 0) {
        fprintf(stderr, "Falha na analise sintatica. Compilacao abortada.\n");
        return 1;
    }
    printf("   Analise sintatica concluida. AST gerada.\n");
    
    // Descomente para depurar a AST:
    // print_ast(ast_root, 0);

    printf("2. Iniciando analise semantica...\n");
    if (check_semantics(ast_root)) {
        fprintf(stderr, "Foram encontrados erros semanticos. Compilacao abortada.\n");
        free_ast(ast_root);
        return 1;
    }
    printf("   Analise semantica concluida. Nenhum erro encontrado.\n");
    
    printf("3. Iniciando geracao de codigo MIPS...\n");
    FILE *outfile = fopen("output.s", "w");
    if (!outfile) {
        perror("output.s");
        free_ast(ast_root);
        return 1;
    }
    
    generate_code(ast_root, outfile);
    fclose(outfile);
    printf("   Geracao de codigo concluida. Arquivo 'output.s' criado.\n");

    free_ast(ast_root);
    fclose(yyin);
    
    printf("\nCompilacao bem-sucedida!\n");
    return 0;
}