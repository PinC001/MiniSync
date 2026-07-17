#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/scanner.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        char buf[128];
        int n = snprintf(buf, sizeof(buf), "Uso: %s <directorio>\n", argv[0]);
        write(2, buf, n);
        return 1;
    }

    static TablaArchivos tabla;
    memset(&tabla, 0, sizeof(tabla));

    if (scanDirectorio(argv[1], &tabla) != 0) {
        char buf[MAX_PATH_LEN + 64];
        int n = snprintf(buf, sizeof(buf), "Error al escanear %s\n", argv[1]);
        write(2, buf, n);
        return 1;
    }

    imprimirTabla(&tabla);
    return 0;
}
