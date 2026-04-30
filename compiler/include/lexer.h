/* ============================================================
   lexer.h  —  Token definitions for the Mini Compiler
   ============================================================ */
#ifndef LEXER_H
#define LEXER_H

/* ----------------------------------------------------------
   Token Categories
   ---------------------------------------------------------- */
typedef enum {
    TOKEN_KEYWORD,      /* int, float, char, bool … */
    TOKEN_IDENTIFIER,   /* variable names            */
    TOKEN_NUMBER,       /* integers and floats       */
    TOKEN_OPERATOR,     /* + - * / =                 */
    TOKEN_SYMBOL,       /* ; ( ) { }                 */
    TOKEN_EOF,          /* end of input              */
    TOKEN_ERROR         /* unrecognised character    */
} TokenType;

/* ----------------------------------------------------------
   Token Data Structure
   ---------------------------------------------------------- */
typedef struct {
    TokenType type;
    char      lexeme[64];
    int       line;
} Token;

/* ----------------------------------------------------------
   Public API
   ---------------------------------------------------------- */
void        initLexer         (const char* input);
Token       getNextToken      (void);
const char* tokenTypeToString (TokenType type);

#endif /* LEXER_H */
