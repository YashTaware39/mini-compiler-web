/* ============================================================
   main.c  —  Mini Compiler Driver
   ============================================================
   Pipeline:
     1. Read source code from stdin
     2. Lexical Analysis  → produce token list
     3. Syntax Analysis   → recursive-descent parse
     4. Symbol Table      → print declared variables

   REFACTORED:
     - Extracted readSource() and printTokenTable() helpers so
       main() reads as a clean, 5-line pipeline.
     - Each helper is self-contained and easy to explain in a
       viva: "readSource reads from stdin", "printTokenTable
       outputs the tokenization results", etc.
   ============================================================ */
#include "include/lexer.h"
#include "include/parser.h"
#include "include/symtab.h"
#include <stdio.h>
#include <string.h>

#define MAX_SOURCE  10000
#define MAX_TOKENS  1000

/* ----------------------------------------------------------
   readSource  —  Read entire source code from stdin into buf
   Returns the number of characters read.
   ---------------------------------------------------------- */
static int readSource(char* buf, int maxLen) {
    char line[1024];
    buf[0] = '\0';

    while (fgets(line, sizeof(line), stdin)) {
        if ((int)(strlen(buf) + strlen(line)) < maxLen)
            strcat(buf, line);
    }
    return (int)strlen(buf);
}

/* ----------------------------------------------------------
   tokenize  —  Run lexer and fill the tokens array
   Returns the number of tokens produced (excluding EOF).
   ---------------------------------------------------------- */
static int tokenize(Token* tokenArr, int maxTokens) {
    int count = 0;
    Token t = getNextToken();

    while (t.type != TOKEN_EOF && count < maxTokens) {
        tokenArr[count++] = t;
        t = getNextToken();
    }
    return count;
}

/* ----------------------------------------------------------
   printTokenTable  —  Display all tokens in a formatted table
   ---------------------------------------------------------- */
static void printTokenTable(const Token* tokenArr, int count) {
    printf("--- TOKENS ---\n");
    printf("%-20s %-20s %-6s\n", "LEXEME", "TOKEN_TYPE", "LINE");
    printf("--------------------------------------------------\n");

    for (int i = 0; i < count; i++) {
        printf("%-20s %-20s %d\n",
               tokenArr[i].lexeme,
               tokenTypeToString(tokenArr[i].type),
               tokenArr[i].line);
    }
    printf("\n");
}

/* ==========================================================
   main  —  A clean, minimal pipeline

   Each step is a single function call, making it easy to
   explain in a viva:
     Step 1: Read input
     Step 2: Tokenize (Lexical Analysis / DFA)
     Step 3: Parse    (Syntax Analysis / CFG)
     Step 4: Display symbol table
   ========================================================== */
int main(void) {
    /* Step 1: Read source code */
    char source[MAX_SOURCE];
    readSource(source, MAX_SOURCE);

    /* Step 2: Lexical Analysis — tokenize using DFA */
    initLexer(source);
    initSymTab();

    Token tokens[MAX_TOKENS];
    int tokenCount = tokenize(tokens, MAX_TOKENS);

    printTokenTable(tokens, tokenCount);

    /* Step 3: Syntax Analysis — parse using CFG */
    parseTokens(tokens, tokenCount);
    printf("\n");

    /* Step 4: Print symbol table */
    printSymTab();

    return 0;
}
