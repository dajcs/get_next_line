
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
		if (is_safe(pos, col, row))   // the new queen is going to kill?
		{
			pos[col] = row;           // we're good with this row, try next column
			solve(pos, n, col + 1);
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
