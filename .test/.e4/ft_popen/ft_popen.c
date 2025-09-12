/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_popen.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/12 14:09:32 by anemet            #+#    #+#             */
/*   Updated: 2025/09/12 14:46:42 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int	ft_popen(const char *file, char *const argv[], char type)
{
	int	p[2];
	int	pid;

	if (type != 'r' && type != 'w')
		return (-1);
	if (pipe(p) == -1)  // create pipe:       read p[0] --- p[1] write
		return (-1);    //                     pipe out <<< pipe in
	if ((pid = fork()) == -1)
	{
		close(p[0]);
		close(p[1]);
		return (-1);
	}
	if (pid == 0) // child process
	{
		// 'r' (read) mode: The parent process needs to read the output of the child
		if (type == 'r')
		{
			close(p[0]);
			if (dup2(p[1], STDOUT_FILENO) == -1)
				exit(1);
			close(p[1]);
		}
		// 'w' (write) mode: The parent process wants to write to the child's input
		else // type == 'w'
		{
			close(p[1]);
			if (dup2(p[0], STDIN_FILENO) == -1)
				exit(1);
			close(p[0]);
		}
		execvp(file, argv);
	}
	else // Parent process
	{
		if (type == 'r')
		{
			close(p[1]);
			return (p[0]);
		}
		else // type == 'w'
		{
			close(p[0]);
			return (p[1]);
		}
	}
	return (-1); // Should not be reached
}
