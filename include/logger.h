#ifndef LOGGER_H
#define LOGGER_H
#define LOG_FIFO_PATH "/tmp/minisync_log_fifo"
#define LOG_FILE_PATH "logs/minisync.log"

void loggerMain(void);

void registrarEvento(const char *formato, ...);

#endif
