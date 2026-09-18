#ifndef SCENARIORAND_H
#define SCENARIORAND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Observability for the game's random number generators.
 *
 * CRT rand(): on Windows, ScenarioRand.c provides rand()/srand() that are
 * bit-identical to the Microsoft CRT (same LCG, same per-thread state, same
 * initial seed of 1) but expose their state and a call counter. Whether the
 * game's calls actually reach this implementation depends on the toolchain
 * (it does with MinGW-w64; MSVC may bind to the CRT import instead), so
 * SCENARIO_rand_is_interposed() checks it at runtime. On other platforms the
 * CRT is left alone and the rng.crt.* probes are not recorded, because the
 * game would then use a different RAND_MAX and algorithm anyway. */
int SCENARIO_rand_is_interposed(void);
uint32_t SCENARIO_rand_state(void);
uint32_t SCENARIO_rand_calls(void);

/* Address of the harness' rand(), for the interposition check. */
void* SCENARIO_rand_impl_address(void);

/* Compares the harness' rand() with the CRT's own rand() (looked up in
 * ucrtbase.dll and msvcrt.dll) over 4 x 1000000 draws and prints the result.
 * Returns 1 if every available CRT matches. Windows only. */
int SCENARIO_rand_check_against_crt(void);

/* Defined in Xoshiro.c (read-only accessor added for the harness). */
void xoshiro_get_state(uint32_t out[4]);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCENARIORAND_H */
