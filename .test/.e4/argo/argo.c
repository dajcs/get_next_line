/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argo.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/25 09:48:20 by anemet            #+#    #+#             */
/*   Updated: 2025/09/25 13:18:50 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*

Assignment name: argo
Expected files: argo.c
Allowed functions: getc, ungetc, printf, malloc, calloc, realloc,
					free, isdigit, fscanf, write
-----------------
Write a function argo that will parse a json file in the structure declared in argo.h:

int	argo(json *dst, FILE *stream);

	dst	- is the pointer to the AST that you will create
	stream	- is the file to parse (man FILE)

Your function will return 1 for success and -1 for failure.
If an unexpected token is found you will print the following message in stdout:
"Unexpected token '%c'"

or if the token is EOF:
"Unexpected end of input"

Only handle numbers, strings and maps.
Numbers will only be basic ints like in scanf("%d")
Handle escaping in the strings only for backslashes and quotation marks (no \u ...)
Don't handle spaces -> consider them as invalid tokens.

In case of doubt how to parse json, read rfc8259.
But you won't need it as the format is simple. Tested with the main,
the output should be exactly the same as the input (except for errors).
There are some functions in argo.c that might help you.

Examples that should work:

$> echo -n '1' | ./argo /dev/stdin | cat -e
1$
$> echo -n '"bonjour"' | ./argo /dev/stdin | cat -e
"bonjour"$
$> echo -n '"escape! \" "' | ./argo /dev/stdin | cat -e
"escape! \" "$
$> echo -n '{"tomatoes":42,"potatoes":234}' | ./argo /dev/stdin | cat -e
{"tomatoes":42,"potatoes":234}$
$> echo -n '{"recursion":{"recursion":{"recursion":{"recursion":"recursion"}}}}' | ./argo /dev/stdin | cat -e
{"recursion":{"recursion":{"recursion":{"recursion":"recursion"}}}}$
$> echo -n '"unfinished string' | ./argo /dev/stdin | cat -e
unexpected end of input$
$> echo -n '"unfinished string 2\"' | ./argo /dev/stdin | cat -e
unexpected end of input$
$> echo -n '{"no value?":}' | ./argo /dev/stdin | cat -e
unexpected token '}'$

*/


#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


typedef struct	json
{
	enum
	{
		MAP,
		INTEGER,
		STRING
	} type;

	union	// means one of: map / int / char *
			// saving memory by reserving the biggest member size (map in our case)
	{
		struct
			{
				struct pair	*data;  // pointer to pair* struc:
									//	- forward definition allowed
									//	- multiple key:value pairs can be stored
									//		similar when having e.g., char *
				size_t		size;
			} map;
		int	integer;
		char	*string;
	};
}	json;

typedef struct	pair {
	char	*key;
	json	value;
}	pair;

void	free_json(json j);
int	argo(json *dst, FILE *stream);
int parse_json(FILE *stream, json *out);

// take a look on the next c
// put it back into the stream, Return c
int	peek(FILE *stream)
{
	int	c = getc(stream);
	ungetc(c, stream);
	return c;
}

// print unexpected token / EOF for next c in stream
void	unexpected(FILE *stream)
{
	if (peek(stream) != EOF)
		printf("unexpected token '%c'\n", peek(stream));
	else
		printf("unexpected end of input\n");
}

// if next char in stream == c: consume it, Return 1
//	else: Return 0
int	accept(FILE *stream, char c)
{
	if (peek(stream) == c)
	{
		(void)getc(stream);
		return 1;
	}
	return 0;
}

// if next char in stream == c: consume it, Return 1
//	else: print unexpected token/EOF, Return 0
int	expect(FILE *stream, char c)
{
	if (accept(stream, c))
		return 1;
	unexpected(stream);
	return 0;
}

/* parse INTEGER
	- only base-10 integer
	- no hex, no float
*/
int parse_number(FILE *stream, json *out)
{
	int value;
	if (fscanf(stream, "%d", &value) != 1)  // returns how many %d integers parsed
		return -1;
	// store INT to json
	out->type = INTEGER;
	out->integer = value;
	return 1;
}

