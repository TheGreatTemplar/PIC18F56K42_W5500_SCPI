#ifndef SCPI_H
#define SCPI_H
#include <stdint.h>
#include <stdbool.h>
void scpi_init(void);
void scpi_on_line(uint8_t sn, const char* line);
bool scpi_send(uint8_t sn, const char* s);
bool scpi_sendf(uint8_t sn, const char* fmt, ...);
#endif
