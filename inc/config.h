#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#define TCP_PORT_DEFAULT 5025
#define MAX_CLIENTS 4
#define NETCFG_MAGIC  0x4B545A31u
#define NETCFG_VERSION 0x0001u
typedef struct __attribute__((packed)) {
    uint32_t magic; uint16_t version;
    uint8_t dhcp; uint8_t ip[4]; uint8_t mask[4]; uint8_t gw[4]; uint8_t mac[6];
    uint16_t port; uint16_t crc;
} netcfg_t;
extern netcfg_t g_netcfg;
#endif
