Write a C program called `filter` that takes a single argument `s`.
The program then reads from STDIN and write all the content read to STDOUT, except that every occurence of `s` must be replaced by `*`-s (as many as the length of `s`).

example:

```bash
./filter bonjour
# will behave in the same way as:
# sed 's/bonjour/*******/g'

./filter abc
# will behave in the same way as:
# sed's/abc/***/g'
```

```bash
$> echo 'abcdefaaaabcdeabcabcdabc' | ./filter abc | cat -e
***defaaa***de******d***$

$> echo 'ababcabababc' | ./filter ababc | cat -e
*****ab*****$
$>
```

In case of error during read or malloc the program writes: "Error: " followed by the error message and should return 1.
If the program is called without argument or with an empty argument or with multiple arguments it must return 1.

All allocated memory must be reed properly.

Only while loops are allowed. No ternary operators are allowed.

Allowed functions: read, write, strlen, memmem, memmove, malloc, calloc, realloc, free, printf, fprintf, stdout, stderr, perror

Assignment name: filter

Expected files: filter.c

NOTES:
```c
// memmem includes:
                #define _GNU_SOURCE
                #include <string.h>

// perror includes:
                #include <errno.h>

// read includes:
                #include <fcntl.h>
```
