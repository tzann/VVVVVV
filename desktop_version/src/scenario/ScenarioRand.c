/* See ScenarioRand.h.
 *
 * This file deliberately does not include <stdlib.h>: on Windows it defines
 * rand() and srand() itself, and the CRT headers may declare them with
 * incompatible linkage attributes. */

#include "ScenarioRand.h"

#include <stddef.h>

#ifdef _WIN32

#if defined(_MSC_VER)
#define SCENARIO_TLS __declspec(thread)
#else
#define SCENARIO_TLS __thread
#endif

/* Identical to the Microsoft CRT: per-thread state, initial seed 1,
 * state = state * 214013 + 2531011, result = (state >> 16) & 0x7FFF. */
static SCENARIO_TLS uint32_t holdrand = 1;
static SCENARIO_TLS uint32_t calls = 0;

int rand(void)
{
    holdrand = holdrand * 214013u + 2531011u;
    ++calls;
    return (int) ((holdrand >> 16) & 0x7FFF);
}

void srand(unsigned int seed)
{
    holdrand = seed;
}

void* SCENARIO_rand_impl_address(void)
{
    return (void*) &rand;
}

uint32_t SCENARIO_rand_state(void)
{
    return holdrand;
}

uint32_t SCENARIO_rand_calls(void)
{
    return calls;
}

#else /* _WIN32 */

void* SCENARIO_rand_impl_address(void)
{
    return NULL;
}

uint32_t SCENARIO_rand_state(void)
{
    return 0;
}

uint32_t SCENARIO_rand_calls(void)
{
    return 0;
}

#endif /* _WIN32 */
