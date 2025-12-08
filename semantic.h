#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

// Função principal de verificação semântica
// Retorna 0 se não houver erros, 1 caso contrário.
int check_semantics(AstNode *ast_root);

#endif