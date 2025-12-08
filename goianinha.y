%code requires {
    #include "ast.h" // Para o .h
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h" // <<< ADICIONE DE VOLTA AQUI, para o .c

// Protótipos
int yylex();
void yyerror(const char *s);

// Variáveis externas
extern int yylineno;
extern char* yytext;

// <<< NOVO: Ponteiro global para a raiz da árvore gerada
AstNode *ast_root = NULL;
%}

/* 
 * O resto do arquivo permanece igual...
 */

/* 
 * <<< MUDANÇA: O %union agora guarda ponteiros para nós da AST, além dos
 * valores antigos.
 */
%union { 
    char *str;
    int val;
    AstNode *node; 
}

/* <<< MUDANÇA: Associações de tipo para terminais e não-terminais */

/* Terminais que retornam valores brutos */
%token <str> T_ID T_CADEIACARACTERES
%token <val> T_INTCONST
%token <str> T_CARCONST // O lexer ainda manda como string (ex: "'a'")

/* Tokens sem valor associado */
%token T_PROGRAMA T_CAR T_INT T_RETORNE T_LEIA T_ESCREVA T_NOVALINHA
%token T_SE T_ENTAO T_SENAO T_ENQUANTO T_EXECUTE
%token T_OU T_E T_IGUAL T_DIFERENTE T_MENORIGUAL T_MAIORIGUAL

/* <<< MUDANÇA: TODOS os não-terminais agora produzem um ponteiro para AstNode */
%type <node> Programa DeclFuncVar DeclProg Bloco DeclVar DeclFunc
%type <node> ListaParametros ListaParametrosCont ListaDeclVar Tipo
%type <node> ListaComando Comando Expr OrExpr AndExpr EqExpr DesigExpr
%type <node> AddExpr MulExpr UnExpr LValueExpr PrimExpr ListExpr

/* Precedência e associatividade (sem mudanças) */
%right '='
%left T_OU
%left T_E
%left T_IGUAL T_DIFERENTE
%left '<' '>' T_MENORIGUAL T_MAIORIGUAL
%left '+' '-'
%left '*' '/'
%right '!' T_UNARY_MINUS

%start Programa

%%

/* --- Regras da Gramática com Ações Semânticas para Construir a AST --- */

Programa:
    DeclFuncVar DeclProg 
    { 
        ast_root = create_node(NODE_PROGRAM, $1, $2, NULL, 1); 
    }
    ;

DeclFuncVar:
    /* Vazio */                                     { $$ = NULL; }
    | DeclFuncVar Tipo T_ID DeclVar ';'             { AstNode* decl = create_node(NODE_VAR_DECL, $2, create_id_node($3, yylineno), $4, yylineno); free($3); $$ = append_to_list($1, decl); }
    | DeclFuncVar Tipo T_ID DeclFunc                { AstNode* decl = create_node(NODE_FUNC_DECL, $2, create_id_node($3, yylineno), $4, yylineno); free($3); $$ = append_to_list($1, decl); }
    ;

DeclProg:
    T_PROGRAMA Bloco                                { $$ = $2; }
    ;

DeclVar:
    /* Vazio */                                     { $$ = NULL; }
    | ',' T_ID DeclVar                              { $$ = create_id_node($2, yylineno); free($2); $$->next = $3; }
    ;

DeclFunc:
    '(' ListaParametros ')' Bloco { 
                                      $4->child3 = $2; // Armazena a lista de parâmetros no child3 do Bloco
                                      $$ = $4; 
                                    }
    ;

ListaParametros:
    /* Vazio */                                     { $$ = NULL; }
    | ListaParametrosCont                           { $$ = $1; }
    ;

ListaParametrosCont:
    Tipo T_ID                                       { AstNode* id = create_id_node($2, yylineno); free($2); $$ = create_node(NODE_PARAM_LIST, $1, id, NULL, yylineno); }
    | Tipo T_ID ',' ListaParametrosCont             { AstNode* id = create_id_node($2, yylineno); free($2); $$ = create_node(NODE_PARAM_LIST, $1, id, NULL, yylineno); $$->next = $4; }
    ;

Bloco:
    '{' ListaDeclVar ListaComando '}'               { $$ = create_node(NODE_BLOCK, $2, $3, NULL, yylineno); }
    | '{' ListaDeclVar '}'                          { $$ = create_node(NODE_BLOCK, $2, NULL, NULL, yylineno); }
    ;

ListaDeclVar:
    /* Vazio */                                     { $$ = NULL; }
| Tipo T_ID DeclVar ';' ListaDeclVar            { 
                                                  AstNode* first_id = create_id_node($2, yylineno);
                                                  first_id->next = $3; // <<< A CORREÇÃO CRÍTICA ESTÁ AQUI
                                                  AstNode* decl = create_node(NODE_VAR_DECL, $1, first_id, NULL, yylineno); 
                                                  free($2); 
                                                  decl->next = $5; 
                                                  $$ = decl; 
                                                }
    ;

Tipo: 
    T_INT                                           { $$ = create_node(NODE_TYPE, NULL, NULL, NULL, yylineno); $$->int_val = TYPE_INT; }
    | T_CAR                                         { $$ = create_node(NODE_TYPE, NULL, NULL, NULL, yylineno); $$->int_val = TYPE_CAR; }
    ;

ListaComando:
    /* Vazio */                                     { $$ = NULL; }
    | Comando ListaComando                          { if ($1) $1->next = $2; $$ = $1 ? $1 : $2; }
    ;

