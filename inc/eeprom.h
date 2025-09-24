#ifndef EEPROM_H
#define EEPROM_H
#include <stdint.h>
void eeprom_write_byte(uint16_t addr, uint8_t val);
uint8_t eeprom_read_byte(uint16_t addr);
void eeprom_write_block(uint16_t addr, const uint8_t* src, uint16_t len);
void eeprom_read_block(uint16_t addr, uint8_t* dst, uint16_t len);
#endif
