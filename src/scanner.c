#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "../include/scanner.h"

int scanDirectorio(const char *directorio, TablaArchivos *tabla) {
    DIR *dir = opendir(directorio);
    if (dir == NULL) {
        return -1;
    }

    struct dirent *entrada;
    while ((entrada = readdir(dir)) != NULL) {

        if (strcmp(entrada->d_name, ".") == 0 ||
            strcmp(entrada->d_name, "..") == 0) {
            continue;
        }

        char rutaCompleta[MAX_PATH_LEN];
        snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s",
                 directorio, entrada->d_name);

        struct stat info;
        if (lstat(rutaCompleta, &info) == -1) {
            continue; 

        if (S_ISDIR(info.st_mode)) {
 
            scanDirectorio(rutaCompleta, tabla);
        } else if (S_ISREG(info.st_mode)) {

            struct stat infoArchivo;
            if (stat(rutaCompleta, &infoArchivo) == -1) {
                continue;
            }
            if (tabla->total < MAX_ARCHIVOS) {
                InfoArchivo *nuevo = &tabla->items[tabla->total];
                strncpy(nuevo->nombre, entrada->d_name, sizeof(nuevo->nombre) - 1);
                nuevo->nombre[sizeof(nuevo->nombre) - 1] = '\0';
                strncpy(nuevo->ruta, rutaCompleta, sizeof(nuevo->ruta) - 1);
                nuevo->ruta[sizeof(nuevo->ruta) - 1] = '\0';
                nuevo->inodo = infoArchivo.st_ino;
                nuevo->tamano = infoArchivo.st_size;
                nuevo->permisos = infoArchivo.st_mode;
                nuevo->mtime = infoArchivo.st_mtime;
                nuevo->es_symlink = 0;
                tabla->total++;
            }
        }
    }

    closedir(dir);
    return 0;
}

void imprimirTabla(const TablaArchivos *tabla) {
    char linea[256];
    int n;

    n = snprintf(linea, sizeof(linea), "%-30s %-10s %-10s %-8s %-20s\n",
                 "NOMBRE", "INODO", "TAMANO", "PERMISOS", "MODIFICADO");
    write(1, linea, n);

    for (int i = 0; i < tabla->total; i++) {
        const InfoArchivo *a = &tabla->items[i];

        char permStr[11];
        snprintf(permStr, sizeof(permStr), "%c%c%c%c%c%c%c%c%c",
            (a->permisos & S_IRUSR) ? 'r' : '-',
            (a->permisos & S_IWUSR) ? 'w' : '-',
            (a->permisos & S_IXUSR) ? 'x' : '-',
            (a->permisos & S_IRGRP) ? 'r' : '-',
            (a->permisos & S_IWGRP) ? 'w' : '-',
            (a->permisos & S_IXGRP) ? 'x' : '-',
            (a->permisos & S_IROTH) ? 'r' : '-',
            (a->permisos & S_IWOTH) ? 'w' : '-',
            (a->permisos & S_IXOTH) ? 'x' : '-');

        char fechaStr[20];
        struct tm *tiempo = localtime(&a->mtime);
        strftime(fechaStr, sizeof(fechaStr), "%Y-%m-%d %H:%M", tiempo);

        n = snprintf(linea, sizeof(linea), "%-30s %-10lu %-10ld %-8s %-20s\n",
               a->nombre, (unsigned long)a->inodo, (long)a->tamano,
               permStr, fechaStr);
        write(1, linea, n);
    }
    n = snprintf(linea, sizeof(linea), "Total: %d archivo(s)\n", tabla->total);
    write(1, linea, n);
}
