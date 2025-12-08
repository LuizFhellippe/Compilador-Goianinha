%{
#include <stdio.h>
#include <string.h>
#include "goianinha.tab.h"
void yyerror(const char *s);
%}

%option noyywrap

%x ML_COMMENT

DIGITO      [0-9]
LETRA       [a-zA-Z]

%%

"/*"                { BEGIN(ML_COMMENT); }
<ML_COMMENT>"*/"    { BEGIN(INITIAL); }
<ML_COMMENT>\n      { yylineno++; }
<ML_COMMENT><<EOF>> { fprintf(stderr, "ERRO: Comentario nao termina\n"); yyterminate(); }
<ML_COMMENT>.       { /* Ignora */ }

"programa"          { return T_PROGRAMA; }
"car"               { return T_CAR; }
"int"               { return T_INT; }
"retorne"           { return T_RETORNE; }
"leia"              { return T_LEIA; }
"escreva"           { return T_ESCREVA; }
"novalinha"         { return T_NOVALINHA; }
"se"                { return T_SE; }
"entao"             { return T_ENTAO; }
"senao"             { return T_SENAO; }
"enquanto"          { return T_ENQUANTO; }
"execute"           { return T_EXECUTE; }
"ou"                { return T_OU; }
"e"                 { return T_E; }

"=="                { return T_IGUAL; }
"!="                { return T_DIFERENTE; }
"<="                { return T_MENORIGUAL; }
">="                { return T_MAIORIGUAL; }

{LETRA}({LETRA}|{DIGITO}|_)* { yylval.str = strdup(yytext); return T_ID; }
{DIGITO}+                 { yylval.val = atoi(yytext); return T_INTCONST; }

\'[^\n\']\'               { yylval.str = strdup(yytext); return T_CARCONST; }

\"[^\"\n]*\"              { yylval.str = strdup(yytext); return T_CADEIACARACTERES; }

[=<>+\-*/!(){},;]   { return yytext[0]; }

[ \t\r]+             { /* Ignora espaco */ }
\n                  { yylineno++; }

.                   { fprintf(stderr, "ERRO: Caractere invalido '%s'\n", yytext); }

%%
