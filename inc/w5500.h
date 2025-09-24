#ifndef W5500_H
#define W5500_H
#include <stdint.h>
#include <stdbool.h>
#define W5500_COMMON 0x00
#define W5500_Sn(n)  ((uint8_t)(0x01 + (n)))
#define Sn_TXBUF(n)  ((uint8_t)(0x10 + (n)))
#define Sn_RXBUF(n)  ((uint8_t)(0x18 + (n)))
#define W5500_MR 0x0000
#define W5500_GAR 0x0001
#define W5500_SUBR 0x0005
#define W5500_SHAR 0x0009
#define W5500_SIPR 0x000F
#define W5500_RTR 0x0019
#define W5500_RCR 0x001B
#define W5500_PHYCFGR 0x002E
#define Sn_MR 0x0000
#define  Sn_MR_TCP 0x01
#define Sn_CR 0x0001
#define  Sn_CR_OPEN 0x01
#define  Sn_CR_LISTEN 0x02
#define  Sn_CR_SEND 0x20
#define  Sn_CR_RECV 0x40
#define Sn_IR 0x0002
#define  Sn_IR_SENDOK 0x10
#define  Sn_IR_TIMEOUT 0x08
#define Sn_SR 0x0003
#define  SOCK_CLOSED 0x00
#define  SOCK_LISTEN 0x14
#define  SOCK_ESTABLISHED 0x17
#define  SOCK_CLOSE_WAIT 0x1C
#define Sn_PORT 0x0004
#define Sn_TX_FSR 0x0020
#define Sn_TX_WR 0x0024
#define Sn_RX_RSR 0x0026
#define Sn_RX_RD 0x0028
void wiz_init_basic(const uint8_t ip[4], const uint8_t gw[4], const uint8_t mask[4], const uint8_t mac[6]);
void wiz_sock_listen(uint8_t sn, uint16_t port);
uint16_t wiz_sock_recv(uint8_t sn, uint8_t* dst, uint16_t maxlen);
bool wiz_sock_send(uint8_t sn, const uint8_t* src, uint16_t len);
uint8_t wiz_sock_state(uint8_t sn);
#endif
