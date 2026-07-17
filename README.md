# Mini Sistema de Sincronizacion de Archivos

Este sistema es una version simplificada de un servicio de sincronizacion tipo Dropbox/rsync
implementado en C sobre Linux, usando llamadas al sistema de bajo nivel, procesos como workers e IPC.

## Arquitectura
Se tienen estos cuatro procesos que sirven de base para todo el sistemas, estos procesos son:

- **Monitor**: Escanea el directorio vigilado cada 5 s, detecta cambios
  comparando metadatos como tamaño y fecha, después reparte el trabajo entre los
  workers mediante pipes.
- **Workers** : Reciben mensajes como `COPIAR <ruta>`
  por su pipe, copian el archivo con `open()/read()/write()`, que son llamadas al sistema de bajo nivel y
  reportan el resultado tanto a la memoria compartida de estadisticas como al logger.
- **Logger**: Proceso independiente que recibe eventos por un FIFO con
  nombre `mkfifo` y los escribe con timestamp en `logs/minisync.log`.

## Estructura del proyecto
```
minisync/
├── Makefile
├── README.md
├── include/        
│   ├── scanner.h
│   ├── backup.h
│   ├── stats.h
│   ├── ipc.h
│   └── logger.h
├── src/ 
│   ├── scan.c        
│   ├── scanner.c     
│   ├── backup.c        
│   ├── stats.c          
│   ├── ipc.c              
│   ├── logger.c            
│   └── monitor.c          
├── backup/
└── logs/            
```

## Compilacion
Se utiliza estos comandos en la carpeta raiz del proyecto que es minisync
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

Antes de copiar, se compara el tamaño y la fecha de modificación
(`st_size`, `st_mtime`) del archivo origen contra el archivo ya
existente en `backup/`; solo se copia si no existe o si alguno de los
dos valores cambio.

