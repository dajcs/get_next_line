/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line_utils.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 10:42:51 by anemet            #+#    #+#             */
/*   Updated: 2025/06/17 11:17:56 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line.h"

// return string length
size_t	gnl_strlen(char *s)
{
	size_t	i;

	i = 0;
	while (s && s[i])
		i++;
	return (i);
}

// return pointer to first occurence of 'c' ('c' can be '\0' as well)
// return NULL if 'c' not in '*s'
char	*gnl_strchr(char *s, int c)
{
	int	i;

	i = 0;
	while (s && s[i])
	{
		if (s[i] == (char)c)
			return (s + i);
		i++;
	}
	if (s && c == 0)
		return (s + i);
	return (NULL);
}

// return str = s1 + s2
// free s1
char	*gnl_strjoin(char *s1, char *s2)
{
	size_t	i;
	size_t	j;
	char	*str;

	str = (char *)malloc(gnl_strlen(s1) + gnl_strlen(s2) + 1);
	if (!str)
		return (NULL);
	i = 0;
	while (s1 && s1[i])
	{
		str[i] = s1[i];
		i++;
	}
	j = 0;
	while (s2 && s2[j])
	{
		str[i + j] = s2[j];
		j++;
	}
	str[i + j] = '\0';
	free(s1);
	return (str);
}

// allocate, copy and return a slice of s[start .. start + len]
char	*gnl_substr(char *s, unsigned int start, size_t len)
{
	size_t	i;
	char	*sub;

	if (!s || start >= gnl_strlen(s))
		return (NULL);
	if (len > gnl_strlen(s + start))
		len = gnl_strlen(s + start);
	sub = malloc(len + 1);
	if (!sub)
		return (NULL);
	i = 0;
	while (i < len)
	{
		sub[i] = s[start + i];
		i++;
	}
	sub[i] = '\0';
	return (sub);
}
