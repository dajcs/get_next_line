Write a C program `permutations`.
- the program will print all the permutations of a string given as argument.
- the solutions must be given in alphabetical order
- the input string doesn't contain duplicates (e.g., 'abccd')

For example this should work:
```bash
$> ./permutations a | cat -e
a$
$> ./permutations ab | cat -e
ab$
ba$
$> ./permutations abc | cat -e
abc$
acb$
bac$
bca$
cab$
cba$
```

- allowed external functions : `puts`, `malloc`, `calloc`, `realloc`, `free`, `write`
- please respect the norminette rules:
  - only while loops are allowed
  - no ternary operators are allowed
