# Mini Sistema de Sincronizacion de Archivos

Version simplificada de un servicio de sincronizacion tipo Dropbox/rsync
implementado en C sobre Linux, usando llamadas al sistema de bajo nivel,
multiples procesos e IPC.

## Arquitectura

```
                +------------------+
                | Monitor (daemon) |
                +------------------+
                 |      |       |
             pipe|  pipe|   FIFO/mq
                 v      v       v
           +--------+ +--------+  +--------+
           |Worker 1| |Worker 2|  | Logger |
           +--------+ +--------+  +--------+
                 \        /
                  v      v
              directorio backup/

  Memoria compartida (shm + semaforo) <- stats: archivos_copiados,
  bytes_copiados, errores. Escrita por los workers, leida por el monitor.
```

- **Monitor**: escanea el directorio vigilado cada 5 s, detecta cambios
  comparando metadatos (tamano/mtime) y reparte el trabajo entre los
  workers via pipes.
- **Workers** (`fork()` del monitor): reciben mensajes `COPIAR <ruta>`
  por su pipe, copian el archivo con `open()/read()/write()`, y
  reportan el resultado tanto a la memoria compartida de estadisticas
  como al logger.
- **Logger**: proceso independiente que recibe eventos por un FIFO con
  nombre (`mkfifo`) y los escribe con timestamp en `logs/minisync.log`.

## Estructura del proyecto

```
minisync/
├── Makefile
├── README.md
├── include/         # headers con las interfaces (.h)
│   ├── scanner.h
│   ├── backup.h
│   ├── stats.h
│   ├── ipc.h
│   └── logger.h
├── src/             # implementacion (.c)
│   ├── scan.c        -> comando standalone `scan <directorio>`
│   ├── scanner.c      -> recorrido recursivo (readdir/stat/lstat)
│   ├── backup.c        -> copiarArchivo() y logica incremental
│   ├── stats.c          -> memoria compartida + semaforo POSIX
│   ├── ipc.c              -> pipes monitor<->worker y workerMain()
│   ├── logger.c            -> proceso logger via FIFO
│   └── monitor.c            -> programa principal `minisyncd`
├── backup/          # destino de las copias (se llena en runtime)
└── logs/            # minisync.log (se llena en runtime)
```

Cada archivo `.c` en `src/` tiene bloques `TODO(n)` describiendo
exactamente que llamadas al sistema usar y en que orden; las firmas ya
estan fijadas en los `.h` correspondientes.

## Compilacion

```bash
make            # compila bin/scan y bin/minisyncd
make clean      # limpia objetos y binarios
```

## Uso

```bash
# Escaneo puntual de un directorio
./bin/scan /ruta/a/directorio

# Monitor en primer plano
./bin/minisyncd /ruta/a/vigilar backup

# Monitor como daemon en segundo plano
./bin/minisyncd /ruta/a/vigilar backup -d
```

## Diseno de IPC

| Canal | Tecnologia | Uso |
|---|---|---|
| Monitor -> Worker | `pipe()` | tareas `COPIAR <ruta>` / `FIN` |
| Worker/Monitor -> Logger | FIFO con nombre (`mkfifo`) | eventos de texto con timestamp |
| Worker <-> Worker/Monitor | memoria compartida (`shm_open`+`mmap`) + semaforo POSIX (`sem_open`) | contador de archivos/bytes copiados y errores, protegido de condiciones de carrera |

## Sincronizacion incremental

Antes de copiar, se compara el tamano y la fecha de modificacion
(`st_size`, `st_mtime`) del archivo origen contra el archivo ya
existente en `backup/`; solo se copia si no existe o si alguno de los
dos valores cambio.

## Pendiente para el informe (no forma parte del codigo)

- Medir archivos/seg y MB/seg variando el numero de workers.
- Medir utilizacion de CPU durante una sincronizacion grande.
- Documentar cualquier ayuda externa utilizada, segun exige el
  enunciado.
