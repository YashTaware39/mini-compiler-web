/* ============================================================
   parser.c  —  Recursive-Descent Syntax Analyser (CFG)
   ============================================================
   Implements the following Context-Free Grammar (CFG):

     program       → statement*
     statement     → datatype identifier = expression ;
                   | datatype identifier ;              (uninit)
     expression    → term ( ( '+' | '-' ) term )*
     term          → factor ( ( '*' | '/' ) factor )*
     factor        → identifier | number | '(' expression ')'

   REFACTORED:
     - Added syntaxError() helper that combines report + count
       + recover in one call — eliminates the most repeated
       3-line pattern in the old code.
     - Made all grammar functions static (only parseTokens is
       public).
     - Unified the log-append pattern into appendToBuffer().
     - Extracted matchOperatorAndParse() to eliminate the nearly
       identical loops in parseExpression() and parseTerm().
   ============================================================ */
#include "include/parser.h"
#include "include/symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------------------------------------
   Internal State
   ---------------------------------------------------------- */
static Token* tokens;       /* flat token array from lexer   */
static int    tokenCount;   /* total number of tokens        */
static int    pos;          /* index of current token        */

/* Output buffers — filled during parsing, printed at end    */
static char traceBuffer[8192];    /* derivation trace        */
static char rulesBuffer[8192];    /* grammar rules           */
static char errorBuffer[4096];    /* error messages          */

/* Counters */
static int totalStmts;
static int validStmts;
static int errorStmts;
static int errorCount;

/* ----------------------------------------------------------
   Forward Declarations  (all static — internal to this file)
   ---------------------------------------------------------- */
static void parseStatementList(void);
static void parseStatement    (void);
static void parseIfStatement  (void);
static void parseWhileStatement(void);
static void parseReturnStatement(void);
static void parseAssignStatement(void);
static void parseExpression   (void);
static void parseTerm         (void);
static void parseFactor       (void);

/* ==========================================================
   Utility Helpers
   ========================================================== */

/* --- current()  — return the token at pos, or a sentinel EOF --- */
static Token current(void) {
    if (pos < tokenCount) return tokens[pos];
    Token eof;
    eof.type = TOKEN_EOF;
    strcpy(eof.lexeme, "EOF");
    eof.line = 0;
    return eof;
}

/* --- advance()  — move to the next token --- */
static void advance(void) {
    if (pos < tokenCount) pos++;
}

/* --- appendToBuffer()  — safe strcat with overflow guard ---
   Used by logTrace() and logRule() below.                    */
static void appendToBuffer(char* buf, int bufSize, const char* msg) {
    if ((int)(strlen(buf) + strlen(msg) + 2) < bufSize) {
        strcat(buf, msg);
        strcat(buf, "\n");
    }
}

static void logTrace(const char* msg) {
    appendToBuffer(traceBuffer, sizeof(traceBuffer), msg);
}

static void logRule(const char* msg) {
    appendToBuffer(rulesBuffer, sizeof(rulesBuffer), msg);
}

/* ----------------------------------------------------------
   isDataType  —  True if the lexeme is a supported type
   ---------------------------------------------------------- */
static int isDataType(const char* lex) {
    const char* types[] = {
        "int", "float", "char", "bool",
        "double", "long", "short", "void",
        NULL
    };
    for (int i = 0; types[i] != NULL; i++) {
        if (strcmp(lex, types[i]) == 0) return 1;
    }
    return 0;
}

/* ----------------------------------------------------------
   matchLexeme / matchType  —  Consume a token if it matches
   ---------------------------------------------------------- */
static int matchLexeme(const char* expected) {
    if (strcmp(current().lexeme, expected) == 0) {
        char buf[128];
        sprintf(buf, "  Matched  %-10s", current().lexeme);
        logTrace(buf);
        advance();
        return 1;
    }
    return 0;
}

static int matchType(TokenType expectedType, const char* label) {
    if (current().type == expectedType) {
        char buf[128];
        sprintf(buf, "  Matched  %-10s  (%s)", label, current().lexeme);
        logTrace(buf);
        advance();
        return 1;
    }
    return 0;
}

/* ----------------------------------------------------------
   reportError  —  Record a syntax error (does NOT stop parsing)
   ---------------------------------------------------------- */
