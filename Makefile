CC=gcc
FLEX=flex
BISON=bison
CFLAGS=-Wall -g

TARGET=goianinha_compiler

# Todos os arquivos fonte C
SOURCES = goianinha.tab.c lex.yy.c ast.c symtab.c semantic.c codegen.c main.c
OBJS = $(SOURCES:.c=.o)

# Header gerado pelo Bison
BISON_H = goianinha.tab.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lfl

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

goianinha.tab.c $(BISON_H): goianinha.y ast.h
	$(BISON) -d -o goianinha.tab.c goianinha.y

lex.yy.c: goianinha.l $(BISON_H)
	$(FLEX) -o lex.yy.c goianinha.l

clean:
	rm -f $(TARGET) $(OBJS) goianinha.tab.c $(BISON_H) lex.yy.c output.s