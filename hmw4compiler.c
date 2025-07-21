//Name: Ameer Khalil
//Assignment: PL/0 Parser and Code Generator - HW4
//Date: 7/19/2025

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define NUM_KEYWORDS 14
#define MAX_SYMBOL_TABLE_SIZE 500
#define MAXLEVEL 3

typedef enum {
    LIT = 1,
    OPR = 2,
    LOD = 3,
    STO = 4,
    CAL = 5,
    INC = 6,
    JMP = 7,
    JPC = 8,
    SYS = 9
} OpCode;

typedef struct {
    OpCode op;
    int l;
    int m;
} Instruction;

int halt = 0;  // Flag to stop processing if errors occur

typedef enum {
    modsym = 1,      // Changed from oddsym to modsym
    identsym = 2,
    numbersym = 3,
    plussym = 4,
    minussym = 5,
    multsym = 6,
    slashsym = 7,
    fisym = 8,
    eqsym = 9,
    neqsym = 10,
    lessym = 11,
    leqsym = 12,
    gtrsym = 13,
    geqsym = 14,
    lparentsym = 15,
    rparentsym = 16,
    commasym = 17,
    semicolonsym = 18,
    periodsym = 19,
    becomessym = 20,
    beginsym = 21,
    endsym = 22,
    ifsym = 23,
    thensym = 24,
    whilesym = 25,
    dosym = 26,
    callsym = 27,
    constsym = 28,
    varsym = 29,
    procsym = 30,
    writesym = 31,
    readsym = 32,
    elsesym = 33
} token_type;

typedef struct {
    char *word;
    token_type token;
} Keyword;

typedef struct {
    token_type token;
    int val;
    char lexeme[12];
} Token;

typedef struct {
    int kind;       // 1 = const, 2 = var, 3 = procedure
    char name[10];
    int val;        // for const
    int level;      // lexical level
    int addr;       // relative address or code address for proc
    int mark;       // 0 = active, 1 = marked (removed)
} symbol;

symbol symbol_table[MAX_SYMBOL_TABLE_SIZE];
int symbolCount = 0;

Token *tokenList = NULL;
int tokenCount = 0;
int tokenCapacity = 100;

Token currentToken;
int currentTokenIndex = 0;

int cidx = 0; // code index
Instruction code[500];

Keyword keywordTable[NUM_KEYWORDS] = {
    {"const", constsym},
    {"var", varsym},
    {"begin", beginsym},
    {"end", endsym},
    {"if", ifsym},
    {"else", elsesym},
    {"then", thensym},
    {"procedure", procsym},
    {"while", whilesym},    // Changed from when to while
    {"do", dosym},
    {"call", callsym},
    {"write", writesym},
    {"read", readsym},
    {"fi", fisym}
};

void printError(char *message) {
    printf("Error: %s\n", message);
    halt = 1;
    exit(1);
}

int symbolTableCheck(char *name, int level){
    // Search symbol table from end backward, return the most recent active symbol with matching name and level <= given
    for(int i = symbolCount - 1; i >= 0; i--){
        if(symbol_table[i].mark == 0 && strcmp(symbol_table[i].name, name) == 0 && symbol_table[i].level <= level){
            return i;
        }
    }
    return -1;
}

void emit(int op, int l, int m){
    if(cidx >= 500){
        printError("code overflow");
        return;
    }
    code[cidx].op = op;
    code[cidx].l = l;
    code[cidx].m = m;
    cidx++;
}

void getToken(){
    if(currentTokenIndex < tokenCount){
        currentToken = tokenList[currentTokenIndex];
        currentTokenIndex++;
    } else {
        currentToken.token = periodsym;
        currentToken.lexeme[0] = '\0';
        currentToken.val = 0;
    }
}

// Add symbol with correct level, mark=0 active
void addSymbolTable(int kind, char *name, int val, int level, int addr){
    if(symbolCount >= MAX_SYMBOL_TABLE_SIZE){
        printError("symbol table overflow");
        return;
    }
    strcpy(symbol_table[symbolCount].name, name);
    symbol_table[symbolCount].kind = kind;
    symbol_table[symbolCount].val = val;
    symbol_table[symbolCount].level = level;
    symbol_table[symbolCount].addr = addr;
    symbol_table[symbolCount].mark = 0;
    symbolCount++;
}

