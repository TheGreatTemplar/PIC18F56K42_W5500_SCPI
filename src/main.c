#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "board.h"
#include "config.h"
#include "w5500.h"
#include "scpi.h"
#include "eeprom.h"

#pragma config FEXTOSC = OFF, RSTOSC = HFINTOSC_64MHZ
#pragma config MCLRE = INTMCLR
#pragma config WDTE = OFF
#pragma config LVP = OFF

netcfg_t g_netcfg;

static uint16_t crc16(const uint8_t* d, uint16_t n){
    uint16_t c=0xFFFF;
    for (uint16_t i=0;i<n;i++){ c ^= d[i]; for (uint8_t b=0;b<8;b++) c = (c&1)? (c>>1)^0xA001 : (c>>1); }
    return c;
}

#define EEADDR_NETCFG  0x0000

static void netcfg_defaults(netcfg_t* nc){
    memset(nc,0,sizeof(*nc));
    nc->magic=NETCFG_MAGIC; nc->version=NETCFG_VERSION; nc->dhcp=0;
    nc->ip[0]=192; nc->ip[1]=168; nc->ip[2]=1; nc->ip[3]=200;
    nc->mask[0]=255; nc->mask[1]=255; nc->mask[2]=255; nc->mask[3]=0;
    nc->gw[0]=192; nc->gw[1]=168; nc->gw[2]=1; nc->gw[3]=1;
    nc->mac[0]=0x00; nc->mac[1]=0x08; nc->mac[2]=0xDC; nc->mac[3]=0x55; nc->mac[4]=0x00; nc->mac[5]=0x01;
    nc->port=TCP_PORT_DEFAULT;
    nc->crc = crc16((const uint8_t*)nc, sizeof(*nc)-2);
}

static void netcfg_load(void){
    eeprom_read_block(EEADDR_NETCFG, (uint8_t*)&g_netcfg, sizeof(g_netcfg));
    uint16_t c = crc16((const uint8_t*)&g_netcfg, sizeof(g_netcfg)-2);
    if (g_netcfg.magic!=NETCFG_MAGIC || g_netcfg.version!=NETCFG_VERSION || g_netcfg.crc!=c){
        netcfg_defaults(&g_netcfg);
    }
}

static void netcfg_save(void){
    g_netcfg.magic=NETCFG_MAGIC; g_netcfg.version=NETCFG_VERSION;
    g_netcfg.crc = crc16((const uint8_t*)&g_netcfg, sizeof(g_netcfg)-2);
    eeprom_write_block(EEADDR_NETCFG, (const uint8_t*)&g_netcfg, sizeof(g_netcfg));
}

static int parse_ip(const char* s, uint8_t out[4]){ unsigned v[4]; if (sscanf(s, "%u.%u.%u.%u", &v[0],&v[1],&v[2],&v[3])!=4) return -1; for(int i=0;i<4;i++){ if(v[i]>255) return -1; out[i]=(uint8_t)v[i]; } return 0; }
static int parse_mac(const char* s, uint8_t out[6]){ unsigned v[6]; if (sscanf(s, "%02x:%02x:%02x:%02x:%02x:%02x", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5])!=6) return -1; for(int i=0;i<6;i++){ if(v[i]>255) return -1; out[i]=(uint8_t)v[i]; } return 0; }

