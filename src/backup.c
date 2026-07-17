#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/backup.h"

#define BUF_SIZE 8192

static void crearDirectoriosPadre(const char *rutaArchivo) {
    char ruta[MAX_PATH_LEN];
    strncpy(ruta, rutaArchivo, sizeof(ruta) - 1);
    ruta[sizeof(ruta) - 1] = '\0';

    for (char *p = ruta + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0'; 
            mkdir(ruta, 0755); 
            *p = '/'; 
        }
    }
}

int copiarArchivo(const char *origen, const char *destino) {
    int fd_origen = open(origen, O_RDONLY);
    if (fd_origen == -1) {
        return -1;
    }

    crearDirectoriosPadre(destino);

    int fd_destino = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_destino == -1) {
        close(fd_origen);
        return -1;
    }

    char buffer[BUF_SIZE];
    ssize_t bytesLeidos;
    while ((bytesLeidos = read(fd_origen, buffer, BUF_SIZE)) > 0) {
        ssize_t totalEscrito = 0;
        while (totalEscrito < bytesLeidos) {
            ssize_t n = write(fd_destino, buffer + totalEscrito, bytesLeidos - totalEscrito);
            if (n == -1) {
                break;
            }
            totalEscrito += n;
        }
    }

    fsync(fd_destino);

    struct stat infoOrigen;
    if (fstat(fd_origen, &infoOrigen) == 0) {
        struct timespec tiempos[2];
        tiempos[0] = infoOrigen.st_atim;
        tiempos[1] = infoOrigen.st_mtim; 
        futimens(fd_destino, tiempos);
    }

    close(fd_origen);
    close(fd_destino);
    return 0;
}

void construirRutaBackup(const char *dirBase, const char *dirBackup,
                          const char *rutaOrigen, char *rutaDestino, size_t n) {

    size_t lenBase = strlen(dirBase);
    const char *relativa = rutaOrigen + lenBase;
    if (*relativa == '/') relativa++;
    snprintf(rutaDestino, n, "%s/%s", dirBackup, relativa);
}

int necesitaSincronizar(const InfoArchivo *origen, const char *rutaBackup) {
    struct stat st;
    if (stat(rutaBackup, &st) != 0) {
        return 1;
    }
    if (st.st_size != origen->tamano || st.st_mtime != origen->mtime) {
        return 1;
    }
    return 0;
}
