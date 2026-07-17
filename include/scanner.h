#ifndef SCANNER_H
#define SCANNER_H

#include <sys/stat.h>
#include <time.h>

#define MAX_PATH_LEN 4096
#define MAX_ARCHIVOS 4096

typedef struct {
    char ruta[MAX_PATH_LEN]; 
    char nombre[256];        
    ino_t inodo;
    off_t tamano;
    mode_t permisos;
    time_t mtime;          
    int es_symlink;
} InfoArchivo;

typedef struct {
    InfoArchivo items[MAX_ARCHIVOS];
    int total;
} TablaArchivos;

int scanDirectorio(const char *directorio, TablaArchivos *tabla);

void imprimirTabla(const TablaArchivos *tabla);

#endif