int scpi_lan_command(uint8_t sn, const char* upper_line, int is_query){
    const char* l = upper_line;
    if (!strncmp(l, ":SYST:COMM:LAN:DHCP", 20)){
        if (is_query){ char b[8]; sprintf(b,"%d\n", g_netcfg.dhcp?1:0); wiz_sock_send(sn,(uint8_t*)b, (uint16_t)strlen(b)); return 0; }
        const char* a = strchr(l, ' '); if (!a) { wiz_sock_send(sn,(uint8_t*)"\n",1); return 0; }
        while(*a==' ') ++a; g_netcfg.dhcp = (!strcmp(a,"1")||!strcmp(a,"ON"))?1:0; wiz_sock_send(sn,(uint8_t*)"\n",1); return 0;
    }
    if (!strncmp(l, ":SYST:COMM:LAN:IP", 18)){
        if (is_query){ char b[32]; sprintf(b,"%u.%u.%u.%u\n", g_netcfg.ip[0],g_netcfg.ip[1],g_netcfg.ip[2],g_netcfg.ip[3]); wiz_sock_send(sn,(uint8_t*)b, (uint16_t)strlen(b)); return 0; }
        const char* a = strchr(l, ' '); if (a){ ++a; uint8_t ip[4]; if (!parse_ip(a, ip)){ memcpy(g_netcfg.ip, ip, 4); } } wiz_sock_send(sn,(uint8_t*)"\n",1); return 0;
    }
    if (!strncmp(l, ":SYST:COMM:LAN:MASK", 20)){
        if (is_query){ char b[32]; sprintf(b,"%u.%u.%u.%u\n", g_netcfg.mask[0],g_netcfg.mask[1],g_netcfg.mask[2],g_netcfg.mask[3]); wiz_sock_send(sn,(uint8_t*)b, (uint16_t)strlen(b)); return 0; }
        const char* a = strchr(l, ' '); if (a){ ++a; uint8_t ip[4]; if (!parse_ip(a, ip)){ memcpy(g_netcfg.mask, ip, 4); } } wiz_sock_send(sn,(uint8_t*)"\n",1); return 0;
    }
    if (!strncmp(l, ":SYST:COMM:LAN:GATE", 20)){
        if (is_query){ char b[32]; sprintf(b,"%u.%u.%u.%u\n", g_netcfg.gw[0],g_netcfg.gw[1],g_netcfg.gw[2],g_netcfg.gw[3]); wiz_sock_send(sn,(uint8_t*)b, (uint16_t)strlen(b)); return 0; }
        const char* a = strchr(l, ' '); if (a){ ++a; uint8_t ip[4]; if (!parse_ip(a, ip)){ memcpy(g_netcfg.gw, ip, 4); } } wiz_sock_send(sn,(uint8_t*)"\n",1); return 0;
    }
    if (!strncmp(l, ":SYST:COMM:LAN:MAC", 19)){
        if (is_query){ char b[32]; sprintf(b,"%02X:%02X:%02X:%02X:%02X:%02X\n", g_netcfg.mac[0],g_netcfg.mac[1],g_netcfg.mac[2],g_netcfg.mac[3],g_netcfg.mac[4],g_netcfg.mac[5]); wiz_sock_send(sn,(uint8_t*)b, (uint16_t)strlen(b)); return 0; }
        const char* a = strchr(l, ' '); if (a){ ++a; uint8_t mac[6]; if (!parse_mac(a, mac)){ memcpy(g_netcfg.mac, mac, 6); } } wiz_sock_send(sn,(uint8_t*)"\n",1); return 0;
    }
    if (!strncmp(l, ":SYST:COMM:LAN:PORT", 20)){
        if (is_query){ char b[16]; sprintf(b,"%u\n", g_netcfg.port); wiz_sock_send(sn,(uint8_t*)b, (uint16_t)strlen(b)); return 0; }
        const char* a = strchr(l, ' '); if (a){ ++a; unsigned p = (unsigned)atoi(a); if (p>0 && p<65535) g_netcfg.port=(uint16_t)p; } wiz_sock_send(sn,(uint8_t*)"\n",1); return 0;
    }
    if (!strncmp(l, ":SYST:COMM:LAN:SAVE", 20)){ netcfg_save(); wiz_sock_send(sn,(uint8_t*)"\n",1); return 0; }
    return 1;
}

#define RX_LINE_MAX 256
typedef struct { uint8_t used; char line[RX_LINE_MAX]; uint16_t len; } client_t;
static client_t clients[MAX_CLIENTS];

static void spi_init(void){
    SSP1STAT=0x40; SSP1CON1=0x20;
    ANSELCbits.ANSELC5=0; ANSELCbits.ANSELC6=0; ANSELCbits.ANSELC7=0;
    W5500_CS_TRIS=0; W5500_CS_LAT=1; W5500_RST_TRIS=0; W5500_RST_LAT=1; W5500_INT_TRIS=1;
}

static void net_start(void){
    wiz_init_basic(g_netcfg.ip, g_netcfg.gw, g_netcfg.mask, g_netcfg.mac);
    for(uint8_t i=0;i<MAX_CLIENTS;i++){ wiz_sock_listen(i, g_netcfg.port); clients[i].used=1; clients[i].len=0; }
}

static void handle_socket(uint8_t sn){
    uint8_t st = wiz_sock_state(sn);
    if (st==0x00){ wiz_sock_listen(sn, g_netcfg.port); clients[sn].len=0; return; }
    if (st==0x1C){ uint8_t tmp[32]; while(wiz_sock_recv(sn,tmp,sizeof(tmp))>0){} wiz_sock_listen(sn,g_netcfg.port); clients[sn].len=0; return; }
    if (st!=0x17 && st!=0x14) return;
    uint8_t b[64]; int16_t n = (int16_t)wiz_sock_recv(sn, b, sizeof(b)); if(n<=0) return;
    for(int i=0;i<n;i++){ char c=(char)b[i]; if(c=='\r') continue; if(c=='\n'){ clients[sn].line[clients[sn].len]=0; scpi_on_line(sn, clients[sn].line); clients[sn].len=0; } else { if(clients[sn].len+1<RX_LINE_MAX) clients[sn].line[clients[sn].len++]=c; else clients[sn].len=0; } }
}

void main(void){
    scpi_init(); netcfg_load(); spi_init(); net_start();
    for(;;){ for(uint8_t i=0;i<MAX_CLIENTS;i++) handle_socket(i); }
}