/* parse STRING
	- starts and ends with "
	- allowed escape sequences: \\ and \"
	- space outside "" are invalid
	Return 1 on success, -1 on failure
*/
int parse_string(FILE *stream, json *out)
{
	int len = 0;
	int cap = 16; // initial buffer size

	// starts with "
	if (!expect(stream, '"'))
		return -1;

	// initial memory reserve for buf
	char *buf = malloc(cap);
	if (!buf)
		return -1;

	for(;;)
	{
		// consume next char
		int c = getc(stream);

		if (c == EOF)   // hit EOF before closing quote
		{
			free(buf);
			unexpected(stream);
			return -1;
		}

		if (c == '"')  // closing quote
			break;

		if (c == '\\') // possible escape sequence
		{
			// next char must be \ or "
			int esc = getc(stream);
			if (esc != '"' && esc != '\\')
			{
				putc(esc, stream);
				unexpected(stream);
				free(buf);
				return -1;
			}
			// replace c = esc
			c = esc;
		}

		// expand buf if needed
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
		// put c into buf
		buf[len] = c;
		len++;
	}
	buf[len] = '\0';

	// store buf into json STRING
	out->type = STRING;
	out->string = buf;
	return 1;
}

/* parse MAP (object)
	- key:value pairs separated by comma ','
	- key is a string
	- starts with '{' and ends with '}'
	Return 1 on success, -1 on failure
*/
int parse_map(FILE *stream, json *out)
{
	out->type = MAP;
	out->map.data = NULL;
	out->map.size = 0;

	// starts with '{'
	if (!expect(stream, '{'))
		return -1;

	// special case of empty map "{}"
	if (accept (stream, '}'))
		return 1;

	for(;;)
	{
		json keyjson;
		if (parse_string(stream, &keyjson) != 1)  // key must be a string
			return -1;
		if (!expect(stream, ':'))  // key:value separated by ':'
		{
			free_json(keyjson);
			return -1;
		}
		json valjson;
		if (parse_json(stream, &valjson) != 1)  // value a json object, recursive call
		{
			free_json(keyjson);
			return -1;
		}

		// reserve memory for the new pair
		pair *tmp = realloc(out->map.data, sizeof(pair) * (out->map.size + 1));
		if (!tmp)
		{
			free_json(keyjson);
			free_json(valjson);
			return -1;
		}
		out->map.data = tmp;  // old data is on place

		// store new pair
		out->map.data[out->map.size].key = keyjson.string;
		out->map.data[out->map.size].value = valjson;
		out->map.size++;

		// End of map?
		if (accept(stream, '}'))
			break;

		// Otherwise we're expecting a comma ','
		if (!expect(stream, ','))
			return -1;
	}
	return 1;
}

// dispatche parsing MAP / STRING / INT
// Return 1 on success, -1 on failure
int parse_json(FILE *stream, json *out)
{
	int c;
	c = peek(stream);

	if (c == '{')
		return (parse_map(stream, out));
	if (c == '"')
		return (parse_string(stream, out));
	if (isdigit(c) || c == '+' || c == '-')
		return (parse_number(stream, out));

	unexpected(stream);
	return -1;
}

int	argo(json *dst, FILE *stream)
{
	return parse_json(stream, dst);
}

void	free_json(json j)
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
			break ;
		case STRING:
			free(j.string);
			break ;
		default:
			break ;
	}
}

void	serialize(json j)
{
	switch (j.type)
	{
		case INTEGER:
			printf("%d", j.integer);
			break ;
		case STRING:
			putchar('"');
			for (int i = 0; j.string[i]; i++)
			{
				if (j.string[i] == '\\' || j.string[i] == '"')
					putchar('\\');
				putchar(j.string[i]);
			}
			putchar('"');
			break ;
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
			break ;
	}
}

int	main(int argc, char **argv)
{
	if (argc != 2)
		return 1;
	char *filename = argv[1];
	FILE *stream = fopen(filename, "r");
	json	file;
	if (argo (&file, stream) != 1)
	{
		free_json(file);
		return 1;
	}
	serialize(file);
	printf("\n");
}
