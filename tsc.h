#pragma once

#define rdtscll_64(val) do {\
        unsigned int __a,__d; \
        __asm__ __volatile__("rdtsc" : "=a" (__a), "=d" (__d)); \
        (val) = (((unsigned long long)__d)<<32) | (__a); \
    } while(0);

#define rdtscll rdtscll_64

