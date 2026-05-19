#include <assembler.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

static void strip_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

static int parse_hex(const char *s, uint16_t *out) {
    char *end;
    const unsigned long val = strtoul(s, &end, 16);
    if (end == s || *end != '\0') return 0;
    *out = (uint16_t)val;
    return 1;
}

static int parse_reg(const char *s, uint8_t *reg) {
    if ((s[0] != 'V' && s[0] != 'v') || s[1] == '\0' || s[2] != '\0')
        return 0;
    char c = (char)toupper((unsigned char)s[1]);
    if (c >= '0' && c <= '9') { *reg = (uint8_t)(c - '0'); return 1; }
    if (c >= 'A' && c <= 'F') { *reg = (uint8_t)(c - 'A' + 10); return 1; }
    return 0;
}

static void write_opcode(FILE *f, const uint16_t opcode) {
    const uint8_t bytes[2] = { (uint8_t)(opcode >> 8), (uint8_t)(opcode & 0xFF) };
    fwrite(bytes, 1, 2, f);
}

/*
 * Attempt to parse one line into a Chip-8 opcode.
 *
 * The instructions look like this
 *   CLS, RET,
 *   JP  <addr>
 *   JP  V0, <addr>
 *   CALL <addr>
 *   SE   Vx, <byte>
 *   SNE  Vx, <byte>
 *   SE   Vx, Vy
 *   LD   Vx, <byte>
 *   ADD  Vx, <byte>
 *   LD   Vx, Vy
 *   OR   Vx, Vy
 *   AND  Vx, Vy
 *   XOR  Vx, Vy
 *   ADD  Vx, Vy
 *   SUB  Vx, Vy
 *   SHR  Vx
 *   SUBN Vx, Vy
 *   SHL  Vx
 *   SNE  Vx, Vy
 *   LD   I, <addr>
 *   RND  Vx, <byte>
 *   DRW  Vx, Vy, <nibble>
 *   SKP  Vx
 *   SKNP Vx
 *   LD   Vx, DT
 *   LD   Vx, K
 *   LD   DT, Vx
 *   LD   ST, Vx
 *   ADD  I, Vx
 *   LD   F, Vx
 *   LD   B, Vx
 *   LD   [I], Vx
 *   LD   Vx, [I]
 */
