Write a C program called `powerset` that should display all the subsets of a set whose sum of its elements is equal to the first argument.
The numbers of the set are from the second argument to the last.
A subset must not be displayed a second time.

ex1
```bash
ex1
./a.out 5 2 3 | cat -e
2 3$
```
ex2
```bash
./a.out 12 5 7 4 3 2 | cat -e
5 7$
5 4 3$
7 3 2$
```

They do not test invalid sequences such as:
```bash
./a.out 5 12 18 | cat -e
```

The order of the lines is not important, however the order of the input is.
This is valid:
```bash
./a.out 5 3 2 1 4 | cat -e
3 2$
1 4$
```

This is valid
```bash
./a.out 5 3 2 1 4 | cat -e
1 4$
3 2$
```

This isn't valid:
```bash
./a.out 5 3 2 1 4 | cat -e
4 1$
2 3$
```

Be careful in the case where the number to reach is 0 because adding together 'nothing' is considered as a possibility, so you will need to display an empty line:
```bash
./a.out 0 1 -1 2 -2 | cat -e
$
-1 1 2 -2$
-1 1$
2 -2$
```

- allowed external functions : `atoi`, `printf`
- please respect the norminette rules:
  - only while loops are allowed
  - no ternary operators are allowed
