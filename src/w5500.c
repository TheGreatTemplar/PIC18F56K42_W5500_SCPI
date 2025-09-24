#include <xc.h>
#include <stdint.h>
#include "board.h"
#include "w5500.h"
static inline void cs_low(void){ W5500_CS_LAT=0; }
static inline void cs_high(void){ W5500_CS_LAT=1; }
static uint8_t spi1_xfer(uint8_t b){ SSP1BUF=b; while(!SSP1STATbits.BF); return SSP1BUF; }
static void hdr(uint16_t addr,uint8_t bsb,uint8_t rw,uint8_t vdm){ cs_low(); spi1_xfer((addr>>8)&0xFF); spi1_xfer(addr&0xFF); spi1_xfer((bsb<<3)|rw|vdm); }
static void w_write(uint16_t a,uint8_t b,const uint8_t* d,uint16_t n){ hdr(a,b,0x04,0x00); for(uint16_t i=0;i<n;i++) spi1_xfer(d[i]); cs_high(); }
static void w_read (uint16_t a,uint8_t b,      uint8_t* d,uint16_t n){ hdr(a,b,0x00,0x00); for(uint16_t i=0;i<n;i++) d[i]=spi1_xfer(0x00); cs_high(); }
static void w_write8(uint16_t a,uint8_t b,uint8_t v){ w_write(a,b,&v,1); }
static uint8_t w_read8(uint16_t a,uint8_t b){ uint8_t v; w_read(a,b,&v,1); return v; }
static uint16_t w_get16(uint16_t a,uint8_t b){ uint8_t t[2]; w_read(a,b,t,2); return ((uint16_t)t[0]<<8)|t[1]; }
static void     w_set16(uint16_t a,uint8_t b,uint16_t v){ uint8_t t[2]={(uint8_t)(v>>8),(uint8_t)v}; w_write(a,b,t,2); }
void wiz_init_basic(const uint8_t ip[4],const uint8_t gw[4],const uint8_t mask[4],const uint8_t mac[6]){
    W5500_RST_TRIS=0; W5500_RST_LAT=0; __delay_ms(2); W5500_RST_LAT=1; __delay_ms(10);
    w_write8(W5500_MR,W5500_COMMON,0x80); __delay_ms(2);
    w_write(W5500_GAR, W5500_COMMON, gw, 4);
    w_write(W5500_SUBR,W5500_COMMON, mask,4);
    w_write(W5500_SHAR,W5500_COMMON, mac,6);
    w_write(W5500_SIPR,W5500_COMMON, ip,4);
    uint8_t rtr[2]={0x07,0xD0}; w_write(W5500_RTR,W5500_COMMON,rtr,2);
    w_write8(W5500_RCR,W5500_COMMON,8);
    w_write8(W5500_PHYCFGR,W5500_COMMON,0xB1);
}
void wiz_sock_listen(uint8_t sn,uint16_t port){
    w_write8(0x0001, W5500_Sn(sn), 0x10); __delay_ms(1); // CLOSE
    w_write8(0x0000, W5500_Sn(sn), 0x01); // MR=TCP
    uint8_t p[2]={(uint8_t)(port>>8),(uint8_t)port}; w_write(0x0004,W5500_Sn(sn),p,2);
    w_write8(0x0001, W5500_Sn(sn), 0x01); // OPEN
    w_write8(0x0001, W5500_Sn(sn), 0x02); // LISTEN
}
uint16_t wiz_sock_recv(uint8_t sn,uint8_t* dst,uint16_t maxlen){
    uint16_t rsr=w_get16(Sn_RX_RSR,W5500_Sn(sn)); if(!rsr) return 0; if(rsr>maxlen) rsr=maxlen;
    uint16_t rd =w_get16(Sn_RX_RD ,W5500_Sn(sn)); w_read(rd, Sn_RXBUF(sn), dst, rsr);
    rd+=rsr; w_set16(Sn_RX_RD,W5500_Sn(sn),rd); w_write8(Sn_CR,W5500_Sn(sn),0x40); return rsr;
}
bool wiz_sock_send(uint8_t sn,const uint8_t* src,uint16_t len){
    uint32_t guard=0; while(w_get16(Sn_TX_FSR,W5500_Sn(sn))<len){ if(w_read8(Sn_SR,W5500_Sn(sn))!=0x17) return false; if(++guard>40000UL) return false; }
    uint16_t wr=w_get16(Sn_TX_WR,W5500_Sn(sn)); w_write(wr,Sn_TXBUF(sn),src,len); wr+=len; w_set16(Sn_TX_WR,W5500_Sn(sn),wr); w_write8(Sn_CR,W5500_Sn(sn),0x20);
    guard=0; while(1){ uint8_t ir=w_read8(Sn_IR,W5500_Sn(sn)); if(ir&0x10){ w_write8(Sn_IR,W5500_Sn(sn),0x10); break;} if(ir&0x08){ w_write8(Sn_IR,W5500_Sn(sn),0x08); return false;} if(++guard>60000UL) return false; }
    return true;
}
uint8_t wiz_sock_state(uint8_t sn){ return w_read8(Sn_SR,W5500_Sn(sn)); }