// Mark symbols with level > current level as removed when leaving block
void markSymbols(int level){
    for(int i = 0; i < symbolCount; i++){
        if(symbol_table[i].mark == 0 && symbol_table[i].level > level){
            symbol_table[i].mark = 1;
        }
    }
}

// Check if identifier already exists at current level (same level check)
int symbolTableCheckSameLevel(char *name, int level){
    for(int i = symbolCount - 1; i >= 0; i--){
        if(symbol_table[i].mark == 0 && strcmp(symbol_table[i].name, name) == 0 && symbol_table[i].level == level){
            return i;
        }
    }
    return -1;
}

// constDeclaration: accepts level, pointer to tx and dx to update symbol table and data index
void constDeclaration(int level, int *tx, int *dx){
    // currentToken is constsym when called
    getToken();
    while (1){
        if(currentToken.token != identsym){
            printError("const, var, procedure must be followed by identifier");
        }
        if(symbolTableCheckSameLevel(currentToken.lexeme, level) != -1){
            printError("symbol name has already been declared");
        }
        char name[10];
        strncpy(name, currentToken.lexeme, 10);
        name[9] = '\0';

        getToken();
        if(currentToken.token != eqsym){
            printError("= must be followed by a number");
        }
        getToken();
        if(currentToken.token != numbersym){
            printError("= must be followed by a number");
        }
        int val = currentToken.val;
        addSymbolTable(1, name, val, level, 0);  // addr unused for const
        (*tx)++;
        getToken();
        if(currentToken.token != commasym){
            break;
        }
        getToken();
    }
    if(currentToken.token != semicolonsym){
        printError("semicolon or comma missing");
    }
    getToken();
}

// varDeclaration: accepts level, pointers to tx and dx to update symbol table and data index
int varDeclaration(int level, int *tx, int *dx){
    int numVars = 0;
    if(currentToken.token == varsym){
        getToken();
        while(1){
            if(currentToken.token != identsym){
                printError("const, var, procedure must be followed by identifier");
            }
            if(symbolTableCheckSameLevel(currentToken.lexeme, level) != -1){
                printError("symbol name has already been declared");
            }
            char name[10];
            strncpy(name, currentToken.lexeme, 10);
            name[9] = '\0';

            // Store variables at correct addresses starting from current dx
            addSymbolTable(2, name, 0, level, *dx);
            (*dx)++;  // increment for next variable
            (*tx)++;
            numVars++;
            getToken();

            if(currentToken.token != commasym){
                break;
            }
            getToken();
        }
        if(currentToken.token != semicolonsym){
            printError("semicolon or comma missing");
        }
        getToken();
    }
    return numVars;
}

// forward declarations to pass level and tx
void statement(int level, int *tx);
void block(int level, int *tx);
void expression(int level, int *tx);

void factor(int level, int *tx){
    if(currentToken.token == identsym){
        int symidx = symbolTableCheck(currentToken.lexeme, level);
        if(symidx == -1){
            printError("undeclared identifier");
        }
        if(symbol_table[symidx].kind == 1){ // const
            emit(LIT, 0, symbol_table[symidx].val);
        }
        else if(symbol_table[symidx].kind == 2){ // var
            emit(LOD, level - symbol_table[symidx].level, symbol_table[symidx].addr);
        }
        else{
            printError("expression must not contain a procedure identifier");
        }
        getToken();
    }
    else if(currentToken.token == numbersym){
        emit(LIT, 0, currentToken.val);
        getToken();
    }
    else if(currentToken.token == lparentsym){
        getToken();
        expression(level, tx);
        if(currentToken.token != rparentsym){
            printError("right parenthesis missing");
        }
        getToken();
    }
    else{
        printError("the preceding factor cannot begin with this symbol");
    }
}

void term(int level, int *tx){
    factor(level, tx);
    while(currentToken.token == multsym || currentToken.token == slashsym || currentToken.token == modsym){
        if(currentToken.token == multsym){
            getToken();
            factor(level, tx);
            emit(OPR, 0, 3);
        }
        else if(currentToken.token == slashsym){
            getToken();
            factor(level, tx);
            emit(OPR, 0, 4);
        }
        else if(currentToken.token == modsym){  // Added mod support
            getToken();
            factor(level, tx);
            emit(OPR, 0, 11);  // MOD operation
        }
    }
}

