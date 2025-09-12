# ft_popen project

- Expected files   : `ft_popen.c`
- Allowed functions: `pipe`, `fork`, `dup2`, `execvp`, `close`, `exit`

---

prototype:
```c
int ft_popen(const char *file, char *const argv[], char type);
```

The function must launch the executable file with the arguments `argv` (using `execvp`). \
If type is `r` the function must return a file descriptor connected to the output of the command.\
If type is `w` the function must return a file descriptor connected to the input of the command. \
In case of error or invalid parameter the function must return -1.

Hints:
- Do not leak file descriptors!
- This exercise is inspired by the libc's popen().

For example, the function could be used like that:

```c
int main()
{
    int  fd;
    char *line;

    fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
    while ((line = get_next_line(fd)))
        ft_putstr(line);
    return (0);
}
```

Another example:

```c
int	main() {
	int	fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
	dup2(fd, 0);
	fd = ft_popen("grep", (char *const []){"grep", "c", NULL}, 'r');
	char	*line;
	while ((line = get_next_line(fd)))
		printf("%s", line);
}
```



## explanation of the second example, which replicates the functionality of `ls | grep "c"`:

### Step 1: `int fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');`

1.  **Action:** The first `ft_popen` is called to execute the `ls` command.
2.  **`type = 'r'`:** The `'r'` argument tells `ft_popen` that we want to *read* from the command's standard output.
3.  **What Happens:**
    *   `ft_popen` creates a child process.
    *   This child process executes `ls`.
    *   The standard output of the `ls` command (which would normally go to your terminal) is redirected to a pipe.
    *   The parent process (your `main` function) gets back a file descriptor, now stored in the `fd` variable. This `fd` is the *read end* of the pipe.
4.  **Result:** At this point, the `ls` command is running, and its output (the list of files and directories) is being sent into the pipe. The `main` function holds the key (`fd`) to read that output from the other side of the pipe.



### Step 2: `dup2(fd, 0);`

1.  **Action:** This is the crucial line that connects the two commands. `dup2(fd, 0)` duplicates the file descriptor `fd` onto file descriptor `0`.
2.  **File Descriptor `0`:** In C and Unix-like systems, file descriptor `0` is always *standard input* (STDIN).
3.  **What Happens:** The standard input of your `main` process is closed, and it is replaced with the read end of the pipe that is connected to the output of `ls`.
4.  **Result:** Now, if any process created by `main` tries to read from its standard input, it won't be reading from the keyboard anymore. Instead, it will be reading the output of the `ls` command.



### Step 3: `fd = ft_popen("grep", (char *const []){"grep", "c", NULL}, 'r');`

1.  **Action:** A second `ft_popen` is called to execute `grep "c"`.
2.  **What Happens:**
    *   `ft_popen` creates a *new* child process to run `grep`.
    *   Crucially, this new child process inherits the file descriptors of its parent (`main`). Because we just changed `main`'s standard input in the previous step, the `grep` process also inherits this modified standard input.
    *   Therefore, when the `grep` command starts, its standard input is already connected to the pipe that's receiving the output from `ls`.
    *   `grep` does its job: it reads from its standard input (the output of `ls`), filters for lines containing the letter "c", and writes the results to its own standard output.
    *   Since this `ft_popen` was also called with `type = 'r'`, its standard output is sent to a *new* pipe.
3.  **Result:** The `fd` variable is now overwritten. It no longer points to the `ls` pipe. Instead, it holds the file descriptor for the read end of the *second* pipe, which contains the final, filtered output from `grep`.



### Step 4: The `while` loop

1.  **Action:** The code enters a loop with `while ((line = get_next_line(fd)))`.
2.  **What Happens:** `get_next_line` reads, line by line, from the file descriptor `fd`. This `fd` is connected to the output of `grep`.
3.  **Result:** The `main` function reads the filtered output from the `grep` process and prints it to the screen using `printf`.

### Summary

In essence, you are using the file descriptors returned by `ft_popen` and the `dup2` system call to manually build a command pipeline.

*   The first `ft_popen` call captures the output of `ls`.
*   The `dup2` call sets up the `main` process so that its standard input *is* the output of `ls`.
*   The second `ft_popen` call starts `grep`, which inherits this setup, effectively making the output of `ls` the input for `grep`.
*   The final loop reads the result of the entire `ls | grep "c"` pipeline.
