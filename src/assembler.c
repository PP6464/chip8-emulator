// ReSharper disable CppDFANullDereference
#include <assembler.h>
#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

static int parse_hex(const char *s, uint16_t *out) {
    char *end;
    const unsigned long v = strtoul(s, &end, 16);
    if (end == s || *end != '\0') return 0;
    *out = (uint16_t)v;
    return 1;
}

static void strip_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

static void strip_comment_and_rtrim(char *s) {
    char *p = strchr(s, ';');
    if (p) *p = '\0';
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}

static char *ltrim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static Header parse_header(const char *line) {
    if (strcmp(line, "DATA:") == 0) {
        return DATA;
    }

    if (strcmp(line, "CODE:") == 0) {
        return CODE;
    }

    return -1;
}

static bool check_variable_decl(const char *line) {
    regex_t regex;

    regcomp(&regex, variable_decl_pattern, REG_EXTENDED | REG_NOSUB);

    const int result = regexec(&regex, line, 0, nullptr, 0);

    regfree(&regex);
    return !result;
}

static bool check_label_decl(const char *line) {
    regex_t regex;

    regcomp(&regex, label_decl_pattern, REG_EXTENDED | REG_NOSUB);

    const int result = regexec(&regex, line, 0, nullptr, 0);

    regfree(&regex);
    return !result;
}

static bool check_function_decl(const char *line) {
    regex_t regex;

    regcomp(&regex, function_decl_pattern, REG_EXTENDED | REG_NOSUB);

    const int result = regexec(&regex, line, 0, nullptr, 0);

    regfree(&regex);
    return !result;
}

static Operation check_operation(const char *line) {
    if (strcmp(line, "CLS") == 0) return CLS;
    if (strcmp(line, "RET") == 0) return RET;
    if (strncmp(line, "CALL ", 5) == 0) {
        regex_t regex;

        regcomp(&regex, address_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 5, 0, nullptr, 0);

        regfree(&regex);
        return !result ? CALL : -1;
    }
    if (strncmp(line, "JUMP ", 5) == 0) {
        regex_t regex;

        regcomp(&regex, jump_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 5, 0, nullptr, 0);

        regfree(&regex);
        return !result ? JUMP : -1;
    }
    if (strncmp(line, "SE ", 3) == 0) {
        regex_t regex;

        regcomp(&regex, se_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 3, 0, nullptr, 0);

        regfree(&regex);
        return !result ? SE : -1;
    }
    if (strncmp(line, "SNE ", 4) == 0) {
        regex_t regex;

        // Same argument pattern as SE
        regcomp(&regex, se_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 3, 0, nullptr, 0);

        regfree(&regex);
        return !result ? SNE : -1;
    }
    if (strncmp(line, "SUB ", 4) == 0) {
        regex_t regex;

        regcomp(&regex, two_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 4, 0, nullptr, 0);

        regfree(&regex);
        return !result ? SUB : -1;
    }
    if (strncmp(line, "SUBN ", 5) == 0) {
        regex_t regex;

        regcomp(&regex, two_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 5, 0, nullptr, 0);

        regfree(&regex);
        return !result ? SUBN : -1;
    }
    if (strncmp(line, "OR ", 3) == 0) {
        regex_t regex;

        regcomp(&regex, two_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 3, 0, nullptr, 0);

        regfree(&regex);
        return !result ? OR : -1;
    }
    if (strncmp(line, "AND ", 4) == 0) {
        regex_t regex;

        regcomp(&regex, two_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 4, 0, nullptr, 0);

        regfree(&regex);
        return !result ? AND : -1;
    }
    if (strncmp(line, "XOR ", 4) == 0) {
        regex_t regex;

        regcomp(&regex, two_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 4, 0, nullptr, 0);

        regfree(&regex);
        return !result ? XOR : -1;
    }
    if (strncmp(line, "SHR ", 4) == 0) {
        regex_t regex;

        regcomp(&regex, one_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 4, 0, nullptr, 0);

        regfree(&regex);
        return !result ? SHR : -1;
    }
    if (strncmp(line, "SHL ", 4) == 0) {
        regex_t regex;

        regcomp(&regex, one_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 4, 0, nullptr, 0);

        regfree(&regex);
        return !result ? SHL : -1;
    }
    if (strncmp(line, "SKP ", 4) == 0) {
        regex_t regex;

        regcomp(&regex, one_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 4, 0, nullptr, 0);

        regfree(&regex);
        return !result ? SKP : -1;
    }
    if (strncmp(line, "SKNP ", 5) == 0) {
        regex_t regex;

        regcomp(&regex, one_register_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 5, 0, nullptr, 0);

        regfree(&regex);
        return !result ? SKNP : -1;
    }
    if (strncmp(line, "DRAW ", 5) == 0) {
        regex_t regex;

        regcomp(&regex, draw_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 5, 0, nullptr, 0);

        regfree(&regex);
        return !result ? DRAW : -1;
    }
    if (strncmp(line, "ADD ", 4) == 0) {
        regex_t regex;

        regcomp(&regex, add_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 4, 0, nullptr, 0);

        regfree(&regex);
        return !result ? ADD : -1;
    }
    if (strncmp(line, "RAND ", 5) == 0) {
        regex_t regex;

        regcomp(&regex, rand_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 5, 0, nullptr, 0);

        regfree(&regex);
        return !result ? RAND : -1;
    }
    if (strncmp(line, "LOAD ", 5) == 0) {
        regex_t regex;

        regcomp(&regex, load_pattern, REG_EXTENDED | REG_NOSUB);

        const int result = regexec(&regex, line + 5, 0, nullptr, 0);

        regfree(&regex);
        return !result ? LOAD : -1;
    }

    return -1;
}

