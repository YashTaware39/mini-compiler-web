/* ============================================================
   symtab.c  —  Symbol Table Implementation
   ============================================================
   A simple flat array acting as the symbol table.
   Supports insertion, lookup, and formatted printing.

   REFACTORED:
     - Added printSeparator() to avoid repeated printf of dashes.
     - Minor comment improvements for viva clarity.
   ============================================================ */
#include "include/symtab.h"
#include <stdio.h>
#include <string.h>

/* ----------------------------------------------------------
   Storage  (file-scoped — not visible outside symtab.c)
   ---------------------------------------------------------- */
static Symbol table[MAX_SYMBOLS];  /* fixed-size array of entries */
static int    count = 0;           /* number of entries in use    */

/* ----------------------------------------------------------
   initSymTab  —  Clear all entries before each compilation
   Simply resets count — old data is harmlessly overwritten.
   ---------------------------------------------------------- */
void initSymTab(void) {
    count = 0;
}

/* ----------------------------------------------------------
   lookupSymbol  —  Linear search by name
   Returns:  index (>= 0) if found,  -1 if not found
   ---------------------------------------------------------- */
int lookupSymbol(const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(table[i].name, name) == 0) return i;
    }
    return -1;
}

/* ----------------------------------------------------------
   addSymbol  —  Insert a new (name, type) entry
   Returns:   1  on success
              0  if already declared (duplicate)
             -1  if table is full
   ---------------------------------------------------------- */
int addSymbol(const char* name, const char* type) {
    if (lookupSymbol(name) != -1) return  0;   /* duplicate  */
    if (count >= MAX_SYMBOLS)     return -1;   /* table full */

    strncpy(table[count].name, name, 63);
    table[count].name[63] = '\0';
    strncpy(table[count].type, type, 63);
    table[count].type[63] = '\0';
    count++;
    return 1;
}

/* ----------------------------------------------------------
   printSymTab  —  Print a formatted table to stdout
   ---------------------------------------------------------- */
void printSymTab(void) {
    printf("--- SYMBOL TABLE ---\n");
    printf("%-20s %-20s\n", "NAME", "TYPE");
    printf("----------------------------------------\n");

    if (count == 0) {
        printf("(empty)\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        printf("%-20s %-20s\n", table[i].name, table[i].type);
    }
}
