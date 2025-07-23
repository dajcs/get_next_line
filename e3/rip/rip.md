Write a C program called `rip`, that will take as argument a string containing only parenthesis.\
if parenthesis are unbalanced (for example `())`) the program shall remove the minimum number of parenthesis for the expression to be balanced.\
By removing we mean replacing by spaces.\
All the solutions should be printed (can be more than one).\
The order of the solutions is not important.

For example this should work:
(For readability reasons the `_` means space and the spaces are for readability only.)
```bash
$> ./rip '( ( )' | cat -e
_ ( ) $
( _ ) $
$> ./rip '( ( ( ) ( ) ( ) ) ( ) )' | cat -e
( ( ( ) ( ) ( ) ) ( ) ) $
$> ./rip '( ) ( ) ) ( )' | cat -e
( ) ( ) _ ( ) $
( ) ( _ ) ( ) $
( _ ( ) ) ( ) $
$> ./rip '( ( ) ( ( ) (' | cat -e
( ( ) _ _ ) _ $
( _ ) ( _ ) _ $
( _ ) _ ( ) _ $
_ ( ) ( _ ) _ $
_ ( ) _ ( ) _ $
```

- allowed external functions : `puts`

