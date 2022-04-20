#ifndef __PDBGPROXY_H
#define __PDBGPROXY_H

enum gdb_command {NONE, GET_GPRS, GET_SPR,
                 STOP_REASON, GET_THREAD, SET_THREAD,
                 V_CONTC, V_CONTS,
                 QF_THREADINFO, QS_THREADINFO,
                 GET_MEM, PUT_MEM,
                 INTERRUPT, DETACH, LAST_CMD};

typedef void (*command_cb)(uint64_t *stack, void *priv);

void parser_init(command_cb *callbacks);
int parse_buffer(char *buf, size_t len, void *priv);
void send_nack(void *priv);
void send_ack(void *priv);
#endif
