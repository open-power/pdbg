#ifndef __PARSERS_H
#define __PARSERS_H

#include <stdint.h>
#include <stdbool.h>

#define ADDRESS (parse_number64, NULL)
#define ADDRESS32 (parse_number32, NULL)
#define DATA (parse_number64, NULL)
#define DATA32 (parse_number32, NULL)
#define DATA16 (parse_number16, NULL)
#define DATA8 (parse_number8, NULL)
#define DEFAULT_DATA(default) (parse_number64, default)
#define DEFAULT_DATA32(default) (parse_number32, default)
#define GPR (parse_gpr, NULL)
#define SPR (parse_spr, NULL)

uint64_t *parse_number64(const char *argv);
uint32_t *parse_number32(const char *argv);
uint16_t *parse_number16(const char *argv);
uint8_t *parse_number8(const char *argv);
uint8_t *parse_number8_pow2(const char *argv);
int *parse_gpr(const char *argv);
int *parse_spr(const char *argv);
bool *parse_flag_noarg(const char *argv);

#endif
