/* This is free and unencumbered software released into the public domain.
   Refer to LICENSE.txt in the enclosing directory. */

/* Randomized stress test of a sqrt(x) caching mechanism. */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    unsigned char number;
    unsigned char sqroot;
} cache_entry_t;

static cache_entry_t g_cache[100];
static int g_cache_size = sizeof(g_cache) / sizeof(g_cache[0]);

static void
cache_init(void)
{
    int i;
    for (i = 0; i < g_cache_size; ++i)
    {
        g_cache[i].number = 0;
        g_cache[i].sqroot = 0;
    }
}

static int
cache_calculate(int number)
{
    int i;
    for (i = 0; i < g_cache_size; ++i)
    {
        if (g_cache[i].number == number)
        {
            /* Cache hit. */
            return g_cache[i].sqroot;
        }
    }

    /* Cache miss. Find correct result and populate a few cache entries. */
    int sqroot = 0;
    int number_adj;
    for (number_adj = number - 1; number_adj < number + 1; ++number_adj)
    {
        int sqroot_adj = (int)(sqrt(number_adj));
        i = (int)(1.0 * g_cache_size * rand() / (RAND_MAX + 1.0));
        g_cache[i].number = number_adj;
        g_cache[i].sqroot = sqroot_adj;

        if (number_adj == number)
        {
            /* This is our return value. */
            sqroot = sqroot_adj;
        }
    }

    return sqroot;
}

int
main(void)
{
    cache_init();

    /* Repeatedly check cache_calculate(). */
    int i;
    for (i = 0;; ++i)
    {
        if (i % 100 == 0)
        {
            printf("i=%i\n", i);
        }
        /* Check cache_calculate() with a random number. */
        int number = (int)(256.0 * rand() / (RAND_MAX + 1.0));
        int sqroot_cache = cache_calculate(number);
        int sqroot_correct = (int)sqrt(number);

        if (sqroot_cache != sqroot_correct)
        {
            /* cached_calculate() returned incorrect value. */
            printf("i=%i: number=%i sqroot_cache=%i sqroot_correct=%i\n",
                   i, number, sqroot_cache, sqroot_correct);
            abort();
        }
    }

    return EXIT_SUCCESS;
}