// Add the symbol unless there are too many or if there is a duplicate symbol
static void add_symbol(SymbolTable *table, const Symbol *symbol) {
    if (table->count == MAX_SYMBOLS - 1) {
        fprintf(stderr, "Too many symbols");
        exit(1);
    }

    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, symbol->name) == 0) {
            fprintf(stderr, "Duplicate symbol: %s", symbol->name);
            printf("\nOriginally, %s is held by the %d kind symbol at address %lu", symbol->name, table->symbols[i].kind, table->symbols[i].address);
            exit(1);
        }
    }

    table->symbols[table->count++] = *symbol;
}

static void construct_table(FILE *input, SymbolTable *table) {
    char line[MAX_LINE_SIZE];
    uint64_t line_number = 0;
    Header header = -1;
    size_t address = ROM_START + 2;

    // For simplicity do not want many CODE, DATA, CODE, DATA CODE etc. sequences of headers
    bool can_change_header = true;

    while (fgets(line, MAX_LINE_SIZE, input)) {
        line_number++;

        strip_comment_and_rtrim(line);
        strip_newline(line);
        char *line_start = ltrim(line);
        if (*line_start == '\0') continue;  // Empty line

        if (parse_header(line_start) == DATA) {
            if (!can_change_header) {
                fprintf(stderr, "Changed headers too many times at line %lu - %s", line_number, line_start);
                exit(1);
            }

            if (header == CODE) {
                can_change_header = false;
            }

            header = DATA;

            continue;
        }

        if (parse_header(line_start) == CODE) {
            if (!can_change_header) {
                fprintf(stderr, "Changed headers too many times at line %lu - %s", line_number, line_start);
                exit(1);
            }

            if (header == DATA) {
                can_change_header = false;
            }

            header = CODE;
            continue;
        }

        if (header == DATA) {
            // Expect variables here
            if (!check_variable_decl(line_start)) {
                fprintf(stderr, "Invalid variable declaration at line %lu - %s", line_number, line_start);
                exit(1);
            }

            size_t length = 0;

            for (int i = 0; i < strlen(line_start); i++) {
                if (line_start[i] == ':') {
                    length = i;
                }
            }

            line_start[length] = '\0';

            if (length >= MAX_NAME_SIZE) {
                fprintf(stderr, "Too long variable name at line %lu - %s", line_number, line_start);
            }

            Symbol var;
            var.address = address;
            var.kind = VARIABLE;
            strncpy(var.name, line_start, MAX_NAME_SIZE - 1);

            add_symbol(table, &var);

            // Find out how many bytes are specified
            const char *byte_ptr = strtok(line_start + length + 2, " ");
            size_t bytes = 0;

            while (byte_ptr != nullptr) {
                bytes++;
                byte_ptr = strtok(nullptr, " ");
            }

            if (bytes > MAX_VAR_SIZE) {
                fprintf(stderr, "Too many bytes specified at line %lu - %s", line_number, line_start);
                exit(1);
            }

            address += bytes;

            if (address >= MEMORY_SIZE) {
                fprintf(stderr, "Ran out of ROM space for line %lu - %s", line_number, line_start);
                exit(1);
            }

            continue;
        }

        if (header == CODE) {
            if (check_label_decl(line_start)) {
                // Add this label to our symbol table
                Symbol label;
                label.address = address;
                label.kind = LABEL;
                strncpy(label.name, line_start + 6, MAX_NAME_SIZE - 1);

                add_symbol(table, &label);

                continue;
            }

            if (check_function_decl(line_start)) {
                // Add this function to our symbol table
                Symbol func;
                func.address = address;
                func.kind = FUNCTION;
                strncpy(func.name, line_start + 9, MAX_NAME_SIZE - 1);

                add_symbol(table, &func);

                continue;
            }

            if (check_operation(line_start) == -1) {
                fprintf(stderr, "Invalid operation at line %lu - %s", line_number, line_start);
                exit(1);
            }

            // We have a valid operation which takes up 2 bytes
            address += 2;

            if (address >= MEMORY_SIZE) {
                fprintf(stderr, "Ran out of ROM space for line %lu - %s", line_number, line_start);
                exit(1);
            }

            continue;
        }

        fprintf(stderr, "Code not under a header at line %lu - %s", line_number, line_start);
        exit(1);
    }
}

