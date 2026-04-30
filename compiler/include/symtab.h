/* ============================================================
   symtab.h  —  Symbol Table Interface
   ============================================================ */
#ifndef SYMTAB_H
#define SYMTAB_H

#define MAX_SYMBOLS 100

/* ----------------------------------------------------------
   One row in the symbol table
   ---------------------------------------------------------- */
typedef struct {
    char name[64];   /* variable name  */
    char type[64];   /* declared type  */
} Symbol;

/* ----------------------------------------------------------
   Public API
   ---------------------------------------------------------- */
void initSymTab  (void);
int  addSymbol   (const char* name, const char* type);
int  lookupSymbol(const char* name);
void printSymTab (void);

#endif /* SYMTAB_H */
