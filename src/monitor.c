#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/scanner.h"
#include "../include/backup.h"
#include "../include/stats.h"
#include "../include/ipc.h"
#include "../include/logger.h"

static volatile sig_atomic_t seguirCorriendo = 1;

static void manejarSenal(int sig) {
    (void)sig;
    seguirCorriendo = 0;
}

static void convertirEnDaemon(void) {
    pid_t pid = fork();
    if (pid > 0) {
        exit(0);
    }
    setsid(); 
    chdir("/"); 
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        char buf[128];
        int n = snprintf(buf, sizeof(buf), "Uso: %s <directorio> <backup> [-d]\n", argv[0]);
        write(2, buf, n);
        return 1;
    }
    const char *dirBase = argv[1];
    const char *dirBackup = argv[2];
    int comoDaemon = (argc > 3 && strcmp(argv[3], "-d") == 0);

    if (comoDaemon) {
        convertirEnDaemon();
    }

    signal(SIGINT, manejarSenal);
    signal(SIGTERM, manejarSenal);

    if (mkdir(dirBackup, 0755) == -1 && errno != EEXIST) {
        char buf[MAX_PATH_LEN + 32];
        int n = snprintf(buf, sizeof(buf), "No se pudo crear %s\n", dirBackup);
        write(2, buf, n);
        return 1;
    }

    pid_t pidLogger = fork();
    if (pidLogger == 0) {
        loggerMain();
        exit(0);
    }

    struct stats *st = statsAbrir(1);
    sem_t *sem = statsAbrirSemaforo(1);
    if (!st || sem == SEM_FAILED) {
        const char *msg = "No se pudo inicializar stats compartidas\n";
        write(2, msg, strlen(msg));
        return 1;
    }

    CanalWorker canales[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        canales[i] = lanzarWorker(i + 1, dirBase, dirBackup);
    }

    registrarEvento("monitor iniciado: vigilando %s -> %s", dirBase, dirBackup);

    static TablaArchivos tabla;
    memset(&tabla, 0, sizeof(tabla));
    scanDirectorio(dirBase, &tabla);

    int siguienteWorker = 0;
    for (int i = 0; i < tabla.total; i++) {
        char rutaBackup[MAX_PATH_LEN];
        construirRutaBackup(dirBase, dirBackup, tabla.items[i].ruta,
                             rutaBackup, sizeof(rutaBackup));
        if (necesitaSincronizar(&tabla.items[i], rutaBackup)) {
            enviarTarea(canales[siguienteWorker].fd_escritura, tabla.items[i].ruta);
            siguienteWorker = (siguienteWorker + 1) % NUM_WORKERS;
        }
    }

    while (seguirCorriendo) {
        sleep(5);
        if (!seguirCorriendo) break;

        static TablaArchivos tablaNueva;
        memset(&tablaNueva, 0, sizeof(tablaNueva));
        scanDirectorio(dirBase, &tablaNueva);

        for (int i = 0; i < tablaNueva.total; i++) {
            char rutaBackup[MAX_PATH_LEN];
            construirRutaBackup(dirBase, dirBackup, tablaNueva.items[i].ruta,
                                 rutaBackup, sizeof(rutaBackup));
            if (necesitaSincronizar(&tablaNueva.items[i], rutaBackup)) {
                enviarTarea(canales[siguienteWorker].fd_escritura, tablaNueva.items[i].ruta);
                siguienteWorker = (siguienteWorker + 1) % NUM_WORKERS;
            }
        }
        tabla = tablaNueva;

        sem_wait(sem);
        char buf[128];
        int n = snprintf(buf, sizeof(buf), "[stats] archivos=%ld bytes=%ld errores=%ld\n",
               st->archivos_copiados, st->bytes_copiados, st->errores);
        write(1, buf, n);
        sem_post(sem);
    }

    for (int i = 0; i < NUM_WORKERS; i++) {
        enviarFin(canales[i].fd_escritura);
    }
    for (int i = 0; i < NUM_WORKERS; i++) {
        close(canales[i].fd_escritura);
        waitpid(canales[i].pid, NULL, 0);
    }

    registrarEvento("SALIR");
    waitpid(pidLogger, NULL, 0);

    statsCerrar(st, sem, 1);
    return 0;
}
