#ifndef BACKUP_H
#define BACKUP_H

#include "scanner.h"

int copiarArchivo(const char *origen, const char *destino);

int necesitaSincronizar(const InfoArchivo *origen, const char *rutaBackup);

void construirRutaBackup(const char *dirBase, const char *dirBackup,
                          const char *rutaOrigen, char *rutaDestino, size_t n);

#endif
