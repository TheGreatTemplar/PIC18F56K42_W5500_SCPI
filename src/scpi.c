#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "scpi.h"
#include "w5500.h"

#define ERRQ_SIZE 8
static int16_t errq[ERRQ_SIZE]; static uint8_t qh=0, qt=0;
static void qpush(int16_t c){ uint8_t n=(uint8_t)((qh+1)%ERRQ_SIZE); if(n==qt) qt=(uint8_t)((qt+1)%ERRQ_SIZE); errq[qh]=c; qh=n; }
static int qpop(int16_t* o){ if(qh==qt) return 0; *o=errq[qt]; qt=(uint8_t)((qt+1)%ERRQ_SIZE); return 1; }
static char tx[256];
bool scpi_send(uint8_t sn, const char* s){ return wiz_sock_send(sn,(const uint8_t*)s,(uint16_t)strlen(s)); }
bool scpi_sendf(uint8_t sn, const char* fmt, ...){ va_list ap; va_start(ap,fmt); vsnprintf(tx,sizeof(tx),fmt,ap); va_end(ap); return scpi_send(sn,tx); }

static float vset=0, iset=0; static uint8_t sens=0, vrange=1; static uint8_t out=0;
static void strtoupper(char* s){ for(;*s;++s)*s=(char)toupper(*s); }
static void trim(char* s){ char* p=s; while(*p==' '||*p=='\t')++p; if(p!=s) memmove(s,p,strlen(p)+1); size_t n=strlen(s); while(n&& (s[n-1]==' '||s[n-1]=='\t')) s[--n]=0; }

void scpi_init(void){ qh=qt=0; vset=0; iset=0; out=0; sens=0; vrange=1; }

static int common(uint8_t sn, const char* l){
    if(!strcmp(l,"*IDN?")) return scpi_send(sn,"HEWLETT-PACKARD,66312A,0,1.0\n")?0:-1;
    if(!strcmp(l,"*RST"))  { scpi_init(); return scpi_send(sn,"\n")?0:-1; }
    if(!strcmp(l,"*CLS"))  { qh=qt=0; return scpi_send(sn,"\n")?0:-1; }
    if(!strcmp(l,":SYST:ERR?")){ int16_t c; if(qpop(&c)) return scpi_sendf(sn,"%d,\"Error\"\n",c)?0:-1; else return scpi_send(sn,"0,\"No error\"\n")?0:-1; }
    if(!strcmp(l,":STAT:QUE?")) return scpi_send(sn,"0\n")?0:-1;
    return 1;
}

static int starts(const char* s, const char* p){ return strncmp(s,p,strlen(p))==0; }

void scpi_on_line(uint8_t sn, const char* in){
    char b[160]; strncpy(b,in,sizeof(b)-1); b[sizeof(b)-1]=0; trim(b); if(!b[0]) return;
    int q=(b[strlen(b)-1]=='?');
    if(common(sn,b)==0) return;
    strtoupper(b); if(q) b[strlen(b)-1]=0;

    if(starts(b,":OUTP")){
        if(q){ scpi_sendf(sn,"%d\n",out?1:0); return; }
        const char* a=strchr(b,' '); if(!a){ qpush(-109); scpi_send(sn,"\n"); return; }
        while(*a==' ') ++a;
        if(!strcmp(a,"ON")||!strcmp(a,"1")){ out=1; scpi_send(sn,"\n"); return; }
        if(!strcmp(a,"OFF")||!strcmp(a,"0")){ out=0; scpi_send(sn,"\n"); return; }
        qpush(-113); scpi_send(sn,"\n"); return;
    }
    if(starts(b,":SOUR:VOLT")||starts(b,":VOLT")||starts(b,":VOLTAGE")){
        if(strstr(b,":RANG")){ if(q){ const char* s=(vrange==0)?"LOW":(vrange==1)?"HIGH":"AUTO"; scpi_sendf(sn,"%s\n",s); return; }
            const char* a=strchr(b,' '); if(!a){ qpush(-109); scpi_send(sn,"\n"); return; } while(*a==' ') ++a;
            if(!strcmp(a,"LOW")) vrange=0; else if(!strcmp(a,"HIGH")) vrange=1; else if(!strcmp(a,"AUTO")) vrange=2; else qpush(-222); scpi_send(sn,"\n"); return; }
        if(q){ scpi_sendf(sn,"%.6f\n",(double)vset); return; }
        const char* a=strchr(b,' '); if(!a){ qpush(-109); scpi_send(sn,"\n"); return; } vset=strtof(a,0); scpi_send(sn,"\n"); return;
    }
    if(starts(b,":SOUR:CURR")||starts(b,":CURR")||starts(b,":CURRENT")){
        if(q){ scpi_sendf(sn,"%.6f\n",(double)iset); return; }
        const char* a=strchr(b,' '); if(!a){ qpush(-109); scpi_send(sn,"\n"); return; } iset=strtof(a,0); scpi_send(sn,"\n"); return;
    }
    if(starts(b,":MEAS:VOLT")){ scpi_sendf(sn,"%.6f\n",(double)vset); return; }
    if(starts(b,":MEAS:CURR")){ scpi_sendf(sn,"%.6f\n",(double)iset); return; }
    if(starts(b,":SENS:FUNC")){ if(q){ scpi_send(sn,sens?"CURR\n":"VOLT\n"); return; } const char* a=strchr(b,' '); if(!a){ qpush(-109); scpi_send(sn,"\n"); return; } while(*a==' ') ++a; if(!strcmp(a,"VOLT")) sens=0; else if(!strcmp(a,"CURR")) sens=1; else qpush(-113); scpi_send(sn,"\n"); return; }
    if(starts(b,":APPL")){ const char* p=strchr(b,' '); if(!p){ qpush(-109); scpi_send(sn,"\n"); return; } ++p; float v=0,i=0; char s[8]={0}; int n=sscanf(p,"%f , %f , %7s",&v,&i,s); if(n<2){ qpush(-109); scpi_send(sn,"\n"); return; } vset=v; iset=i; if(n==3){ if(!strcmp(s,"ON")||!strcmp(s,"1")) out=1; else if(!strcmp(s,"OFF")||!strcmp(s,"0")) out=0; } scpi_send(sn,"\n"); return; }
    // LAN commands are in main.c (scpi_lan_command)
    extern int scpi_lan_command(uint8_t sn, const char* upper_line, int is_query);
    if(starts(b,":SYST:COMM:LAN")){ int r=scpi_lan_command(sn,b,q); if(r==0) return; }
    qpush(-113); scpi_send(sn,"\n");
}