static int parse_line(const char *line, uint16_t *opcode) {
    // Tokenise: copy into a mutable buffer and split on spaces/commas. */
    char buf[64];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // Replace commas with spaces for uniform splitting. */
    for (char *p = buf; *p; p++) if (*p == ',') *p = ' ';

    char *tok[8];
    int n = 0;
    char *p = strtok(buf, " \t");
    while (p && n < 8) { tok[n++] = p; p = strtok(nullptr, " \t"); }
    if (n == 0) return 0; // No tokens in the instruction

    const char *operation = tok[0];

    if (strcmp(operation, "CLS") == 0 && n == 1) {
        *opcode = 0x00E0; return 1;
    }

    if (strcmp(operation, "RET") == 0 && n == 1) {
        *opcode = 0x00EE; return 1;
    }

    // JP  <addr> (1nnn)
    // JP  V0, <addr> (Bnnn)
    if (strcmp(operation, "JP") == 0) {
        if (n == 2) {
            // JP addr */
            uint16_t addr;
            if (!parse_hex(tok[1], &addr) || addr > 0xFFF) return 0;
            *opcode = (uint16_t)(0x1000 | (addr & 0x0FFF)); return 1;
        }
        
        if (n == 3) {
            // JP V0, addr */
            uint8_t reg;
            uint16_t addr;
            if (!parse_reg(tok[1], &reg) || reg != 0) return 0;
            if (!parse_hex(tok[2], &addr) || addr > 0xFFF) return 0;
            *opcode = (uint16_t)(0xB000 | addr & 0x0FFF); return 1;
        }
        
        return 0;
    }

    // CALL <addr> (2nnn)
    if (strcmp(operation, "CALL") == 0 && n == 2) {
        uint16_t addr;
        if (!parse_hex(tok[1], &addr) || addr > 0xFFF) return 0;
        *opcode = (uint16_t)(0x2000 | addr & 0x0FFF); return 1;
    }

    // SKP  Vx (Ex9E)
    // SKNP Vx (ExA1)
    if (strcmp(operation, "SKP") == 0 && n == 2) {
        uint8_t x;
        if (!parse_reg(tok[1], &x)) return 0;
        *opcode = (uint16_t)(0xE09E | (uint16_t)x << 8); return 1;
    }
    
    if (strcmp(operation, "SKNP") == 0 && n == 2) {
        uint8_t x;
        if (!parse_reg(tok[1], &x)) return 0;
        *opcode = (uint16_t)(0xE0A1 | (uint16_t)x << 8); return 1;
    }

    // SHR  Vx (8xy6)
    // SHL  Vx (8xyE)
    if (strcmp(operation, "SHR") == 0 && n == 2) {
        uint8_t x;
        
        if (!parse_reg(tok[1], &x)) return 0;
        
        *opcode = (uint16_t)(0x8006 | (uint16_t)x << 8); return 1;
    }
    
    if (strcmp(operation, "SHL") == 0 && n == 2) {
        uint8_t x;
        
        if (!parse_reg(tok[1], &x)) return 0;
        
        *opcode = (uint16_t)(0x800E | (uint16_t)x << 8); return 1;
    }

    // RND  Vx, byte (Cxkk)
    if (strcmp(operation, "RND") == 0 && n == 3) {
        uint8_t x;
        uint16_t kk;
        
        if (!parse_reg(tok[1], &x)) return 0;
        if (!parse_hex(tok[2], &kk) || kk > 0xFF) return 0;
        
        *opcode = (uint16_t)(0xC000 | (uint16_t)x << 8 | kk & 0xFF); return 1;
    }

    // DRW  Vx, Vy, nibble (Dxyn)
    if (strcmp(operation, "DRW") == 0 && n == 4) {
        uint8_t x, y;
        uint16_t nibble;
        
        if (!parse_reg(tok[1], &x)) return 0;
        if (!parse_reg(tok[2], &y)) return 0;
        if (!parse_hex(tok[3], &nibble) || nibble > 0xF) return 0;
        
        *opcode = (uint16_t)(0xD000 | (uint16_t)x << 8 | (uint16_t)y << 4 | nibble & 0xF);
        return 1;
    }

    // SE  Vx, byte  (3xkk)  or  SE  Vx, Vy (5xy0)
    // SNE Vx, byte  (4xkk)  or  SNE Vx, Vy (9xy0)
    if (strcmp(operation, "SE") == 0 && n == 3) {
        uint8_t x, y;
        uint16_t kk;
        
        if (!parse_reg(tok[1], &x)) return 0;
        
        if (parse_reg(tok[2], &y)) {
            // SE Vx, Vy
            *opcode = (uint16_t)(0x5000 | ((uint16_t)x << 8) | ((uint16_t)y << 4)); return 1;
        }
        
        if (parse_hex(tok[2], &kk) && kk <= 0xFF) {
            // SE Vx, byte
            *opcode = (uint16_t)(0x3000 | ((uint16_t)x << 8) | (kk & 0xFF)); return 1;
        }
        
        return 0;
    }
    
    if (strcmp(operation, "SNE") == 0 && n == 3) {
        uint8_t x, y;
        uint16_t kk;
        
        if (!parse_reg(tok[1], &x)) return 0;
        
        if (parse_reg(tok[2], &y)) {
            // SNE Vx, Vy
            *opcode = (uint16_t)(0x9000 | ((uint16_t)x << 8) | ((uint16_t)y << 4)); return 1;
        }
        
        if (parse_hex(tok[2], &kk) && kk <= 0xFF) {
            // SNE Vx, byte
            *opcode = (uint16_t)(0x4000 | ((uint16_t)x << 8) | (kk & 0xFF)); return 1;
        }
        
        return 0;
    }

    // ADD Vx, byte (7xkk)
    // ADD Vx, Vy (8xy4)
    // ADD I,  Vx (Fx1E)
    if (strcmp(operation, "ADD") == 0 && n == 3) {
        uint8_t x, y;
        uint16_t kk;
        
        if (strcmp(tok[1], "I") == 0) {
            // ADD I, Vx */
            if (!parse_reg(tok[2], &y)) return 0;
            *opcode = (uint16_t)(0xF01E | (uint16_t)y << 8); return 1;
        }
        
        if (!parse_reg(tok[1], &x)) return 0;
        
        if (parse_reg(tok[2], &y)) {
            // ADD Vx, Vy */
            *opcode = (uint16_t)(0x8004 | (uint16_t)x << 8 | (uint16_t)y << 4); return 1;
        }
        
        if (parse_hex(tok[2], &kk) && kk <= 0xFF) {
            // ADD Vx, byte */
            *opcode = (uint16_t)(0x7000 | (uint16_t)x << 8 | kk & 0xFF); return 1;
        }
        
        return 0;
    }

    // OR   Vx, Vy  (8xy1)
    // AND  Vx, Vy  (8xy2)
    // XOR  Vx, Vy  (8xy3)
    // SUB  Vx, Vy  (8xy5)
    // SUBN Vx, Vy  (8xy7)

    // ReSharper disable once CppTooWideScope
    const struct { const char *name; uint8_t lo; } alu_ops[] = {
        { "OR",   0x1 }, { "AND",  0x2 }, { "XOR",  0x3 },
        { "SUB",  0x5 }, { "SUBN", 0x7 },
    };
    
    for (int i = 0; i < 5; i++) {
        if (strcmp(operation, alu_ops[i].name) == 0 && n == 3) {
            uint8_t x, y;
            if (!parse_reg(tok[1], &x)) return 0;
            if (!parse_reg(tok[2], &y)) return 0;
            *opcode = (uint16_t)(0x8000 | ((uint16_t)x << 8) | ((uint16_t)y << 4) | alu_ops[i].lo);
            return 1;
        }
    }

    // LD  Vx, byte  (6xkk)
    // LD  Vx, Vy   (8xy0)
    // LD  I, addr  (Annn)
    // LD  Vx, DT   (Fx07)
    // LD  Vx, K    (Fx0A)
    // LD  DT, Vx   (Fx15)
    // LD  ST, Vx   (Fx18)
    // LD  F,  Vx   (Fx29)
    // LD  B,  Vx   (Fx33)
    // LD  [I], Vx  (Fx55)
    // LD  Vx, [I]  (Fx65)
    if (strcmp(operation, "LD") == 0 && n == 3) {
        uint8_t x, y;
        uint16_t addr, kk;

        // LD I, addr
        if (strcmp(tok[1], "I") == 0) {
            if (!parse_hex(tok[2], &addr) || addr > 0xFFF) return 0;
            *opcode = (uint16_t)(0xA000 | addr & 0x0FFF); return 1;
        }
        
        // LD DT, Vx */
        if (strcmp(tok[1], "DT") == 0) {
            if (!parse_reg(tok[2], &y)) return 0;
            *opcode = (uint16_t)(0xF015 | (uint16_t)y << 8); return 1;
        }
        
        // LD ST, Vx */
        if (strcmp(tok[1], "ST") == 0) {
            if (!parse_reg(tok[2], &y)) return 0;
            *opcode = (uint16_t)(0xF018 | (uint16_t)y << 8); return 1;
        }
        
        // LD F, Vx */
        if (strcmp(tok[1], "F") == 0) {
            if (!parse_reg(tok[2], &y)) return 0;
            *opcode = (uint16_t)(0xF029 | (uint16_t)y << 8); return 1;
        }
        
        // LD B, Vx */
        if (strcmp(tok[1], "B") == 0) {
            if (!parse_reg(tok[2], &y)) return 0;
            *opcode = (uint16_t)(0xF033 | (uint16_t)y << 8); return 1;
        }
        
        // LD [I], Vx */
        if (strcmp(tok[1], "[I]") == 0) {
            if (!parse_reg(tok[2], &y)) return 0;
            *opcode = (uint16_t)(0xF055 | (uint16_t)y << 8); return 1;
        }

        // Destination must be a register from here on */
        if (!parse_reg(tok[1], &x)) return 0;

        // LD Vx, DT */
        if (strcmp(tok[2], "DT") == 0) {
            *opcode = (uint16_t)(0xF007 | (uint16_t)x << 8); return 1;
        }
        
        // LD Vx, K */
        if (strcmp(tok[2], "K") == 0) {
            *opcode = (uint16_t)(0xF00A | (uint16_t)x << 8); return 1;
        }
        
        // LD Vx, [I] */
        if (strcmp(tok[2], "[I]") == 0) {
            *opcode = (uint16_t)(0xF065 | (uint16_t)x << 8); return 1;
        }
        
        // LD Vx, Vy */
        if (parse_reg(tok[2], &y)) {
            *opcode = (uint16_t)(0x8000 | (uint16_t)x << 8 | (uint16_t)y << 4); return 1;
        }
        
        // LD Vx, byte */
        if (parse_hex(tok[2], &kk) && kk <= 0xFF) {
            *opcode = (uint16_t)(0x6000 | (uint16_t)x << 8 | kk & 0xFF); return 1;
        }
        
        return 0;
    }

    return 0; // Unrecognised instruction
}

