#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../include/ipc.h"
#include "../include/backup.h"
#include "../include/stats.h"
#include "../include/logger.h"

CanalWorker lanzarWorker(int idWorker, const char *dirBase, const char *dirBackup) {
    CanalWorker canal = {-1, -1, -1};
    int fds[2];
    pipe(fds);
    pid_t pid = fork();
    if (pid == 0) {
        close(fds[1]);

        for (int fdExtra = 3; fdExtra < 64; fdExtra++) {
            if (fdExtra != fds[0]) {
                close(fdExtra);
            }
        }
        workerMain(idWorker, fds[0], dirBase, dirBackup);
        exit(0);
    } else {
        close(fds[0]);
        canal.fd_escritura = fds[1];
        canal.pid = pid;
    }
    return canal;
}

void workerMain(int idWorker, int fd_lectura, const char *dirBase, const char *dirBackup) {
    struct stats *st = statsAbrir(0);
    struct stat info;
    sem_t *sem = statsAbrirSemaforo(0);
    FILE *entrada = fdopen(fd_lectura, "r");
    char linea[MAX_MSG_LEN];

    while (fgets(linea, sizeof(linea), entrada) != NULL) {
        if (strncmp(linea, "FIN", 3) == 0) {
            break;
        }
        if (strncmp(linea, "COPIAR ", 7) == 0) {
            char ruta[MAX_PATH_LEN];
            char destino[MAX_PATH_LEN];
            strncpy(ruta, linea + 7, sizeof(ruta) - 1);
            ruta[sizeof(ruta) - 1] = '\0';
            size_t len = strlen(ruta);
            if (len > 0 && ruta[len - 1] == '\n') {
                ruta[len - 1] = '\0';
            }

            stat(ruta, &info);
            construirRutaBackup(dirBase, dirBackup, ruta, destino, sizeof(destino));
            int resultado = copiarArchivo(ruta, destino);
            if (resultado == 0) {
                statsActualizar(st, sem, 1, info.st_size, 0);
                registrarEvento("worker %d copio %s", idWorker, ruta);
            } else {
                statsActualizar(st, sem, 0, 0, 1);
                registrarEvento("worker %d fallo copiando %s", idWorker, ruta);
            }
        }
    }

    statsCerrar(st, sem, 0);
    close(fd_lectura);
}

void enviarTarea(int fd_escritura, const char *ruta) {
    dprintf(fd_escritura, "COPIAR %s\n", ruta);
}

void enviarFin(int fd_escritura) {
    dprintf(fd_escritura, "FIN\n");
}
