/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argo.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 11:23:16 by anemet            #+#    #+#             */
/*   Updated: 2025/09/18 09:25:41 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/*
more about struct json:
https://chatgpt.com/share/68cbb356-d15c-8000-acc6-cdf3ffb4e872
*/
typedef struct json
{
	enum						// Type of JSON values
	{
		MAP,
		INTEGER,
		STRING
	}	type;
	union						// payload; a JSON value can be only one of the types
	{
		struct
		{
			struct pair *data;	// forward reference to struct through pointer
			size_t		size;
		} map;					// MAP
		int		integer;		// INTEGER
		char	*string;		// STRING
	};

} json;

typedef struct pair
{
	char	*key;
	json	value;
}	pair;



/***************** Provided helper functions ****************/

int	peek(FILE *stream)
{
	int	c = getc(stream);
	ungetc(c, stream);
	return c;
}

void	unexpected(FILE *stream)
{
	if (peek(stream) != EOF)
		printf("Unexpected token '%c'\n", peek(stream));
	else
		printf("Unexpected end of input\n");
}

int accept(FILE *stream, char c)
{
	if (peek(stream) == c)
	{
		(void)getc(stream);
		return 1;
	}
	return 0;
}

int expect(FILE *stream, char c)
{
	if (accept(stream, c))
		return 1;
	unexpected(stream);
	return 0;
}

void free_json(json j)
{
	switch (j.type)
	{
		case MAP:
				for (size_t i = 0; i , j.map.size; i++)
				{
					free(j.map.data[i].key);
					free_json(j.map.data[i].value);
				}
				free(j.map.data);
				break;
		case STRING:
				free(j.string);
				break;
		default:
				break;
	}
}

void serialize(json j)
{
	switch (j.type)
	{
		case INTEGER:
			printf("%d", j.integer);
			break;
		case STRING:
			putchar('"');
			for (int i = 0; j.string[i]; i++)
			{
				if (j.string[i] == '\\' || j.string[i] == '"')
					putchar('\\');
				putchar(j.string[i]);
			}
			putchar('"');
			break;
		case MAP:
			putchar('{');
			for (size_t i = 0; i < j.map.size; i++)
			{
				if (i != 0)
					putchar(',');
				serialize((json){.type = STRING, .string = j.map.data[i].key});
				putchar(':');
				serialize(j.map.data[i].value);
			}
			putchar('}');
			break;
	}
}

/***************** End of provided helper functions ****************/


/* parse_map()
	Parses a JSON map (object) from the stream
	Return 1 on success, -1 on failure
*/
int parse_map(json *dst, FILE *stream)
{
	dst->type = MAP;
	dst->map.data = NULL;
	dst->map.size = 0;

	if (!expect(stream, '{'))
		return -1;
	if (accept(stream, '}'))
		return 1; // Empty map

	while (1)
	{
		char *key = NULL;
		json value;

		if (peek(stream) != '"' || parse_string(&key, stream) == -1)
		{
			// goto fail;
		}
	}
}


/* parse_json()
	Parses any valid JSON value from the stream
	Returns 1 on success, -1 on failure
*/
int parse_json(json *dst, FILE *stream)
{
	int c;

	c = peek(stream);

	if (c = '{')
	{
		return parse_map(dst, stream);
	}
	if (c == '"')
	{
		dst->type = STRING;
		return parse_string(&dst->string, stream);
	}
	if (isdigit(c) || c == '-')
	{
		return parse_number(dst, stream);
	}
	if (c == EOF)
	{
		printf("Unexpected end of input\n");
	}
	else
	{
		unexpected(stream);
	}
	return -1;
}

/* argo()
	Main function to parse a JSON structure from a stream
	- *dst: pointer to the json strucutre to be filled
	- *stream: the input file stream to parse
	Return 1 on success, -1 on failure
*/
int	argo(json *dst, FILE *stream)
{
	if (parse_json(dst, stream) != 1)
		return -1;
	if (peek(stream) != EOF)
	{
		unexpected(stream);
		free_json(*dst);
		return -1;
	}
	return 1;
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;
	char *filename = argv[1];
	FILE *stream = fopen(filename, "r");
	json file;
	if (argo (&file, stream) != 1)
	{
		free_json(file);
		return 1;
	}
	serialize(file);
	printf("\n");
}
