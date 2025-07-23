#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

size_t ft_strlen(char *s)
{
	size_t i;

	i = 0;
	while (s[i])
		i++;
	return i;
}

// copies a source string to a destination
void ft_strcpy(char *dest, char *src)
{
	int i;
	i = 0;
	while (src[i])
	{
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
}

// swaps two characters using pointers
void swap_chars(char *a, char *b)
{
	char tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

// sorts a string in alphabetical order using Bubble Sort
void sort_string(char *str, int len)
{
	int i;
	int j;

	i = 0;
	while (i < len - 1)
	{
		j = 0;
		while (j < len - i - 1)
		{
			if (str[j] > str[j + 1])
				swap_chars(&str[j], &str[j+1]);
			j++;
		}
		i++;
	}
}

// reverses a portion of a string from a given start to end index
void reverse_suffix(char *str, int start, int end)
{
	while (start < end)
	{
		swap_chars(&str[start], &str[end]);
		start++;
		end--;
	}
}

// generates the next lexicographical permutation of the string
// returns 1 if a new permutation is found, 0 otherwise
int next_permutation(char *str, int len)
{
	int i;
	int j;

	// 1.) find the `pivot`:
	//              the left side of the rightmost alphabetically ordered pair
	//              the right part of the string will be in descending order
	//         e.g.: str = `deacb` -> 'ac' -> pivot 'a'  -> str[i]
	i = len - 2;
	while (i >= 0 && str[i] >= str[i + 1])
		i--;
	if (i < 0)
		return 0;  // This was the last permutation

	// our pivot str[i] = str[2] = ('a')
	// 2. find the "successor" of the pivot: a char to the right of str[i] that is greater than str[i]
	//    sweeping starts again from right to left => successor 'c'
	j = len - 1;
	while (j > i && str[j] <= str[i])
		j--;
	// 3. Swap the two found characters str[j] = str[4] = 'b'
	swap_chars(&str[i], &str[j]);      //  `deacb` -> `debca`
	// 4. reverse the suffix of the string starting after the pivot  -> 'debac`
	//    we need this step, because the string after pivot is in descending order
	//    (we were using this property to find the pivot) -> by reversing it will be in ascending order
	reverse_suffix(str, i + 1, len - 1);
	return 1;
}



int main(int argc, char *argv[])
{
	char *perm_str;
	int len;

	if (argc != 2)
	{
		write(1, "\n", 1);
		return 1;
	}
	len = ft_strlen(argv[1]);
	if (len == 0)
	{
		write(1, "\n", 1);
		return 1;
	}
	perm_str = (char *)malloc(len + 1);
	if (!perm_str)
		return 1;
	ft_strcpy(perm_str, argv[1]);
	sort_string(perm_str, len);   /* start from first lexicographic word */
	while (1)
	{
		puts(perm_str);
		if (!next_permutation(perm_str, len))
			break;
	}
	free(perm_str);
	return 0;
}