Comando:
    ';'                                             { $$ = NULL; }
    | Expr ';'                                      { $$ = $1; }
    | T_RETORNE Expr ';'                            { $$ = create_node(NODE_RETURN, $2, NULL, NULL, yylineno); }
    | T_LEIA LValueExpr ';'                         { $$ = create_node(NODE_READ, $2, NULL, NULL, yylineno); }
    | T_ESCREVA Expr ';'                            { $$ = create_node(NODE_WRITE, $2, NULL, NULL, yylineno); }
    | T_ESCREVA T_CADEIACARACTERES ';'              { $$ = create_node(NODE_WRITE, create_str_node($2, yylineno), NULL, NULL, yylineno); free($2); }
    | T_NOVALINHA ';'                               { $$ = create_node(NODE_NEWLINE, NULL, NULL, NULL, yylineno); }
    | T_SE '(' Expr ')' T_ENTAO Comando             { $$ = create_node(NODE_IF, $3, $6, NULL, yylineno); }
    | T_SE '(' Expr ')' T_ENTAO Comando T_SENAO Comando { $$ = create_node(NODE_IF, $3, $6, $8, yylineno); }
    | T_ENQUANTO '(' Expr ')' T_EXECUTE Comando     { $$ = create_node(NODE_WHILE, $3, $6, NULL, yylineno); }
    | Bloco                                         { $$ = $1; }
    ;

Expr: 
    OrExpr                                          { $$ = $1; } 
    | LValueExpr '=' Expr                           { $$ = create_node(NODE_ASSIGN, $1, $3, NULL, yylineno); }
    ;

OrExpr: 
    AndExpr                                         { $$ = $1; } 
    | OrExpr T_OU AndExpr                           { $$ = create_node(NODE_OR, $1, $3, NULL, yylineno); }
    ;

AndExpr: 
    EqExpr                                          { $$ = $1; }
    | AndExpr T_E EqExpr                            { $$ = create_node(NODE_AND, $1, $3, NULL, yylineno); }
    ;

EqExpr: 
    DesigExpr                                       { $$ = $1; }
    | EqExpr T_IGUAL DesigExpr                      { $$ = create_node(NODE_EQ, $1, $3, NULL, yylineno); }
    | EqExpr T_DIFERENTE DesigExpr                  { $$ = create_node(NODE_NEQ, $1, $3, NULL, yylineno); }
    ;

DesigExpr: 
    AddExpr                                         { $$ = $1; }
    | DesigExpr '<' AddExpr                         { $$ = create_node(NODE_LT, $1, $3, NULL, yylineno); }
    | DesigExpr '>' AddExpr                         { $$ = create_node(NODE_GT, $1, $3, NULL, yylineno); }
    | DesigExpr T_MAIORIGUAL AddExpr                { $$ = create_node(NODE_GTE, $1, $3, NULL, yylineno); }
    | DesigExpr T_MENORIGUAL AddExpr                { $$ = create_node(NODE_LTE, $1, $3, NULL, yylineno); }
    ;

AddExpr: 
    MulExpr                                         { $$ = $1; }
    | AddExpr '+' MulExpr                           { $$ = create_node(NODE_ADD, $1, $3, NULL, yylineno); }
    | AddExpr '-' MulExpr                           { $$ = create_node(NODE_SUB, $1, $3, NULL, yylineno); }
    ;

MulExpr: 
    UnExpr                                          { $$ = $1; }
    | MulExpr '*' UnExpr                            { $$ = create_node(NODE_MUL, $1, $3, NULL, yylineno); }
    | MulExpr '/' UnExpr                            { $$ = create_node(NODE_DIV, $1, $3, NULL, yylineno); }
    ;

UnExpr: 
    PrimExpr                                        { $$ = $1; }
    | '-' PrimExpr %prec T_UNARY_MINUS              { $$ = create_node(NODE_UNARY_MINUS, $2, NULL, NULL, yylineno); }
    | '!' PrimExpr                                  { $$ = create_node(NODE_NOT, $2, NULL, NULL, yylineno); }
    ;

LValueExpr: 
    T_ID                                            { $$ = create_id_node($1, yylineno); free($1); }
    ;

PrimExpr: 
    T_ID '(' ListExpr ')'                           { $$ = create_node(NODE_CALL, create_id_node($1, yylineno), $3, NULL, yylineno); free($1); }
    | T_ID '(' ')'                                  { $$ = create_node(NODE_CALL, create_id_node($1, yylineno), NULL, NULL, yylineno); free($1); }
    | LValueExpr                                    { $$ = $1; }
    | T_CARCONST                                    { $$ = create_char_node($1[1], yylineno); free($1); }
    | T_INTCONST                                    { $$ = create_int_node($1, yylineno); }
    | '(' Expr ')'                                  { $$ = $2; }
    ;

ListExpr: 
    Expr                                            { $$ = create_node(NODE_ARG_LIST, $1, NULL, NULL, $1->lineno); }
    | ListExpr ',' Expr                             { $$ = append_to_list($1, create_node(NODE_ARG_LIST, $3, NULL, NULL, $3->lineno)); }
    ;

%%

// A função de erro permanece a mesma, para reportar erros sintáticos.
void yyerror(const char *s) {
    fprintf(stderr, "ERRO SINTATICO: %s na linha %d, proximo de '%s'\n", s, yylineno, yytext);
}