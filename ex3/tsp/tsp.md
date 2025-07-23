Write a C program called `tsp` that is going to solve the travelling salesman problem.
More generally we should find the shortest CLOSED CURVE containing n given points in the plane.

For example the following set of points:
```csv
0, 0
1, 0
2, 0
0, 1
1, 1
2, 1
1, 2
2, 2
```

which can be represented on plane as follows:
```
+ + +
+ + +
  + +
```

and the shortest path is:
```
 _____
|__   |
   |__|
```

and the program should print the length of this path that is:
```
8.00
```

- the program will read a set of city coordinates in the form `%f, %f\n` from the standard input and will print the length of the shortest possible path containing all these cities under the form `%.2f`.
- the maximum number of cities is 11
- the file `tsp_give.c` contains the boilerplate/boring parts of the exercise
- hint: to user `sqrtf` add `-lm` to the compilation command

For example this should work:
```bash
$> cat square.txt
1, 1
0, 1
1, 0
0, 0
$> ./tsp < square.txt | cat -e
4.00$
```

- allowed external functions : write, sqrtf, getline, fseek, fscanf, ferror, feof, fabsf, memcpy, fprintf, fclose, malloc, calloc, realloc, free, fopen, errno, stderr, stdin, stdout
- please respect the norminette rules:
  - only while loops are allowed
  - no ternary operators are allowed