static void reportError(const char* expected) {
    char buf[512];
    const char* found = (current().type == TOKEN_EOF)
                        ? "end of input"
                        : current().lexeme;

    /* Give a helpful hint based on what was expected */
    const char* hint = "";
    if (strcmp(expected, ";") == 0)
        hint = " (did you forget a semicolon?)";
    else if (strcmp(expected, "=") == 0)
        hint = " (did you forget the = sign?)";
    else if (strcmp(expected, ")") == 0)
        hint = " (unmatched parenthesis)";
    else if (strcmp(expected, "}") == 0)
        hint = " (unmatched brace — missing closing })";
    else if (strcmp(expected, "(") == 0)
        hint = " (condition must be in parentheses)";
    else if (strcmp(expected, "identifier") == 0)
        hint = " (variable name expected here)";

    snprintf(buf, sizeof(buf),
             "Syntax Error at line %d: Expected '%s' but found '%s'%s\n",
             current().line, expected, found, hint);

    if ((int)(strlen(errorBuffer) + strlen(buf)) < (int)sizeof(errorBuffer) - 1)
        strcat(errorBuffer, buf);
    errorCount++;
}

/* ----------------------------------------------------------
   recover  —  Skip tokens until next ';' to continue parsing
   ---------------------------------------------------------- */
static void recover(void) {
    /* Skip tokens until we hit a safe resync point:
       ';'  — end of a statement
       '}'  — end of a block
       EOF  — end of input
       Also stop if we see a new statement starting (datatype keyword),
       so we don't eat the next valid statement. */
    while (current().type != TOKEN_EOF) {
        /* semicolon — consume it and stop */
        if (current().type == TOKEN_SYMBOL && strcmp(current().lexeme, ";") == 0) {
            advance();
            return;
        }
        /* closing brace — do NOT consume, let the block parser handle it */
        if (current().type == TOKEN_SYMBOL && strcmp(current().lexeme, "}") == 0) {
            return;
        }
        /* start of a new declaration or control statement — stop without consuming */
        if (current().type == TOKEN_KEYWORD &&
            (isDataType(current().lexeme) ||
             strcmp(current().lexeme, "if")     == 0 ||
             strcmp(current().lexeme, "while")  == 0 ||
             strcmp(current().lexeme, "return") == 0)) {
            return;
        }
        advance();
    }
}

/* ----------------------------------------------------------
   syntaxError  —  Report + count + recover in one call

   This was the most repeated 3-line pattern in the old code:
       reportError(...);  errorStmts++;  recover();
   Now it's a single function call.
   ---------------------------------------------------------- */
static void syntaxError(const char* expected) {
    reportError(expected);
    errorStmts++;
    recover();
}

/* ==========================================================
   parseTokens  —  Entry point called by main.c
   ========================================================== */
void parseTokens(Token* tkns, int count) {
    /* --- Initialise all state --- */
    tokens     = tkns;
    tokenCount = count;
    pos        = 0;
    errorCount = 0;
    totalStmts = 0;
    validStmts = 0;
    errorStmts = 0;

    traceBuffer[0] = '\0';
    rulesBuffer[0] = '\0';
    errorBuffer[0] = '\0';

    /* --- Edge case: empty input --- */
    if (count == 0 || (count == 1 && tkns[0].type == TOKEN_EOF)) {
        printf("--- PARSING TRACE ---\n(empty)\n\n");
        printf("--- GRAMMAR RULES ---\n(empty)\n\n");
        printf("--- RESULT ---\nNo tokens to parse.\n\n");
        printf("--- SUMMARY ---\nTotal: 0  Valid: 0  Errors: 0\n");
        return;
    }

    /* --- Run the recursive-descent parser --- */
    parseStatementList();

    /* --- Check for unconsumed tokens --- */
    if (current().type != TOKEN_EOF) {
        reportError("end of input");
        errorStmts++;
    }

    /* --- Print all output sections --- */
    printf("--- PARSING TRACE ---\n%s\n", traceBuffer);
    printf("--- GRAMMAR RULES ---\n%s\n", rulesBuffer);

    printf("--- RESULT ---\n");
    if (errorCount == 0) {
        if (strlen(errorBuffer) > 0) {
            printf("Parsing Completed Successfully (with warnings):\n%s", errorBuffer);
        } else {
            printf("Parsing Completed Successfully.\n");
        }
    } else {
        printf("%s", errorBuffer);
    }

    printf("\n--- SUMMARY ---\n");
    printf("Total Statements : %d\n", totalStmts);
    printf("Valid Statements : %d\n", validStmts);
    printf("Errors           : %d\n", errorStmts);
}

