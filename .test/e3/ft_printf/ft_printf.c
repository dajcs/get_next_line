#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int ft_putchar(char c)
{
	return (write(1, &c, 1));
}

// recursively prints a hex number in lowercase format
void ft_put_hex(unsigned int num)
{
	if (num < 16)
	{
		if (num < 10)
			ft_putchar('0' + num);
		else
			ft_putchar('a' + (num - 10));
	}
	else
	{
		ft_put_hex(num / 16);
		ft_put_hex(num % 16);
	}
}

// return hex number length
int ft_hex_len(unsigned int num)
{
	int len;
	len = 0;
	while (num)
	{
		num = num / 16;
		len++;
	}
	return len;
}

// prints num in lowercase hex format
// if num = 0 prints '0'
// return nr of characters printed
int ft_print_hex(unsigned int num)
{
	if (num == 0)
		return write(1, "0", 1);
	ft_put_hex(num);
	return (ft_hex_len(num));
}

void fill_res(char *res, long long num, int len)
{
	int i;
	char c;

	if (num < 0)
	{
		num = -num;
		res[0] = '-';
	}
	else if (num == 0)
		res[0] = '0';
	i = 0;
	while (num)
	{
		c = '0' + num % 10;
		res[len - 1 - i] = c;
		num = num / 10;
		i++;
	}
	res[len] = '\0';
	return ;
}

// malloc and return string corresponding to n
char *ft_itoa(int n)
{
	char *res;
	int len;
	long long num;

	len = 0;
	if (n <= 0)
		len = 1;
	num = n;
	while (num)
	{
		num = num / 10;
		len++;
	}
	res = (char *)malloc(len + 1);
	if (!res)
		return NULL;
	fill_res(res, (long long)n, len);
	return res;
}



// prints str, handles NULL by printing (null).
// return the number of characters printed
int ft_print_str(char *str)
{
	int i;

	i = 0;
	if (!str)
	{
		write(1, "(null)", 6);
		return 6;
	}
	while (str[i])
	{
		write(1, &str[i], 1);
		i++;
	}
	return i;
}


// prints a signed decimal number
// returns the number of characters printed
int ft_print_nbr(int n)
{
	int len;
	char *num;

	len = 0;
	num = ft_itoa(n);
	len = ft_print_str(num);
	free(num);
	return len;
}


int ft_formats(va_list args, char format)
{
	int print_length;

	print_length = 0;
	if (format == 's')
		print_length += ft_print_str(va_arg(args, char *));
	else if (format == 'd')
		print_length += ft_print_nbr(va_arg(args, int));
	else if (format == 'x')
		print_length += ft_print_hex(va_arg(args, unsigned int));
	return print_length;
}

int ft_printf(const char *str, ...)
{
	int i;
	va_list args;
	int print_length;

	i = 0;
	print_length = 0;
	va_start(args, str);
	while (str[i])
	{
		if (str[i] == '%')
		{
			print_length += ft_formats(args, str[i+1]);
			i++;
		}
		else
		{
			print_length += write(1, &str[i], 1);
		}
		i++;
	}
	va_end(args);
	return print_length;
}

int main(int argc, char *argv[])
{
	int len;

	if (argc < 2)
		return 1;
	len = ft_printf("str: \"%s\"\n", argv[1]);
	ft_printf("len: %d\n", len);
	ft_printf("hex: %x\n", atoi(argv[1]));
}

