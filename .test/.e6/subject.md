
# mini_serv

- Assignment name  : mini_serv
- Expected files   : `mini_serv.c`
- Allowed functions: `write`, `close`, `select`, `socket`, `accept`, `listen`, `send`, `recv`, `bind`, `strstr`, `malloc`, `realloc`, `free`, `calloc`, `bzero`, `atoi`, `sprintf`, `strlen`, `exit`, `strcpy`, `strcat`, `memset`
--------------------------------------------------------------------------------

Write a program that will listen for client to connect on a certain port on `127.0.0.1` and will let clients to speak with each other

This program will take as first argument the port to bind to
If no argument is given, it should write in stderr **"Wrong number of arguments"** followed by a `\n` and exit with status `1`

If a System Call returns an error before the program start accepting connection, it should write in stderr **"Fatal error"** followed by a `\n` and exit with status `1`
If can't allocate memory it should write in stderr **"Fatal error"** followed by a `\n` and exit with status `1`

- The program must be non-blocking but client can be lazy and if they don't read the message we must NOT disconnect them...
- The program must not contain #define preproc
- The program must only listen to `127.0.0.1`
- The received `fd` will already be set to make `recv` or `send` to block if `select` hasn't be called before calling them, but will not block otherwise. 

When a client connects to the server:
- the client will be given an `id`. The first client will receive the `id` `0` and each new client will received the last client `id + 1`
- `%d` will be replaced by this number in the messages below
- a message is sent to all the client that was connected to the server: **"server: client %d just arrived\n"**

Clients must be able to send messages to the program.
- message will only be printable characters, no need to check
- a single message can contains multiple \n
- when the server receives a message, it must resend it to all the other clients with **"client %d: "** before every line!

When a client disconnect from the server:
- a message is sent to all the client that was connected to the server: **"server: client %d just left\n"**

Memory or fd leaks are forbidden

There is a boilerplate in the file `template.c` with the beginning of a server and some useful functions.
(Beware this file might use forbidden functions or contain things that must not be there in the final program)

**Warning**: our tester is expecting that messages are sent in a fast pace. Don't do un-necessary buffer.


Evaluation can be a bit longer than usual...

**Hint**: you can use `nc` to test your program
**Hint**: you should use `nc` to test your program
**Hint**: To test you can use `fcntl(fd, F_SETFL, O_NONBLOCK)` but use `select` and **NEVER** check `EAGAIN` (`man 2 send`)
