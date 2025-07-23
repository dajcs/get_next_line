Write a `C` function `ft_scanf` which is going to mimic the real `scanf` with the following constraints:
- it will manage only the following conversion: `s`, `d`, and `c`
- doesn't have to handle the options: `*`, `m` and `'`
- doesn't have to handle the maximum field width
- doesn't have to handle the types modifier characters like: `h`, `hh`, `l`, etc.
- doesn't have to handle the conversions beginning with `%n$`
- Whitespace in the format string should be handled correctly (i.e., multiple spaces and newlines should be skipped in the input).
- Memory management and correct handling of stdin buffering are expected.

- prototype:
```c
int ft_scanf(const char *, ... );
```
- Expected files   : ft_scanf.c
- Allowed functions: fgetc, ungetc, ferror, feof, isspace, isdigit, stdin, va_start, va_arg, va_copy, va_end
- the file `ft_scanf_bp.c` contains the boilerplate/boring parts of the exercise
