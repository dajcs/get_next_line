/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argo.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/17 11:23:16 by anemet            #+#    #+#             */
/*   Updated: 2025/09/18 14:27:22 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>            // isdigit()
#include <string.h>
#include <stdlib.h>

/*
more about JSON struct:
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
				for (size_t i = 0; i < j.map.size; i++)
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

// Forward declaration of the recursive parser
static int parse_json(FILE *stream, json *out);

/* parse_number()
	Parse an integer
	- Only bas-10 ints (scanf-like)
	- no floats, no hex,...
*/
int parse_number(FILE *stream, json *out)
{
	int value;
	if (fscanf(stream, "%d", &value) != 1)	// try to read an int
		return -1;
	out->type = INTEGER;
	out->integer = value;
	return 1;
}

/* parse_string()
	Parsing a string
	- starts and ends with "
	- allowed escapes: \" and \\
	- spaces outside "" are invalid tokens
	Return 1 on success, -1 on failure
*/
int parse_string(FILE *stream, json *out)
{
	if (!accept(stream, '"'))	// must start with a quote
	{
		unexpected(stream);
		return -1;
	}

	size_t cap = 16;	// initial buffer size
	size_t len = 0;
	char *buf = malloc(cap);
	if (!buf)
		return -1;

	for (;;)
	{
		int c = getc(stream);
		if (c == EOF)		// hit EOF before closing quote
		{
			free(buf);
			unexpected(stream);
			return -1;
		}
		if (c == '"')		// closing quote found
			break;

		if (c == '\\')		// c == backslash -> possible escape sequence
		{
			int esc = getc(stream);
			if (esc == EOF)
			{
				free(buf);
				unexpected(stream);
				return -1;
			}
			// only \" and \\ is allowed
			// (next c should be backslash or quotation mark)
			if (esc != '\\' && esc != '"')
			{
				free(buf);
				printf("unexpected token '%c'\n", esc);
				return -1;
			}
			c = esc;	// replace c with the actual character
		}

		// Ensure space in buffer
		if (len + 1 >= cap)
		{
			cap *= 2;
			char *tmp = realloc(buf, cap);
			if (!tmp)
			{
				free(buf);
				return -1;
			}
			buf = tmp;
		}
		buf[len++] = c;
	}
	buf[len] = '\0';

	out->type = STRING;
	out->string = buf;
	return 1;
}

/* parse_map()
	Parse a map (JSON object)
	- starts with '{'
	- contains key:value pairs, separated by commas
	- keys must be strings
	- ends with '}'
	Return 1 on success, -1 on failure
*/
int parse_map(FILE *stream, json *out)
{
	if (!accept(stream, '{'))
	{
		unexpected(stream);
		return -1;
	}

	out->type = MAP;
	out->map.data = NULL;
	out->map.size = 0;

	if (accept(stream, '}'))
		return 1;				// special case: empty map "{}"

	for (;;)
	{
		json keyjson;
		if (parse_string(stream, &keyjson) != 1)	// key must be string
			return -1;
		if (!expect(stream, ':'))		// Expect ':'
		{
			free_json(keyjson);
			return -1;
		}
		json value;
		if (parse_json(stream, &value) != 1)	// parse value (recursive call)
		{
			free_json(keyjson);
			return -1;
		}

		// Grow the array of pairs
		pair *tmp = realloc(out->map.data, sizeof(pair) * (out->map.size + 1));
		if (!tmp)
		{
			free_json(keyjson);
			free_json(value);
			return -1;
		}
		out->map.data = tmp;

		// Store new pair
		out->map.data[out->map.size].key = keyjson.string;
		out->map.data[out->map.size].value = value;
		out->map.size++;

		// End of map?
		if (accept(stream, '}'))
			break;

		// Otherwise we're expecting a comma
		if (!expect(stream, ','))
			return -1;
	}
	return 1;
}

/* parse_json
	Main dispatcher: decide what type of JSON value is next
	- *stream: the input file stream to parse
	- *out: pointer to the json strucutre to be filled
	Return 1 on success, -1 on failure
*/
int parse_json(FILE *stream, json *out)
{
	int c;

	c = peek(stream);

	if (c == '{')
		return parse_map(stream, out);			// MAP (JSON object)
	if (c == '"')
		return parse_string(stream, out);		// STRING
	if (isdigit(c) || c == '-' || c == '+')
		return parse_number(stream, out);		// INTEGER

	unexpected(stream);
	return -1;
}

/* argo()
	Entry point required by the assignment
	to parse a JSON structure from a stream
	- *dst: pointer to the json strucutre to be filled
	- *stream: the input file stream to parse
	Return 1 on success, -1 on failure
*/
int argo(json *dst, FILE *stream)
{
	return parse_json(stream, dst);
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return 1;
	char *filename = argv[1];
	FILE *stream = fopen(filename, "r");
	json res;
	if (argo (&res, stream) != 1)
	{
		free_json(res);
		return 1;
	}
	serialize(res);
	printf("\n");
}