static void emit_bytes(FILE *input, FILE *output, const SymbolTable *table) {
    // We have a syntactically valid input file since otherwise construct_table would have failed
    char line[MAX_LINE_SIZE];
    Header header = -1;

    int main_index = -1;
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, "main") == 0) {
            main_index = i;
            break;
        }
    }

    if (main_index == -1) {
        fprintf(stderr, "No main function found");
        exit(1);
    }

    // Emit instruction to jump to main
    uint16_t jump_to_main_opcode = 0x1000;
    jump_to_main_opcode |= table->symbols[main_index].address;

    const uint8_t top = jump_to_main_opcode >> 8;
    const uint8_t bottom = jump_to_main_opcode & 0xFF;

    fwrite(&top, 1, 1, output);
    fwrite(&bottom, 1, 1, output);

    size_t line_number = 0;

    while (fgets(line, MAX_LINE_SIZE, input)) {
        line_number++;
        strip_comment_and_rtrim(line);
        strip_newline(line);
        char *line_start = ltrim(line);
        if (*line_start == '\0') continue; // Empty line

        if (parse_header(line_start) == DATA) {
            header = DATA;
            continue;
        }

        if (parse_header(line_start) == CODE) {
            header = CODE;
            continue;
        }

        if (header == DATA) {
            // Emit bytes for variable
            size_t colon_position = 0;

            for (int i = 0; i < strlen(line_start); i++) {
                if (line_start[i] == ':') {
                    colon_position = i;
                }
            }

            const char *byte_ptr = strtok(line_start + colon_position + 2, " ");

            while (byte_ptr != nullptr) {
                uint16_t parsed_byte = 0;
                parse_hex(byte_ptr, &parsed_byte);
                uint8_t byte = (uint8_t) parsed_byte & 0xFF;
                fwrite(&byte, 1, 1, output);
                byte_ptr = strtok(nullptr, " ");
            }
        }

        if (header == CODE) {
            const Operation operation = check_operation(line_start);
            uint16_t opcode = 0;

            switch (operation) {
                case CLS: {
                    opcode = 0x00E0;
                    break;
                }
                case RET: {
                    opcode = 0x00EE;
                    break;
                }
                case JUMP: {
                    if (line_start[5] == '<') {
                        // Address of a label
                        int i = 1;

                        while (true) {
                            if (line_start[5 + i] == '>') {
                                line_start[5 + i] = '\0';
                                break;
                            }

                            i++;
                        }

                        const char *name = line_start + 6;

                        uint16_t address = 0;

                        for (int j = 0; j < table->count; j++) {
                            if (strcmp(name, table->symbols[j].name) == 0) {
                                if (table->symbols[j].kind != LABEL) {
                                    fprintf(stderr, "Jump to symbol that is not a label at line %lu - %s", line_number, line_start);
                                    exit(1);
                                }

                                address = table->symbols[j].address;
                                break;
                            }
                        }

                        if (address == 0) {
                            fprintf(stderr, "Unknown label at line %lu - %s", line_number, line_start);
                            exit(1);
                        }

                        opcode = 0x1000;
                        opcode |= address;
                    } else {
                        // Hardcoded address
                        uint16_t address = 0;
                        parse_hex(line_start + 5, &address);

                        opcode = 0x1000;
                        opcode |= address;
                    }

                    break;
                }
                case CALL: {
                    if (line_start[5] == '<') {
                        // Address of a label
                        int i = 1;

                        while (true) {
                            if (line_start[5 + i] == '>') {
                                line_start[5 + i] = '\0';
                                break;
                            }

                            i++;
                        }

                        const char *name = line_start + 6;

                        uint16_t address = 0;

                        for (int j = 0; j < table->count; j++) {
                            if (strcmp(name, table->symbols[j].name) == 0) {
                                if (table->symbols[j].kind != FUNCTION) {
                                    fprintf(stderr, "Jump to symbol that is not a label at line %lu - %s", line_number, line_start);
                                    exit(1);
                                }

                                address = table->symbols[j].address;
                                break;
                            }
                        }

                        if (address == 0) {
                            fprintf(stderr, "Unknown label at line %lu - %s", line_number, line_start);
                            exit(1);
                        }

                        opcode = 0x2000;
                        opcode |= address;
                    } else {
                        // Hardcoded address
                        uint16_t address = 0;
                        parse_hex(line_start + 5, &address);

                        opcode = 0x2000;
                        opcode |= address;
                    }

                    break;
                }
                case SE: {
                    const char x = line_start[4];

                    if ('0' <= x && x <= '9') {
                        opcode = (x - '0') << 8;
                    } else {
                        opcode = (x - 'A' + 10) << 8;
                    }

                    switch (line_start[6]) {
                        case 'V': {
                            const char y = line_start[7];

                            if ('0' <= y && y <= '9') {
                                opcode |= 0x5000 | (y - '0') << 4;
                            } else {
                                opcode |= 0x5000 | (y - 'A' + 10) << 4;
                            }

                            break;
                        }
                        default: {
                            uint16_t byte = 0;
                            parse_hex(line_start + 6, &byte);

                            opcode |= 0x3000 | byte;

                            break;
                        }
                    }

                    break;
                }
                case SNE: {
                    const char x = line_start[5];

                    if ('0' <= x && x <= '9') {
                        opcode = (x - '0') << 8;
                    } else {
                        opcode = (x - 'A' + 10) << 8;
                    }

                    switch (line_start[7]) {
                        case 'V': {
                            const char y = line_start[8];

                            if ('0' <= y && y <= '9') {
                                opcode |= 0x9000 | (y - '0') << 4;
                            } else {
                                opcode |= 0x9000 | (y - 'A' + 10) << 4;
                            }

                            break;
                        }
                        default: {
                            uint16_t byte = 0;
                            parse_hex(line_start + 7, &byte);

                            opcode |= 0x4000 | byte;

                            break;
                        }
                    }

                    break;
                }
                case LOAD: {
                    // We know there are exactly two arguments
                    char * tokens[2];

                    tokens[0] = strtok(line_start + 5, " ");
                    tokens[1] = strtok(nullptr, " ");

                    if (*tokens[0] == 'F') {
                        uint16_t reg;
                        parse_hex(tokens[1] + 1, &reg);
                        opcode = 0xF029 | reg << 8;
                        break;
                    }

                    if (*tokens[0] == 'B') {
                        uint16_t reg;
                        parse_hex(tokens[1] + 1, &reg);
                        opcode = 0xF033 | reg << 8;
                        break;
                    }

                    if (*tokens[0] == 'I') {
                        if (*tokens[1] == '<') {
                            // Address of a label
                            int i = 1;

                            while (true) {
                                if (tokens[1][i] == '>') {
                                    tokens[1][i] = '\0';
                                    break;
                                }

                                i++;
                            }

                            const char *name = tokens[1] + 1;

                            uint16_t address = 0;

                            for (int j = 0; j < table->count; j++) {
                                if (strcmp(name, table->symbols[j].name) == 0) {
                                    address = table->symbols[j].address;
                                    break;
                                }
                            }

                            if (address == 0) {
                                fprintf(stderr, "Unknown symbol at line %lu - %s", line_number, line_start);
                                exit(1);
                            }

                            opcode = 0xA000;
                            opcode |= address;
                        } else {
                            // Hardcoded address
                            uint16_t address = 0;
                            parse_hex(tokens[1], &address);

                            opcode = 0xA000;
                            opcode |= address;
                        }

                        break;
                    }

                    if (strncmp(tokens[0], "[I]", 3) == 0) {
                        uint16_t reg;
                        parse_hex(tokens[1] + 1, &reg);
                        opcode = 0xF055 | reg << 8;

                        break;
                    }

                    if (strncmp(tokens[0], "DT", 2) == 0) {
                        uint16_t reg;
                        parse_hex(tokens[1] + 1, &reg);
                        opcode = 0xF015 | reg << 8;

                        break;
                    }

                    if (strncmp(tokens[0], "ST", 2) == 0) {
                        uint16_t reg;
                        parse_hex(tokens[1] + 1, &reg);
                        opcode = 0xF018 | reg << 8;

                        break;
                    }

                    if (*tokens[1] == 'K') {
                        tokens[0][2] = '\0';
                        uint16_t nibble;
                        parse_hex(tokens[0] + 1, &nibble);
                        opcode = 0xF00A | nibble << 8;

                        break;
                    }

                    // From here on in we know that the first argument is a register
                    tokens[0][2] = '\0';
                    uint16_t reg_x;
                    parse_hex(tokens[0] + 1, &reg_x);

                    if (strncmp(tokens[1], "DT", 2) == 0) {
                        opcode = 0xF015 | reg_x << 8;

                        break;
                    }

                    if (strncmp(tokens[1], "[I]", 3) == 0) {
                        opcode = 0xF065 | reg_x << 8;

                        break;
                    }

                    if (*tokens[1] == 'V') {
                        uint16_t reg_y;
                        parse_hex(tokens[1] + 1, &reg_y);
                        opcode = 0x8000 | reg_x << 8 | reg_y << 4;

                        break;
                    }

                    uint16_t byte;
                    parse_hex(tokens[1], &byte);
                    opcode = 0x6000 | reg_x << 8 | byte;

                    break;
                }
                case ADD: {
                    if (line_start[4] == 'I') {
                        uint16_t reg;
                        parse_hex(line_start + 7, &reg);
                        opcode = 0xF01E | reg << 8;

                        break;
                    }

                    // Otherwise the first argument is a register
                    uint16_t reg_x;
                    line_start[6] = '\0';
                    parse_hex(line_start + 5, &reg_x);

                    if (line_start[7] == 'V') {
                        uint16_t reg_y;
                        parse_hex(line_start + 8, &reg_y);
                        opcode = 0x8004 | reg_x << 8 | reg_y << 4;

                        break;
                    }

                    uint16_t byte;
                    parse_hex(line_start + 7, &byte);
                    opcode = 0x7000 | reg_x << 8 | byte;

                    break;
                }
                case OR: {
                    uint16_t reg_x, reg_y;
                    line_start[5] = '\0';
                    parse_hex(line_start + 4, &reg_x);
                    parse_hex(line_start + 7, &reg_y);

                    opcode = 0x8001 | reg_x << 8 | reg_y << 4;

                    break;
                }
                case AND: {
                    uint16_t reg_x, reg_y;
                    line_start[6] = '\0';
                    parse_hex(line_start + 5, &reg_x);
                    parse_hex(line_start + 8, &reg_y);

                    opcode = 0x8002 | reg_x << 8 | reg_y << 4;

                    break;
                }
                case XOR: {
                    uint16_t reg_x, reg_y;
                    line_start[6] = '\0';
                    parse_hex(line_start + 5, &reg_x);
                    parse_hex(line_start + 8, &reg_y);

                    opcode = 0x8003 | reg_x << 8 | reg_y << 4;

                    break;
                }
                case SUB: {
                    uint16_t reg_x, reg_y;
                    line_start[6] = '\0';
                    parse_hex(line_start + 5, &reg_x);
                    parse_hex(line_start + 8, &reg_y);

                    opcode = 0x8005 | reg_x << 8 | reg_y << 4;

                    break;
                }
                case SUBN: {
                    uint16_t reg_x, reg_y;
                    line_start[7] = '\0';
                    parse_hex(line_start + 6, &reg_x);
                    parse_hex(line_start + 9, &reg_y);

                    opcode = 0x8007 | reg_x << 8 | reg_y << 4;

                    break;
                }
                case SHR: {
                    uint16_t reg;
                    parse_hex(line_start + 5, &reg);

                    opcode = 0x8006 | reg << 8;

                    break;
                }
                case SHL: {
                    uint16_t reg;
                    parse_hex(line_start + 5, &reg);

                    opcode = 0x800E | reg << 8;

                    break;
                }
                case RAND: {
                    line_start[7] = '\0';

                    uint16_t reg;
                    parse_hex(line_start + 6, &reg);
                    uint16_t byte;
                    parse_hex(line_start + 8, &byte);

                    opcode = 0xC000 | reg << 8 | byte;

                    break;
                }
                case DRAW: {
                    line_start[7] = '\0';
                    line_start[10] = '\0';

                    uint16_t reg_x, reg_y, h;
                    parse_hex(line_start + 6, &reg_x);
                    parse_hex(line_start + 9, &reg_y);
                    parse_hex(line_start + 11, &h);

                    opcode = 0xD000 | reg_x << 8 | reg_y << 4 | h;

                    break;
                }
                case SKP: {
                    uint16_t reg;
                    parse_hex(line_start + 5, &reg);

                    opcode = 0xE09E | reg << 8;

                    break;
                }
                case SKNP: {
                    uint16_t reg;
                    parse_hex(line_start + 5, &reg);

                    opcode = 0xE0A1 | reg << 8;

                    break;
                }
                // This line was a symbol declaration
                default: continue;
            }

            const uint8_t opcode_top = opcode >> 8;
            const uint8_t opcode_bottom = opcode & 0xFF;

            fwrite(&opcode_top, 1, 1, output);
            fwrite(&opcode_bottom, 1, 1, output);
        }
    }
}

