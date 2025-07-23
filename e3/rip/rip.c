#include <stdio.h>

// is_val - has a dual purpose:
// calculate the minimum number of parentheses to remove
// check if a string is currently balanced
int is_val(char *str)
{
	int opened;
	int closed;
	int i;

	opened = 0;
	closed = 0;
	i = 0;
	while (str[i])
	{
		if (str[i] == '(' )
			opened++;       // count every opened parenthesis
		else if (str[i] == ')' )
		{
			if (opened > 0)
				opened--;   // first the closed are lowering the opened counts
			else
				closed++;  // or we increment the closing side
		}
		i++;
	}
	return (opened + closed);
}

// rip: recursive function that performs a "backtracking" search to find the solutions
// parameters:
//  - str: the current string being worked on
//  - must_fix: the target number of parentheses to remove (calculated once in main / is_val )
//  - n_fix: how many parentheses have been removed so far in this recursive path
//  - pos: the starting index for the loop to avoid duplicate combinations
void rip(char *str, int must_fix, int n_fix, int pos)
{
	int i;
	char c;

	// 1. Base Case: Check if we have a potential solution
	if (must_fix == n_fix && !is_val(str))
	{
		puts(str);  // print a solution
		return;
	}
	// too much stuff removed, don't go further
	if (n_fix >= must_fix)
		return;

	// 2. Recursive Step: iterate through the string to find the next character to remove
	i = pos;
	while (str[i])
	{
		if (str[i] == '(' || str[i] == ')' )
		{
			// A. Try removing the character at index 'i'
			c = str[i];    // remember str[i] for back-track
			str[i] = ' ';  // remove () = replacing by space

			// B. Recurse: Look for the next character to remove
			//  - n_fix + 1: We have now removed one more character
			//  - i: Start the next search from the current position to avoid permutations
			rip(str, must_fix, n_fix + 1, i);

			// C. Backtrack: Undo the change
			str[i] = c; // Put the character back to explore other possibilities
		}
		i++;
	}
}

int main(int argc, char *argv[])
{
	int m_fix;

	if (argc != 2)
		return 1;
	// 1. Calculates the minimum number of fixes needed (just once)
	m_fix = is_val(argv[1]);

	// 2. Start the recursive search
	// argv[1] - the string to modify
	// m_fix: The target number of removals
	// 0: We start with 0 characters removed
	// 0: We start search from the beginning of the string
	rip(argv[1], m_fix, 0, 0);

	return 0;
}
