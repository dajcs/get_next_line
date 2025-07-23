#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>


/* match_space: Consumes whitespace characters from the stream
- reads characters from f while they are whitespace.
- pushes first non-whitespace character back to the stream
- parameters: f - input file stream
- return: 1 on success, -1 on read error                       */
int match_space(FILE *f)
{
	int c;

	c = fgetc(f);
	while (isspace(c))
		c = fgetc(f);
	if (c != EOF)
		ungetc(c, f);
	if (ferror(f))
		return -1;
	return 1;
}

/* match_char: Matches a literal character from the stream
       if match return 1
       if mismatch push back character and return 0
- parameters:
  - f: the input file stream
  - c: the character to match                              */
int match_char(FILE *f, char c)
{
	int in_c;

	in_c = fgetc(f);
	if (in_c == c)
		return 1;
	if (in_c != EOF)
		ungetc(in_c, f);
	return 0;
}

/* schan_char: Scans a single charactr for the %c conversion
    Reads the next character from the stream and assigns it
    Does not skip whitespace
	return 1 on successful assignment, 0 on input failure
	parameters:
		f: the input file stream
		ap: the va_list containing the destination char ptr */
int scan_char(FILE *f, va_list ap)
{
	char *p_char;
	int c;

	p_char = va_arg(ap, char *);
	c = fgetc(f);
	if (c == EOF)
		return 0;
	*p_char = (char)c;
	return 1;
}

/* scan_int: Scans a decimal integer for the %d conversion
	reads an optional sign (+/-) followed by decimal digits
	returns 1 on successful assignment, 0 on matching failure
	parameters:
		f: the input file stream
		ap: the va_list containing the destination char ptr */
int scan_int(FILE *f, va_list ap)
{
	long result;
	int sign;
	int c;
	int digits_read;
	int *p_int;

	result = 0;
	sign = 1;
	digits_read = 0;
	c = fgetc(f);
	if (c == '-')
	{
		sign = -1;
		c = fgetc(f);
	}
	else if (c == '+')
		c = fgetc(f);
	while (isdigit(c))
	{
		digits_read = 1;
		result = result * 10 + (c - '0');
		c = fgetc(f);
	}
	if (c != EOF)
		ungetc(c, f);
	if (digits_read)
	{
		p_int = va_arg(ap, int *);
		*p_int = (int)(result * sign);
		return 1;
	}
	return 0;
}

/* scan_string: Scans a string of non-whitespace characters for %s conversion
	return 1 on successful assignment, 0 on matching failure
	parameters:
		f: the input file stream
		ap: the va_list containing the destination char ptr */
int scan_string(FILE *f, va_list ap)
{
	char *p_str;
	int c;
	int char_read;

	p_str = va_arg(ap, char *);
	char_read = 0;
	c = fgetc(f);
	while (c != EOF && !isspace(c))
	{
		char_read = 1;
		*p_str = (char)c;
		p_str++;
		c = fgetc(f);
	}
	if (c != EOF)
		ungetc(c, f);
	*p_str = '\0';
	return char_read;
}


int	match_conv(FILE *f, const char **format, va_list ap)
{
	switch (**format)
	{
		case 'c':
			return scan_char(f, ap);
		case 'd':
			match_space(f);
			return scan_int(f, ap);
		case 's':
			match_space(f);
			return scan_string(f, ap);
		case EOF:
			return -1;
		default:
			return -1;
	}
}

int ft_vfscanf(FILE *f, const char *format, va_list ap)
{
	int nconv = 0;

	int c = fgetc(f);
	if (c == EOF)
		return EOF;
	ungetc(c, f);

	while (*format)
	{
		if (*format == '%')
		{
			format++;
			if (match_conv(f, &format, ap) != 1)
				break;
			else
				nconv++;
		}
		else if (isspace(*format))
		{
			if (match_space(f) == -1)
				break;
		}
		else if (match_char(f, *format) != 1)
			break;
		format++;
	}

	if (ferror(f))
		return EOF;
	return nconv;
}

/* ft_scanf: Read formatted data from standard input
- parameters:
  - format: the format string
  - ...: variable arguments for the conversions
- return the number of successfully assigned, or EOF on error */
int ft_scanf(const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = ft_vfscanf(stdin, format, ap);
	va_end(ap);
	return ret;
}


#include <stdio.h>
#include <string.h>

void run_test(const char *text)
{
	int x;
	char buf[32];
	char ch;
	int n;
	FILE *f;
	FILE *f_ft;

	/* -------- ft_scanf on a private memory stream ------- */
	{
		f_ft = fmemopen((void *)text, strlen(text), "r");
		if (!f_ft)
			return;
		stdin = f_ft;
		n = ft_scanf("%d %c %s", &x, &ch, buf);
		printf("ft_scanf: n = %d, x=%d, ch='%c', buf='%s'\n", n, x, ch, buf);
	}
	{
		f = fmemopen((void *)text, strlen(text), "r");
		if (!f)
			return;
		stdin = f;
		n = scanf("%d %c %s", &x, &ch, buf);
		printf("scanf   : n = %d, x=%d, ch='%c', buf='%s'\n", n, x, ch, buf);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("use: %s <format>\n", argv[0]);
		return 1;
	}
	run_test(argv[1]);
	return 0;
}