void expression(int level, int *tx){
    int addop = 0;
    if(currentToken.token == plussym || currentToken.token == minussym){
        addop = currentToken.token;
        getToken();
        term(level, tx);
        if(addop == minussym){
            emit(OPR, 0, 2);
        }
    }
    else{
        term(level, tx);
    }
    while(currentToken.token == plussym || currentToken.token == minussym){
        addop = currentToken.token;
        getToken();
        term(level, tx);
        if(addop == plussym){
            emit(OPR, 0, 1);
        } else{
            emit(OPR, 0, 2);
        }
    }
}

void condition(int level, int *tx){
    expression(level, tx);
    if(currentToken.token == eqsym){
        getToken();
        expression(level, tx);
        emit(OPR, 0, 5);
    }
    else if(currentToken.token == neqsym){
        getToken();
        expression(level, tx);
        emit(OPR, 0, 6);
    }
    else if(currentToken.token == lessym){
        getToken();
        expression(level, tx);
        emit(OPR, 0, 7);
    }
    else if(currentToken.token == leqsym){
        getToken();
        expression(level, tx);
        emit(OPR, 0, 8);
    }
    else if(currentToken.token == gtrsym){
        getToken();
        expression(level, tx);
        emit(OPR, 0, 9);
    }
    else if(currentToken.token == geqsym){
        getToken();
        expression(level, tx);
        emit(OPR, 0, 10);
    }
    else{
        printError("condition must contain comparison operator");
    }
}

void statement(int level, int *tx){
    if(currentToken.token == identsym){
        int symidx = symbolTableCheck(currentToken.lexeme, level);
        if(symidx == -1){
            printError("undeclared identifier");
        }
        if(symbol_table[symidx].kind != 2){
            printError("assignment to constant or procedure is not allowed");
        }
        getToken();
        if(currentToken.token != becomessym){
            printError("assignment operator expected");
        }
        getToken();
        expression(level, tx);
        emit(STO, level - symbol_table[symidx].level, symbol_table[symidx].addr);
        return;
    }

    if(currentToken.token == beginsym){
        getToken();
        statement(level, tx);
        while(currentToken.token == semicolonsym){
            getToken();
            statement(level, tx);
        }
        if(currentToken.token != endsym){
            printError("semicolon or end expected");
        }
        getToken();
        return;
    }

    if(currentToken.token == ifsym){
        getToken();
        condition(level, tx);
        int jpcidx = cidx;
        emit(JPC, 0, 0);
        if(currentToken.token != thensym){
            printError("then expected");
        }
        getToken();
        statement(level, tx);

        // MUST have else clause per grammar
        if(currentToken.token != elsesym){
            printError("else expected");
        }
        int jmpidx = cidx;
        emit(JMP, 0, 0);  // jump over else part
        code[jpcidx].m = cidx;  // backpatch JPC to else part

        getToken();
        statement(level, tx);  // else statement
        code[jmpidx].m = cidx;  // backpatch JMP to end

        if(currentToken.token != fisym){
            printError("fi expected");
        }
        getToken();
        return;
    }

    if(currentToken.token == whilesym){  // Changed from whensym
        getToken();
        int loopidx = cidx;
        condition(level, tx);
        if(currentToken.token != dosym){
            printError("do expected");
        }
        getToken();
        int jpcidx = cidx;
        emit(JPC, 0, 0);
        statement(level, tx);
        emit(JMP, 0, loopidx);
        code[jpcidx].m = cidx;
        return;
    }

    if(currentToken.token == callsym){
        getToken();
        if(currentToken.token != identsym){
            printError("call must be followed by an identifier");
        }
        int symidx = symbolTableCheck(currentToken.lexeme, level);
        if(symidx == -1){
            printError("undeclared identifier");
        }
        if(symbol_table[symidx].kind != 3){
            printError("call of a constant or variable is meaningless");
        }
        emit(CAL, level - symbol_table[symidx].level, symbol_table[symidx].addr);
        getToken();
        return;
    }

    if(currentToken.token == readsym){
        getToken();
        if(currentToken.token != identsym){
            printError("const, var, procedure must be followed by identifier");
        }
        int symidx = symbolTableCheck(currentToken.lexeme, level);
        if(symidx == -1){
            printError("undeclared identifier");
        }
        if(symbol_table[symidx].kind != 2){
            printError("assignment to constant or procedure is not allowed");
        }
        getToken();
        emit(SYS, 0, 2);
        emit(STO, level - symbol_table[symidx].level, symbol_table[symidx].addr);
        return;
    }

    if(currentToken.token == writesym){
        getToken();
        expression(level, tx);
        emit(SYS, 0, 1);
        return;
    }
}

