#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "precompiler.h"

// Struttura per contenere statistiche
typedef struct {
    int variabili_controllate;
    int errori_identificatori;
    int righe_commento_eliminate;
    int file_inclusi;
    int righe_input;
    int byte_input;
    int righe_output;
    int byte_output;
} Stats;

// Controlla se un identificatore è valido in C
int is_valid_identifier(const char *name) {
    if (!name || !isalpha(name[0]) && name[0] != '_') return 0;
    for (int i = 1; name[i]; i++) {
        if (!isalnum(name[i]) && name[i] != '_') return 0;
    }
    return 1;
}

// Rimuove i commenti dal codice e aggiorna le statistiche
char* remove_comments(const char *code, Stats *stats) {
    char *clean = malloc(strlen(code) + 1);
    if (!clean) return NULL;

    int i = 0, j = 0;
    while (code[i]) {
        if (code[i] == '/' && code[i+1] == '/') {
            stats->righe_commento_eliminate++;
            while (code[i] && code[i] != '\n') i++;
        } else if (code[i] == '/' && code[i+1] == '*') {
            stats->righe_commento_eliminate++;
            i += 2;
            while (code[i] && !(code[i] == '*' && code[i+1] == '/')) {
                if (code[i] == '\n') stats->righe_commento_eliminate++;
                i++;
            }
            if (code[i]) i += 2;
        } else {
            clean[j++] = code[i++];
        }
    }
    clean[j] = '\0';
    return clean;
}

// Estrae e controlla i nomi delle variabili locali e globali
void check_variable_names(const char *code, const char *filename, Stats *stats) {
    char line[1024];
    int riga = 0;
    FILE *f = fopen(filename, "r");
    if (!f) return;

    int dentro_main = 0, fine_dichiarazioni = 0;
    while (fgets(line, sizeof(line), f)) {
        riga++;

        if (strstr(line, "main(")) dentro_main = 1;
        else if (dentro_main && (strchr(line, '{') || strchr(line, '}'))) continue;

        if (!dentro_main || !fine_dichiarazioni) {
            char tipo[32], nome[64];
            if (sscanf(line, "%31s %63[^;=]", tipo, nome) == 2) {
                char *token = strtok(nome, ", ");
                while (token) {
                    stats->variabili_controllate++;
                    if (!is_valid_identifier(token)) {
                        stats->errori_identificatori++;
                        fprintf(stderr, "Errore identificatore: %s (file: %s, riga: %d)\n", token, filename, riga);
                    }
                    token = strtok(NULL, ", ");
                }
            } else {
                if (dentro_main) fine_dichiarazioni = 1;
            }
        }
    }

    fclose(f);
}

// Processa direttive #include
char* process_includes(const char *code, Stats *stats) {
    char *processed = malloc(strlen(code) * 10); // spazio abbondante
    if (!processed) return NULL;
    processed[0] = '\0';

    char line[1024];
    const char *p = code;

    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (eol - p + 1) : strlen(p);

        strncpy(line, p, len);
        line[len] = '\0';

        if (strncmp(line, "#include \"", 10) == 0) {
            char filename[256];
            if (sscanf(line, "#include \"%255[^\"]\"", filename) == 1) {
                FILE *f = fopen(filename, "r");
                if (f) {
                    stats->file_inclusi++;
                    char inc_line[1024];
                    while (fgets(inc_line, sizeof(inc_line), f)) {
                        strcat(processed, inc_line);
                    }
                    fclose(f);
                } else {
                    fprintf(stderr, "Errore apertura include: %s\n", filename);
                }
            }
        } else {
            strncat(processed, p, len);
        }

        p += len;
    }

    return processed;
}

// Funzione principale di elaborazione file
void process_file(const char *input_filename, const char *output_filename, int verbose) {
    FILE *f = fopen(input_filename, "r");
    if (!f) {
        fprintf(stderr, "Errore apertura file input: %s\n", input_filename);
        return;
    }

    Stats stats = {0};
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Errore allocazione memoria\n");
        fclose(f);
        return;
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    stats.byte_input = size;

    // Conta righe input
    for (int i = 0; buffer[i]; i++)
        if (buffer[i] == '\n') stats.righe_input++;

    char *no_comments = remove_comments(buffer, &stats);
    char *with_includes = process_includes(no_comments, &stats);
    check_variable_names(buffer, input_filename, &stats);

    FILE *out = output_filename ? fopen(output_filename, "w") : stdout;
    if (!out) {
        fprintf(stderr, "Errore apertura file output\n");
        return;
    }

    fputs(with_includes, out);
    if (output_filename) fclose(out);

    // Calcola righe e byte dell'output
    stats.byte_output = strlen(with_includes);
    for (int i = 0; with_includes[i]; i++)
        if (with_includes[i] == '\n') stats.righe_output++;

    if (verbose) {
        fprintf(stderr, "\nStatistiche:\n");
        fprintf(stderr, "Variabili controllate: %d\n", stats.variabili_controllate);
        fprintf(stderr, "Errori identificatori: %d\n", stats.errori_identificatori);
        fprintf(stderr, "Commenti eliminati: %d righe\n", stats.righe_commento_eliminate);
        fprintf(stderr, "File inclusi: %d\n", stats.file_inclusi);
        fprintf(stderr, "Input - righe: %d, byte: %d\n", stats.righe_input, stats.byte_input);
        fprintf(stderr, "Output - righe: %d, byte: %d\n", stats.righe_output, stats.byte_output);
    }

    free(buffer);
    free(no_comments);
    free(with_includes);
}
