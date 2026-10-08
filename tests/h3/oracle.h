#ifndef H3_ORACLE_H
#define H3_ORACLE_H
#include <stdint.h>
uint8_t *oracle_build(unsigned histogram[12]);
void oracle_decode(uint32_t rank, uint8_t state[14]);
uint32_t oracle_rank(const uint8_t state[14]);
void oracle_apply(uint8_t state[14], uint8_t move);
#endif