void assemble(const char *input_file_name, const char *output_file_name) {
    const size_t in_len  = strlen(input_file_name);
    const size_t out_len = strlen(output_file_name);

    char input_path[16 + in_len];
    char output_path[16 + out_len];

    snprintf(input_path,  15 + in_len  + 1, "chip8-source/%s", input_file_name);
    snprintf(output_path, 15 + out_len + 1, "assembled/%s",    output_file_name);

    struct stat st = {0};
    if (stat("assembled", &st) == -1) {
        mkdir("assembled", 0700);
    }

    FILE *input  = fopen(input_path,  "r");
    FILE *output = fopen(output_path, "wb");

    if (!input) {
        fprintf(stderr, "Could not open chip8 source file from path %s\n", input_path);
        exit(1);
    }
    if (!output) {
        fprintf(stderr, "Could not create chip8 binary file at path %s\n", output_path);
        fclose(input);
        exit(1);
    }

    char line[64];
    int line_num = 0;

    while (fgets(line, sizeof(line), input)) {
        line_num++;
        strip_newline(line);

        // Skip blank lines and comments (lines starting with ';'). */
        if (line[0] == '\0' || line[0] == ';') continue;

        uint16_t opcode;
        if (!parse_line(line, &opcode)) {
            fprintf(stderr, "Line %d: erroneous instruction: %s\n", line_num, line);
            fclose(input); fclose(output);
            exit(1);
        }

        write_opcode(output, opcode);
    }

    fclose(input);
    fclose(output);
}