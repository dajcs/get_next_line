#include <stdlib.h>
#include <stdio.h>

void print_subset(int *subset, int size)
{
	int i;

	i = 0;
	while (i < size)
	{
		printf("%d", subset[i]);
		if (i < size - 1)
			printf(" ");
		i++;
	}
	printf("\n");
}

void find_subsets(int target, int *numbers, int count)
{
	int i;
	int j;
	int current_sum;
	int *subset;
	int subset_size;

	subset = (int *)malloc(sizeof(int) * count);
	if (!subset)
		return;
	i = 0;
	while (i < (1 << count))
	{
		current_sum = 0;
		subset_size = 0;
		j = 0;
		while (j < count)
		{
			// i >> j   shift the binary representation of `i` to right by `j` positions
			if ((i >> j) & 1)   // this is checking if the j-th bit of i is 0 or 1
			{
				current_sum += numbers[j];
				subset[subset_size] = numbers[j];
				subset_size++;
			}
			j++;
		}
		if (current_sum == target)
			print_subset(subset, subset_size);
		i++;
	}
	free(subset);
}

int main(int argc, char **argv)
{
	int target;
	int *numbers;
	int count;
	int i;

	if (argc < 2)
		return 1;
	target = atoi(argv[1]);
	count = argc - 2;
	numbers = (int *)malloc(sizeof(int) * count);
	if (!numbers)
		return 1;
	i = 0;
	while (i < count)
	{
		numbers[i] = atoi(argv[i + 2]);
		i++;
	}
	find_subsets(target, numbers, count);
	free(numbers);
	return 0;
}