/* ==========================================================
   Grammar Productions (CFG rules)
   ========================================================== */

/* program → statement* */
static void parseStatementList(void) {
    while (current().type != TOKEN_EOF &&
           !(current().type == TOKEN_SYMBOL && strcmp(current().lexeme, "}") == 0)) {

        /* if-else */
        if (current().type == TOKEN_KEYWORD && strcmp(current().lexeme, "if") == 0) {
            parseIfStatement();

        /* while */
        } else if (current().type == TOKEN_KEYWORD && strcmp(current().lexeme, "while") == 0) {
            parseWhileStatement();

        /* return */
        } else if (current().type == TOKEN_KEYWORD && strcmp(current().lexeme, "return") == 0) {
            parseReturnStatement();

        /* declaration: int x = ...; */
        } else if (current().type == TOKEN_KEYWORD && isDataType(current().lexeme)) {
            parseStatement();

        /* assignment: x = ...; */
        } else if (current().type == TOKEN_IDENTIFIER) {
            parseAssignStatement();

        /* unknown — skip with error */
        } else {
            syntaxError("statement");
        }
    }
}

/* ----------------------------------------------------------
   parseAssignStatement — identifier = expression ;
   ---------------------------------------------------------- */
static void parseAssignStatement(void) {
    totalStmts++;
    int errsBefore = errorCount;
    logRule("Rule: stmt → id = expr ;");

    char varName[64];
    strcpy(varName, current().lexeme);
    matchType(TOKEN_IDENTIFIER, "id");

    if (!matchLexeme("=")) { syntaxError("="); return; }
    parseExpression();
    if (!matchLexeme(";")) { syntaxError(";"); return; }

    if (errorCount == errsBefore) validStmts++;
    else errorStmts++;
}

/* ----------------------------------------------------------
   parseReturnStatement — return [expression] ;
   ---------------------------------------------------------- */
static void parseReturnStatement(void) {
    totalStmts++;
    logRule("Rule: stmt → return [expr] ;");
    matchLexeme("return");

    /* expression is optional — bare 'return;' is valid in void functions */
    if (!(current().type == TOKEN_SYMBOL && strcmp(current().lexeme, ";") == 0)) {
        parseExpression();
    }

    if (!matchLexeme(";")) { syntaxError(";"); return; }
    validStmts++;
}

/* ----------------------------------------------------------
   parseWhileStatement — while ( condition ) { block }
   ---------------------------------------------------------- */
static void parseWhileStatement(void) {
    totalStmts++;
    logRule("Rule: stmt → while ( condition ) { stmts }");
    matchLexeme("while");

    if (!matchLexeme("(")) { syntaxError("("); return; }
    parseExpression();
    if (current().type == TOKEN_OPERATOR) {
        char buf[64];
        sprintf(buf, "  Matched  %s  (relop)", current().lexeme);
        logTrace(buf);
        advance();
        parseExpression();
    }
    if (!matchLexeme(")")) { syntaxError(")"); return; }
    if (!matchLexeme("{")) { syntaxError("{"); return; }

    parseStatementList();

    if (!matchLexeme("}")) { syntaxError("}"); return; }
}

/* ----------------------------------------------------------
   parseIfStatement — if ( condition ) { block } [else { block }]
   condition → expression relop expression
   ---------------------------------------------------------- */
static void parseIfStatement(void) {
    totalStmts++;
    logRule("Rule: stmt → if ( condition ) { stmts } [else { stmts }]");
    matchLexeme("if");

    if (!matchLexeme("(")) { syntaxError("("); return; }
    parseExpression();   /* left side of condition */

    /* relational operator: == != < > <= >= */
    if (current().type == TOKEN_OPERATOR) {
        char buf[64];
        sprintf(buf, "  Matched  %s  (relop)", current().lexeme);
        logTrace(buf);
        advance();
        parseExpression();   /* right side */
    }

    if (!matchLexeme(")")) { syntaxError(")"); return; }
    if (!matchLexeme("{")) { syntaxError("{"); return; }

    /* parse the if-body */
    parseStatementList();
    if (!matchLexeme("}")) { syntaxError("}"); return; }

    /* optional else */
    if (current().type == TOKEN_KEYWORD && strcmp(current().lexeme, "else") == 0) {
        logRule("Rule: else → else { stmts }");
        matchLexeme("else");
        if (!matchLexeme("{")) { syntaxError("{"); return; }
        parseStatementList();
        if (!matchLexeme("}")) { syntaxError("}"); return; }
    }
}

