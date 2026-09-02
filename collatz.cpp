#include <cstdint>
#include "collatz.h"

static const int TAM_CACHE = 65536;
static uint16_t cache_collatz[TAM_CACHE];

void preencher_cache_collatz() {
    cache_collatz[1] = 0;
    for (uint64_t i = 2; i < TAM_CACHE; ++i) {
        uint64_t atual = i;
        uint16_t passos = 0;
        while (atual >= i) {
            if ((atual & 1) == 0) {
                atual >>= 1;
            } else {
                atual = (3 * atual + 1) >> 1;
                passos++;
            }
            passos++;
        }
        cache_collatz[i] = passos + cache_collatz[atual];
    }
}

uint64_t conta_passos_collatz(uint64_t n) {
    uint64_t passos = 0;
    while (n >= TAM_CACHE) {
        if ((n & 1) == 0) {
            int zeros = __builtin_ctzll(n);
            n >>= zeros;
            passos += zeros;
        } else {
            n = (3 * n + 1) >> 1;
            passos += 2;
        }
    }
    return passos + cache_collatz[n];
}
