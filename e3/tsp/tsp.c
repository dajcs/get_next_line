#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_CITIES 11

/* ------------------------ Distance --------------------------*/

float distance(float a[2], float b[2])
{
	return sqrtf((b[0] - a[0]) * (b[0] - a[0]) + (b[1] - a[1]) * (b[1] - a[1]));
}


/* --------------------- TSP Core --------------------------- */
/*
  Brute force DFS over all permutations, pruned whenever the partial length already
  exceeds the best found so far.
  The depth-first search enumerates at most 10! = 3 628 800 permutations
   - start city is fixed at index 0 (symmetry avoids mirror duplicates)
   - fits in an `unsigned` mask because n <= 11                                  */


/* -------- file‑scope (global) variables -------------------------*/

/* Square matrix holding the pre‑computed Euclidean distance
   between every pair of cities.
   Filled once in tsp() and then read‑only.*/
static float     g_dist[MAX_CITIES][MAX_CITIES];

/* Number of cities actually loaded from the input file.
   Used by the DFS and the bit‑masking logic */
static ssize_t   g_n;

/* Length of the best (shortest) tour found so far.
   Acts as the pruning threshold while the depth‑first search explores permutations.*/
static float     g_best;

/* Depth-first search over all tours that
    - start at city 0  (fixed)
    - visit every other city once (Hamiltonian path)
    - return to city 0 (closed curve)
- Arguments:
    - current: index of the city we're standing rith now
    - visited: bitmask - bit `i` is 1 => city `i` already in the path
    - len    : total length travelled so far
- Globals used:
    - g_n    : number of cities
    - g_dist : pre-computed distance matrix
    - g_best : current best (shortest) tour length
*/
void dfs(int current, unsigned visited, float len)
{
	int next;

    /* 1. Pruning this path because it leads nowhere */
	if (len >= g_best)
		return;

    /* 2. ----- Base case: all cities visited --------- */
    // visited == (1u << g_n) - 1 => every bit 0..g_n-1 is 1
	if (visited == (1u << g_n) - 1)
	{
        /* close the loop by returning to the city 0 */
		len += g_dist[current][0];
        /* update the global optimum if this tour is better */
		if (len < g_best)
			g_best = len;
		return;
	}

    /* 3. ----- Recursive step: try every unvisited city ------- */
	next = 1;   // skipping 0 - it is the fixed start/end
	while (next < g_n)
	{
        /* if city 'next' hasn't been used yet */
		if (!(visited & (1u << next)))
        /* recurse with:
               - new current city:       `next`
               - mark it visited:        visited | (1 << next)
               - add edge length         len + g_dist[current][next]
        */
			dfs(next,
                visited | (1U << next),
                len + g_dist[current][next]);
		next++;
	}
}

float tsp(float (*array)[2], ssize_t size)
{
	ssize_t i;
	ssize_t j;
	if (size <= 1)
		return 0.0f;

	g_n = size;
	i = 0;
	while (i < size)
	{
		j = 0;
		while (j < size)
		{
			g_dist[i][j] = distance(array[i], array[j]);
			j++;
		}
	i++;
	}
	g_best = INFINITY;
	dfs(0, 1u, 0.0f);
	return (g_best);
}



ssize_t    file_size(FILE *file)
{
    char    *buffer = NULL;
    size_t    n = 0;
    ssize_t ret;

    errno = 0;
    for (ret = 0; getline(&buffer, &n, file) != -1; ret++);
    free(buffer);
    if (errno || fseek(file, 0, SEEK_SET))
        return -1;
    return ret;
}

int        retrieve_file(float (*array)[2], FILE *file)
{
    int tmp;

    for (size_t i = 0;
         (tmp = fscanf(file, "%f, %f\n", array[i] + 0, array[i] + 1)) != EOF;
         i++)
        if (tmp != 2)
        {
            errno = EINVAL;
            return -1;
        }
    if (ferror(file))
        return -1;
    return 0;
}

int        main(int ac, char **av)
{
    char *filename = "stdin";
    FILE *file = stdin;

    if (ac > 1)
    {
        filename = av[1];
        file = fopen(filename, "r");
    }
    if (!file)
    {
        fprintf(stderr, "Error opening %s: %m\n", filename);
        return 1;
    }

    ssize_t size = file_size(file);
    if (size == -1)
    {
        fprintf(stderr, "Error reading %s: %m\n", filename);
        fclose(file);
        return 1;
    }
    if (size > MAX_CITIES)
    {
        fprintf(stderr, "Error: at most %d cities supported\n", MAX_CITIES);
        fclose(file);
        return 1;
    }

    float (*array)[2] = calloc(size, sizeof (float [2]));
    if (!array)
    {
        fprintf(stderr, "Error: %m\n");
        fclose(file);
        return 1;
    }

    if (retrieve_file(array, file) == -1)
    {
        fprintf(stderr, "Error reading %s: %m\n", filename);
        fclose(file);
        free(array);
        return 1;
    }
    if (ac > 1)
        fclose(file);

    printf("%.2f\n", tsp(array, size));
    free(array);
    return 0;
}