void procedureDeclaration(int level, int *tx){
    while(currentToken.token == procsym){
        getToken();
        if(currentToken.token != identsym){
            printError("const, var, procedure must be followed by identifier");
        }
        char procName[10];
        strncpy(procName, currentToken.lexeme, 10);
        procName[9] = '\0';

        if(symbolTableCheckSameLevel(procName, level) != -1){
            printError("symbol name has already been declared");
        }

        addSymbolTable(3, procName, 0, level,0);
        (*tx)++;
        getToken();

        if(currentToken.token != semicolonsym){
            printError("semicolon or comma missing");
        }
        getToken();

        block(level + 1, tx); // recursive block for procedure body

        if(currentToken.token != semicolonsym){
            printError("semicolon between statements missing");
        }
        getToken();

    }
    emit(OPR, 0, 0);
}

void block(int level, int *tx){
    int dx = 3;
    int tx0 = *tx;
    int blockStart = *tx;

    int jmpidx = cidx;
    emit(JMP, 0, 0); // jump over declarations, fix later


    if(currentToken.token == constsym){
        constDeclaration(level, tx, &dx);
    }

    // var declarations
    int numVars = varDeclaration(level, tx, &dx);

    // procedure declarations
    procedureDeclaration(level, tx);

    code[jmpidx].m = cidx;
    markSymbols(level);

    emit(INC, 0, dx);

    statement(level, tx);
    emit(OPR, 0, 0);

}

void program(){
    cidx = 0;
    symbolCount = 0;
    currentTokenIndex = 0;


    getToken();
    block(0, &symbolCount);
    if(currentToken.token != periodsym){
        printError("period expected");
    }
    emit(SYS, 0, 3); // halt

    // Mark all symbols as inactive at the end of program
    for(int i = 0; i < symbolCount; i++){
        symbol_table[i].mark = 1;
    }
}


// Add token to dynamic array, expanding capacity if needed
void addToken(token_type token, char *lexeme, int val){
    if(tokenCount >= tokenCapacity){
        tokenCapacity *= 2;
        tokenList = realloc(tokenList, tokenCapacity * sizeof(Token));
    }

    tokenList[tokenCount].token = token;
    if(token == numbersym){
        tokenList[tokenCount].val = val;
    }
    if(lexeme != NULL){
        strncpy(tokenList[tokenCount].lexeme, lexeme, 11);
        tokenList[tokenCount].lexeme[11] = '\0';
    }
    else{
        tokenList[tokenCount].lexeme[0] = '\0';
    }
    tokenCount++;
}

// Check if identifier is a keyword or return identsym
token_type lookupKeyword(const char *str){
    for(int i = 0; i < NUM_KEYWORDS; i++){
        if(strcmp(str, keywordTable[i].word) == 0){
            return keywordTable[i].token;
        }
    }
    return identsym;
}

