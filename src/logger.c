#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "../include/logger.h"
#include "../include/ipc.h"

void loggerMain(void) {
    if (mkfifo(LOG_FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        return;
    }

    FILE *archivoLog = fopen(LOG_FILE_PATH, "a");
    char linea[512];
    int seguir = 1;

    while (seguir) {
        int fd = open(LOG_FIFO_PATH, O_RDONLY);
        FILE *entrada = fdopen(fd, "r");

        while (fgets(linea, sizeof(linea), entrada) != NULL) {
            if (strncmp(linea, "SALIR", 5) == 0) {
                seguir = 0;
                break;
            }
            time_t ahora = time(NULL);
            struct tm *tiempo = localtime(&ahora);
            char marca[32];
            strftime(marca, sizeof(marca), "%Y-%m-%d %H:%M:%S", tiempo);
            fprintf(archivoLog, "[%s] %s", marca, linea);
            fflush(archivoLog); 
            char pantalla[600];
            int n = snprintf(pantalla, sizeof(pantalla), "[%s] %s", marca, linea);
            write(1, pantalla, n);
        }
        fclose(entrada); 
    }
    fclose(archivoLog);
}

void registrarEvento(const char *formato, ...) {
    char mensaje[MAX_MSG_LEN];
    va_list args;
    va_start(args, formato);
    vsnprintf(mensaje, sizeof(mensaje), formato, args);
    va_end(args);

    int fd = open(LOG_FIFO_PATH, O_WRONLY);
    if (fd != -1) {
        dprintf(fd, "%s\n", mensaje);
        close(fd);
    }
}
