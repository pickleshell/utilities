#ifndef OLLAMA_H
#define OLLAMA_H
#include <stddef.h>
typedef struct {
    char name[64];
    char id[32];
    char size[32];
    char processor[32];
    char context[32];
    char until[32];
} ollama_row;
int parse_ollama_ps(const char *text, ollama_row *out, size_t cap, size_t *n);
#endif
