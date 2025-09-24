#include <xc.h>
#include <stdint.h>
#include "eeprom.h"
void eeprom_write_byte(uint16_t addr, uint8_t val){
    NVMADRH=(addr>>8)&0xFF; NVMADRL=addr&0xFF; NVMDATL=val;
    NVMCON1bits.NVMREGS=1; NVMCON1bits.FREE=0; NVMCON1bits.WREN=1;
    INTCONbits.GIE=0; NVMCON2=0x55; NVMCON2=0xAA; NVMCON1bits.WR=1;
    while(NVMCON1bits.WR); NVMCON1bits.WREN=0;
}
uint8_t eeprom_read_byte(uint16_t addr){
    NVMADRH=(addr>>8)&0xFF; NVMADRL=addr&0xFF; NVMCON1bits.NVMREGS=1; NVMCON1bits.RD=1; return NVMDATL;
}
void eeprom_write_block(uint16_t addr, const uint8_t* src, uint16_t len){ for(uint16_t i=0;i<len;i++) eeprom_write_byte(addr+i, src[i]); }
void eeprom_read_block(uint16_t addr, uint8_t* dst, uint16_t len){ for(uint16_t i=0;i<len;i++) dst[i]=eeprom_read_byte(addr+i); }
