
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void print_solution(int *pos, int n)
{
	int i;

	i = 0;
	while (i < n)
	{
		fprintf(stdout, "%d", pos[i]);
		if (i+1 < n)
			fprintf(stdout, " ");
		i++;
	}
	fprintf(stdout, "\n");
}


// is_safe: checks that a candidate (row, col) isn't attacked
// horizontally or diagonally by earlier queens
int is_safe(int *pos, int col, int row)
{
	int i;

	i = 0;
	while (i < col)
	{
		if (pos[i] == row)
			return 0;
		if (abs(pos[i] - row) == col - i)
			return 0;
		i++;
	}
	return 1;
}

// solve - DFS backtracking
// walks column-by-column (depth-first).
// for each column it tries every row;
// when col == n reached, a complete placement is printed
void solve(int *pos, int n, int col)
{
	int row;

	if (col == n)        // base case - all columns filled,
	{
		print_solution(pos, n);   // print solution
		return;          // back-track one level up (another column?)
	}
	row = 0;
	while (row < n)      // try every row in this column
	{
		if (is_safe(pos, col, row))   // if queen @ col/row can be a good fit
		{
			pos[col] = row;           // place
			solve(pos, n, col + 1);   //      ... and recurse with next col
			/* --- explicit undo step (for educational purposes only) --- */
			pos[col] = -1;            // after all remaining DFS has been exhausted
			                          // reset position and try the next possibility
									  // this is called "back-tracking"
			/* in a different setup this would make a difference, but in our case
			   we are going anyway overwrite the `-1` with the next row value */
		}
		row++;
	}
}

// pos[col] stores the row index of the queen in column col
int main(int argc, char *argv[])
{
	int n;
	int *pos;

	if (argc != 2)
		return 1;
	n = atoi(argv[1]);
	if (n <= 0)
		return 0;
	pos = (int *)malloc(sizeof(int) * n);
	if (!pos)
		return 0;
	solve(pos, n, 0);
	free(pos);
	return 0;
}