void assemble(const char *input_file_name, const char *output_file_name) {
    char input_file_path[32 + strlen(input_file_name)];
    char output_file_path[32 + strlen(output_file_name)];

    snprintf(input_file_path, 32 + strlen(input_file_name), "chip8-source/%s.c8c", input_file_name);
    snprintf(output_file_path, 32 + strlen(output_file_name), "rom/%s.ch8", output_file_name);

    FILE *input = fopen(input_file_path, "r");
    FILE *output = fopen(output_file_path, "wb");

    if (!input) {
        fprintf(stderr, "Could not open input file %s", input_file_path);
        exit(1);
    }

    if (!output) {
        fprintf(stderr, "Could not open output file %s", output_file_path);
        exit(1);
    }

    SymbolTable *table = malloc(sizeof(SymbolTable));

    if (table == nullptr) {
        fprintf(stderr, "Could not allocate memory for symbol table");
        exit(1);
    }

    construct_table(input, table);

    rewind(input);

    emit_bytes(input, output, table);

    free(table);

    const int result1 = fclose(input);
    const int result2 = fclose(output);

    if (result1 != 0) {
        fprintf(stderr, "Could not close file %s", input_file_path);
        exit(1);
    }

    if (result2 != 0) {
        fprintf(stderr, "Could not close file %s", output_file_path);
        exit(1);
    }
}
