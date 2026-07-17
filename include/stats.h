#ifndef STATS_H
#define STATS_H

#include <semaphore.h>

#define SHM_STATS_NAME "/minisync_stats"
#define SEM_STATS_NAME "/minisync_stats_sem"

struct stats {
    long archivos_copiados;
    long bytes_copiados;
    long errores;
};

struct stats *statsAbrir(int crear);

sem_t *statsAbrirSemaforo(int crear);

void statsActualizar(struct stats *st, sem_t *sem,
                      long archivos, long bytes, long errores);

void statsCerrar(struct stats *st, sem_t *sem, int destruir);

#endif
