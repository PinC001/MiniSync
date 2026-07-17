#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include "../include/stats.h"

struct stats *statsAbrir(int crear) {
    int flags = crear ? (O_CREAT | O_RDWR) : O_RDWR;

    int fd = shm_open(SHM_STATS_NAME, flags, 0666);
    if (fd == -1) {
        return NULL;
    }

    if (crear) {

        if (ftruncate(fd, sizeof(struct stats)) == -1) {
            close(fd);
            return NULL;
        }
    }

    struct stats *st = mmap(NULL, sizeof(struct stats),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); 

    if (st == MAP_FAILED) {
        return NULL;
    }

    if (crear) {
        st->archivos_copiados = 0;
        st->bytes_copiados = 0;
        st->errores = 0;
    }

    return st;
}

sem_t *statsAbrirSemaforo(int crear) {
    sem_t *sem;
    if (crear) {
        sem = sem_open(SEM_STATS_NAME, O_CREAT, 0666, 1);
    } else {
        sem = sem_open(SEM_STATS_NAME, 0);
    }
    return sem; 
}

void statsActualizar(struct stats *st, sem_t *sem,
                      long archivos, long bytes, long errores) {
    sem_wait(sem);

    st->archivos_copiados += archivos;
    st->bytes_copiados += bytes;
    st->errores += errores;

    sem_post(sem); 
}

void statsCerrar(struct stats *st, sem_t *sem, int destruir) {
    if (st != NULL) {
        munmap(st, sizeof(struct stats));
    }
    if (sem != NULL) {
        sem_close(sem);
    }
    if (destruir) {
        shm_unlink(SHM_STATS_NAME);
        sem_unlink(SEM_STATS_NAME);
    }
}
