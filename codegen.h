#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

// Função principal da geração de código
void generate_code(AstNode *ast_root, FILE *output_file);

#endif