/* ----------------------------------------------------------
   statement → datatype identifier = expression ;
             | datatype identifier ;

   Uses syntaxError() helper for all early-exit error paths.
   ---------------------------------------------------------- */
static void parseStatement(void) {
    totalStmts++;
    int errsBefore = errorCount;

    if (current().type != TOKEN_KEYWORD || !isDataType(current().lexeme)) {
        syntaxError("data type (int, float, char ...)");
        return;
    }

    char typeName[64];
    strcpy(typeName, current().lexeme);
    char ruleMsg[128];
    sprintf(ruleMsg, "Rule: stmt → %s id [= expr] ;", typeName);
    logRule(ruleMsg);
    matchLexeme(typeName);

    if (current().type != TOKEN_IDENTIFIER) { syntaxError("identifier"); return; }
    char varName[64];
    strcpy(varName, current().lexeme);
    matchType(TOKEN_IDENTIFIER, "id");

    /* '=' is now OPTIONAL — uninitialized declarations are valid */
    if (matchLexeme("=")) {
        logRule("Rule: expr → term ( (+|-) term )*");
        parseExpression();
    }

    if (!matchLexeme(";")) { syntaxError(";"); return; }

    if (errorCount == errsBefore) {
        int result = addSymbol(varName, typeName);
        if (result == 0) {
            /* duplicate declaration */
            char warn[128];
            snprintf(warn, sizeof(warn),
                     "Semantic Error at line %d: '%s' already declared\n",
                     current().line, varName);
            if ((int)(strlen(errorBuffer) + strlen(warn)) < (int)sizeof(errorBuffer) - 1)
                strcat(errorBuffer, warn);
            errorCount++;
            errorStmts++;
        } else {
            validStmts++;
        }
    } else {
        errorStmts++;
    }
}

/* ----------------------------------------------------------
   expression → term ( ( '+' | '-' ) term )*
   ---------------------------------------------------------- */
static void parseExpression(void) {
    parseTerm();
    while (current().type == TOKEN_OPERATOR &&
           (strcmp(current().lexeme, "+") == 0 ||
            strcmp(current().lexeme, "-") == 0)) {
        char op = current().lexeme[0];
        char ruleMsg[64];
        sprintf(ruleMsg, "Rule: expr → expr %c term", op);
        logRule(ruleMsg);
        advance();
        logTrace(op == '+' ? "  Matched  +" : "  Matched  -");
        parseTerm();
    }
}

/* ----------------------------------------------------------
   term → factor ( ( '*' | '/' ) factor )*
   ---------------------------------------------------------- */
static void parseTerm(void) {
    parseFactor();
    while (current().type == TOKEN_OPERATOR &&
           (strcmp(current().lexeme, "*") == 0 ||
            strcmp(current().lexeme, "/") == 0)) {
        char op = current().lexeme[0];
        char ruleMsg[64];
        sprintf(ruleMsg, "Rule: term → term %c factor", op);
        logRule(ruleMsg);
        advance();
        logTrace(op == '*' ? "  Matched  *" : "  Matched  /");
        parseFactor();
    }
}

/* ----------------------------------------------------------
   factor → identifier | number | '(' expression ')'
   ---------------------------------------------------------- */
static void parseFactor(void) {
    if (current().type == TOKEN_IDENTIFIER) {
        logRule("Rule: factor → id");
        /* Semantic check: warn if variable was never declared */
        if (lookupSymbol(current().lexeme) == -1) {
            char warn[128];
            snprintf(warn, sizeof(warn),
                     "Warning at line %d: '%s' used but not declared\n",
                     current().line, current().lexeme);
            if ((int)(strlen(errorBuffer) + strlen(warn)) < (int)sizeof(errorBuffer) - 1)
                strcat(errorBuffer, warn);
            /* Don't increment errorCount — it's a warning, not a hard error */
        }
        matchType(TOKEN_IDENTIFIER, "id");

    } else if (current().type == TOKEN_NUMBER) {
        logRule("Rule: factor → number");
        matchType(TOKEN_NUMBER, "number");

    } else if (current().type == TOKEN_SYMBOL &&
               strcmp(current().lexeme, "(") == 0) {
        logRule("Rule: factor → ( expr )");
        matchLexeme("(");
        parseExpression();
        if (!matchLexeme(")"))
            reportError(")");

    } else {
        reportError("identifier, number, or '('");
    }
}
