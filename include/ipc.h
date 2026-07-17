#ifndef IPC_H
#define IPC_H

#include <sys/types.h>

#define MAX_MSG_LEN 4200
#define MAX_PATH_LEN_IPC 4096
#define NUM_WORKERS 2

typedef struct {
    char comando[16];
    char ruta[MAX_PATH_LEN_IPC];
} MensajeTarea;


typedef struct {
    int fd_lectura;   
    int fd_escritura;  
    pid_t pid;
} CanalWorker;


CanalWorker lanzarWorker(int idWorker, const char *dirBase, const char *dirBackup);


void workerMain(int idWorker, int fd_lectura, const char *dirBase, const char *dirBackup);

void enviarTarea(int fd_escritura, const char *ruta);

void enviarFin(int fd_escritura);

#endif