int lexAnalyze(char *filename){
    FILE *fp = fopen(filename, "r");
    if(fp == NULL){
        printf("Error: Could not open file %s\n", filename);
        return 1;
    }

    char buffer[100];
    int c;

    // Main lexical analysis loop
    while((c = fgetc(fp)) != EOF){
        if(isspace(c)){
            continue;
        }
        // Handle identifiers and keywords
        else if(isalpha(c)){
            int i = 0;
            buffer[i++] = c;
            while(isalnum(c = fgetc(fp)) && i < 11){
                buffer[i++] = c;
            }
            buffer[i] = '\0';
            if(i > 11){
                printError("identifier too long");
            }
            else{
                token_type keyword = lookupKeyword(buffer);
                addToken(keyword, buffer, 0);
            }
            ungetc(c, fp);
        }
        // Handle numbers
        else if(isdigit(c)){
            int i = 0;
            buffer[i++] = c;
            while(isdigit(c = fgetc(fp)) && i < 5){
                buffer[i++] = c;
            }
            buffer[i] = '\0';
            if(i > 5){
                printError("this number is too large");
            }
            else{
                addToken(numbersym, buffer, atoi(buffer));
            }
            ungetc(c, fp);
        }
        // Handle mod operator
        else if(c == '%'){
            addToken(modsym, "%", 0);
        }
        // Handle symbols and operators
        else{
            switch(c){
                case '+':
                    addToken(plussym, "+", 0);
                    break;
                case '-':
                    addToken(minussym, "-", 0);
                    break;
                case ':': {
                    int next = fgetc(fp);
                    if(next == '='){
                        addToken(becomessym, ":=", 0);
                    }
                    else{
                        printError("invalid symbol");
                    }
                }
                break;
                case '=':
                    addToken(eqsym, "=", 0);
                    break;
                case ',':
                    addToken(commasym, ",", 0);
                    break;
                case '*':
                    addToken(multsym, "*", 0);
                    break;
                case ';':
                    addToken(semicolonsym, ";", 0);
                    break;
                case '.':
                    addToken(periodsym, ".", 0);
                    break;
                case '>': {
                    int next = fgetc(fp);
                    if(next == '='){
                        addToken(geqsym, ">=", 0);
                    }
                    else{
                        addToken(gtrsym, ">", 0);
                        ungetc(next, fp);
                    }
                }
                break;
                case '<': {
                    int next = fgetc(fp);
                    if(next == '='){
                        addToken(leqsym, "<=", 0);
                    }
                    else if(next == '>'){
                        addToken(neqsym, "<>", 0);
                    }
                    else{
                        addToken(lessym, "<", 0);
                        ungetc(next, fp);
                    }
                }
                break;
                case '/': {
                    int next = fgetc(fp);
                    if(next == '*'){
                        int prev = 0;
                        while((c = fgetc(fp)) != EOF){
                            if(prev == '*' && c == '/'){
                                break;
                            }
                            prev = c;
                        }
                    }
                    else{
                        addToken(slashsym, "/", 0);
                        ungetc(next, fp);
                    }
                }
                break;
                case '(':
                    addToken(lparentsym, "(", 0);
                    break;
                case ')':
                    addToken(rparentsym, ")", 0);
                    break;
                default:
                    printError("invalid symbol");
                    break;
            }
        }
    }

    fclose(fp);
    return 0;
}

void printAssemblyCode(){
    printf("No errors, program is syntactically correct.\n\n");
    printf("Assembly Code:\n");
    printf("Line OP L M\n");
    for(int i = 0; i < cidx; i++){
        printf("%d ", i);
        switch(code[i].op){
            case LIT: printf("LIT"); break;
            case OPR: printf("OPR"); break;
            case LOD: printf("LOD"); break;
            case STO: printf("STO"); break;
            case CAL: printf("CAL"); break;
            case INC: printf("INC"); break;
            case JMP: printf("JMP"); break;
            case JPC: printf("JPC"); break;
            case SYS: printf("SYS"); break;
        }
        printf(" %d %d\n", code[i].l, code[i].m);
    }
}

void printSymbolTable(){
    printf("\nSymbol Table:\n");
    printf("Kind | Name | Value | Level | Address | Mark\n");
    printf("---------------------------------------------------\n");
    for(int i = 0; i < symbolCount; i++){
        // Only print valid symbols (non-empty names)
        if(strlen(symbol_table[i].name) > 0){
            printf("%d | %s | %d | %d | %d | %d\n",
                   symbol_table[i].kind, symbol_table[i].name, symbol_table[i].val,
                   symbol_table[i].level, symbol_table[i].addr, symbol_table[i].mark);
        }
    }
}

void createElfFile(){
    FILE *fp = fopen("elf.txt", "w");
    if(fp == NULL){
        printf("Error: Could not create elf.txt\n");
        return;
    }

    for(int i = 0; i < cidx; i++){
        fprintf(fp, "%d %d %d\n", code[i].op, code[i].l, code[i].m);
    }

    fclose(fp);
}

void printSourceCode(char *filename){
    FILE *fp = fopen(filename, "r");
    if(fp == NULL){
        return;
    }

    printf("Source Code:\n");
    int c;
    while((c = fgetc(fp)) != EOF){
        printf("%c", c);
    }
    printf("\n\n");

    fclose(fp);
}

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    tokenList = malloc(tokenCapacity * sizeof(Token));

    printSourceCode(argv[1]);

    if(lexAnalyze(argv[1]) != 0){
        return 1;
    }

    if(halt){
        return 1;
    }

    program();

    if(!halt){
        printAssemblyCode();
        printSymbolTable();
        createElfFile();
    }

    free(tokenList);
    return 0;
}
