#ifndef CHIP8_ASSEMBLER_H
#define CHIP8_ASSEMBLER_H

#define MAX_SYMBOLS 256
#define MAX_NAME_SIZE 64
#define MAX_LINE_SIZE 256
#define MAX_VAR_SIZE 32
#define MAX_TOKENS_PER_LINE 8

#include <stddef.h>
#include <stdint.h>

typedef enum { DATA, CODE } Header;
typedef enum { LABEL, FUNCTION, VARIABLE } SymbolKind;
typedef enum {
    CLS,
    RET,
    JUMP,
    CALL,
    SE,
    SNE,
    LOAD,
    ADD,
    OR,
    AND,
    XOR,
    SUB,
    SHR,
    SUBN,
    SHL,
    RAND,
    DRAW,
    SKP,
    SKNP,
} Operation;

typedef struct {
    size_t address;
    SymbolKind kind;
    char name[MAX_NAME_SIZE];
} Symbol;

typedef struct {
    uint8_t count;
    Symbol symbols[MAX_SYMBOLS];
} SymbolTable;

constexpr char label_decl_pattern[] = "^LABEL [_a-zA-Z][a-zA-Z0-9_]*$";
constexpr char function_decl_pattern[] = "^FUNCTION [_a-zA-Z][a-zA-Z0-9_]*$";
constexpr char variable_decl_pattern[] = "^[_a-zA-Z][a-zA-Z0-9_]*:([ ][0-9A-F]{2})+$";
constexpr char address_pattern[] = "^(<[_a-zA-Z][a-zA-Z0-9_]*>|[0-9A-F]{1,3})$";
constexpr char jump_pattern[] = "^(<[_a-zA-Z][a-zA-Z0-9_]*>|[0-9A-F]{1,3}|V0 (<[a-zA-Z][a-zA-Z0-9_]*>|[0-9A-F]{1,3}))$";
constexpr char se_pattern[] = "^(V[A-F0-9] V[A-F0-9]|V[A-F0-9] [0-9A-F]{2})$";
constexpr char two_register_pattern[] = "^V[0-9A-F] V[0-9A-F]$";
constexpr char one_register_pattern[] = "^V[0-9A-F]$";
constexpr char draw_pattern[] = "^V[0-9A-F] V[0-9A-F] [0-9A-F]$";
constexpr char rand_pattern[] = "^V[0-9A-F] [A-F0-9]{2}$";
constexpr char add_pattern[] = "^(V[0-9A-F] V[0-9A-F]|V[0-9A-F] [0-9A-F]{2}|I V[0-9A-F])$";
constexpr char load_pattern[] = "^(\\[I] V[0-9A-F]|V[0-9A-F] (DT|V[0-9A-F]|[0-9A-F]{2}|\\[I]|K)|I (<[_a-zA-Z][a-zA-Z0-9_]*>|[0-9A-F]{1,3})|([DS]T|F|B|\[I]) V[0-9A-F]|)$";

void assemble(const char *input_file_name, const char *output_file_name);

#endif //CHIP8_ASSEMBLER_H