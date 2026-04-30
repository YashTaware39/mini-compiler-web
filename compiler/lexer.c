/* ============================================================
   lexer.c  —  DFA-based Lexical Analyser
   ============================================================
   Reads the source string character by character and groups
   characters into tokens (lexemes) using a Deterministic
   Finite Automaton (DFA) approach.

   REFACTORED:
     - Added makeToken() helper to eliminate repeated token
       construction code across all DFA states.
     - Added skipWhitespace() to separate whitespace handling
       from the main tokeniser logic.
     - Added peek() and nextChar() for cleaner character access.
   ============================================================ */
#include "include/lexer.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ----------------------------------------------------------
   Lexer State  (file-scoped — not visible outside lexer.c)
   ---------------------------------------------------------- */
static const char* src  = NULL;  /* pointer to source string  */
static int         pos  = 0;     /* current character index   */
static int         line = 1;     /* current line number       */

/* ----------------------------------------------------------
   initLexer  —  Reset state before each compilation
   ---------------------------------------------------------- */
void initLexer(const char* input) {
    src  = input;
    pos  = 0;
    line = 1;
}

/* ----------------------------------------------------------
   Character Helpers
   peek()     — look at current char without consuming
   nextChar() — consume current char and return it
   ---------------------------------------------------------- */
static char peek(void) {
    return src ? src[pos] : '\0';
}

static char nextChar(void) {
    return src ? src[pos++] : '\0';
}

/* ----------------------------------------------------------
   makeToken  —  Construct a token in one place
   Eliminates repeated field-assignment code across states.
   ---------------------------------------------------------- */
static Token makeToken(TokenType type, const char* lexeme) {
    Token t;
    t.type = type;
    t.line = line;
    strncpy(t.lexeme, lexeme, 63);
    t.lexeme[63] = '\0';
    return t;
}

/* ----------------------------------------------------------
   skipWhitespace  —  Advance past spaces, tabs, newlines
   ---------------------------------------------------------- */
static void skipWhitespace(void) {
    while (peek() == ' '  || peek() == '\t' ||
           peek() == '\n' || peek() == '\r') {
        if (peek() == '\n') line++;
        pos++;
    }
}

/* ----------------------------------------------------------
   isKeyword  —  Check whether an identifier is a keyword
   ---------------------------------------------------------- */
static int isKeyword(const char* word) {
    const char* keywords[] = {
        "int", "float", "char", "bool",
        "double", "long", "short", "void",
        "return", "if", "else", "while",
        NULL
    };
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(word, keywords[i]) == 0) return 1;
    }
    return 0;
}

/* ----------------------------------------------------------
   getNextToken  —  Main DFA: returns the next token

   Each if-block below corresponds to one DFA state:
     1. Identifier / Keyword
     2. Number (integer or float)
     3. Character literal  e.g. 'a'
     4. Operator  ( + - * / = )
     5. Symbol    ( ; ( ) { } )
     6. Error     (unrecognised character)
   ---------------------------------------------------------- */
Token getNextToken(void) {
    if (!src) return makeToken(TOKEN_EOF, "EOF");

    skipWhitespace();

    if (peek() == '\0') return makeToken(TOKEN_EOF, "EOF");

    char c = peek();
    char buf[64];   /* scratch buffer for building lexemes */

    /* --- STATE 1: Identifier or Keyword ---
       Starts with a letter or underscore, continues with
       letters, digits, or underscores.                      */
    if (isalpha(c) || c == '_') {
        int i = 0;
        while (isalnum(peek()) || peek() == '_') {
            if (i < 63) buf[i++] = nextChar();
            else pos++;  /* overflow guard: skip but don't store */
        }
        buf[i] = '\0';
        return makeToken(isKeyword(buf) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER, buf);
    }

    /* --- STATE 2: Integer or Floating-point Number ---
       Accepts digits and at most one decimal point.         */
    if (isdigit(c)) {
        int i = 0, hasDot = 0;
        while (isdigit(peek()) || (peek() == '.' && !hasDot)) {
            if (peek() == '.') hasDot = 1;
            if (i < 63) buf[i++] = nextChar();
            else pos++;
        }
        buf[i] = '\0';
        return makeToken(TOKEN_NUMBER, buf);
    }

    /* --- STATE 3: Character Literal  e.g. 'a' ---
       Stored as a NUMBER token for parser simplicity.       */
    if (c == '\'') {
        int i = 0;
        buf[i++] = nextChar();                    /* opening quote  */
        if (peek() != '\0' && peek() != '\'') {
            buf[i++] = nextChar();                /* the character  */
            if (peek() == '\'') {
                buf[i++] = nextChar();            /* closing quote  */
                buf[i]   = '\0';
                return makeToken(TOKEN_NUMBER, buf);
            }
        }
        buf[1] = '\0';  /* malformed literal */
        return makeToken(TOKEN_ERROR, buf);
    }

    /* --- STATE 4: Operator (including two-char relational ops) --- */
    if (c == '+' || c == '-' || c == '*' || c == '/' ||
        c == '=' || c == '<' || c == '>' || c == '!') {
        buf[0] = nextChar();
        buf[1] = '\0';
        /* peek ahead for two-char ops: == != <= >= */
        if (peek() == '=' && (buf[0]=='=' || buf[0]=='!' || buf[0]=='<' || buf[0]=='>')) {
            buf[1] = nextChar();
            buf[2] = '\0';
        }
        return makeToken(TOKEN_OPERATOR, buf);
    }

    /* --- STATE 5: Symbol --- */
    if (c == ';' || c == '(' || c == ')' || c == '{' || c == '}') {
        buf[0] = nextChar();
        buf[1] = '\0';
        return makeToken(TOKEN_SYMBOL, buf);
    }

    /* --- STATE 6: Unrecognised character --- */
    buf[0] = nextChar();
    buf[1] = '\0';
    return makeToken(TOKEN_ERROR, buf);
}

/* ----------------------------------------------------------
   tokenTypeToString  —  Human-readable token category
   ---------------------------------------------------------- */
const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD:    return "KEYWORD";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER:     return "NUMBER";
        case TOKEN_OPERATOR:   return "OPERATOR";
        case TOKEN_SYMBOL:     return "SYMBOL";
        case TOKEN_EOF:        return "EOF";
        case TOKEN_ERROR:      return "ERROR";
        default:               return "UNKNOWN";
    }
}
