
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "precompiler.h"

int main(int argc, char *argv[]) {
    char *inputFile = NULL;
    char *outputFile = NULL;
    int verbose = 0;

    static struct option long_options[] = {
        {"in", required_argument, 0, 'i'},
        {"out", required_argument, 0, 'o'},
        {"verbose", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:v", long_options, NULL)) != -1) {
        switch (opt) {
            case 'i': inputFile = optarg; break;
            case 'o': outputFile = optarg; break;
            case 'v': verbose = 1; break;
            default:
                fprintf(stderr, "Usage: %s -i <input.c> [-o <output>] [-v]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (!inputFile && optind < argc) {
        inputFile = argv[optind];
    }

    if (!inputFile) {
        fprintf(stderr, "Errore: file di input non specificato.\n");
        return EXIT_FAILURE;
    }

    if (access(inputFile, F_OK) != 0) {
        fprintf(stderr, "Errore: il file di input \"%s\" non esiste.\n", inputFile);
        return EXIT_FAILURE;
    }

    process_file(inputFile, outputFile, verbose);
    return EXIT_SUCCESS;
}
