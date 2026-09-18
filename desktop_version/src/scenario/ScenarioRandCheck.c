/* See SCENARIO_rand_check_against_crt() in ScenarioRand.h. Kept apart from
 * ScenarioRand.c because it needs <windows.h>. */

#include "ScenarioRand.h"

#include <stdio.h>

#ifdef _WIN32

#include <windows.h>

typedef int (__cdecl *rand_fn)(void);
typedef void (__cdecl *srand_fn)(unsigned int);

/* The harness' srand() (defined in ScenarioRand.c). */
void srand(unsigned int seed);

int SCENARIO_rand_check_against_crt(void)
{
    const char* const dlls[] = {"ucrtbase.dll", "msvcrt.dll"};
    const unsigned int seeds[] = {1, 0, 12345, 0xDEADBEEF};
    const rand_fn ours = (rand_fn) SCENARIO_rand_impl_address();
    int checked = 0;
    int failures = 0;
    size_t d, k;

    for (d = 0; d < sizeof(dlls) / sizeof(dlls[0]); ++d)
    {
        HMODULE crt = LoadLibraryA(dlls[d]);
        rand_fn crt_rand;
        srand_fn crt_srand;
        int mismatches = 0;

        if (crt == NULL)
        {
            printf("%s: not available\n", dlls[d]);
            continue;
        }
        crt_rand = (rand_fn) (void*) GetProcAddress(crt, "rand");
        crt_srand = (srand_fn) (void*) GetProcAddress(crt, "srand");
        if (crt_rand == NULL || crt_srand == NULL)
        {
            printf("%s: rand/srand not found\n", dlls[d]);
            continue;
        }

        for (k = 0; k < sizeof(seeds) / sizeof(seeds[0]); ++k)
        {
            int i;
            srand(seeds[k]);
            crt_srand(seeds[k]);
            for (i = 0; i < 1000000; ++i)
            {
                if (ours() != crt_rand())
                {
                    ++mismatches;
                    break;
                }
            }
        }
        ++checked;
        failures += mismatches;
        printf("%s: %s\n", dlls[d], mismatches == 0 ? "identical for 4 x 1000000 draws" : "MISMATCH");
    }
    fflush(stdout);
    return checked > 0 && failures == 0;
}

#else /* _WIN32 */

int SCENARIO_rand_check_against_crt(void)
{
    printf("-scenario-check-rand is only meaningful on Windows\n");
    return 0;
}

#endif /* _WIN32 */
