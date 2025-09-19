/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vbc.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/19 13:09:26 by anemet            #+#    #+#             */
/*   Updated: 2025/09/19 17:23:44 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// defining the struct of a node in our AST (Abstract Syntax Tree)
typedef struct node
{
	// enum representing the type of the node
	// it can be addition/multiplication/numerical value
	enum
	{
		ADD,	// represents the '+' operation
		MULTI,	// represents the '*' operation
		VAL,	// represents a single digit value (0..9)
	}	type;
	int val;		// if the node type is VAL, this will hold the number
	struct node *l;	// pointer to the left child node (for type ADD and MULTI)
	struct node *r;	// pointer to the right child node (for type ADD and MULTI)
}	node;

/* new_node()
	A helper function to create a new node on the heap
	- it takes a 'node' struct as parameter
	- allocates memory for it
	- copies the contents of the parameter `n` (data on stack) to `*ret` (on heap)
	Returns the pointer `ret` to the newly allocated node on the heap
*/
node *new_node(node n)
{
	// allocate memory for one 'node' on heap (initialized to zero)
	node *ret = calloc(1, sizeof(n));
	// subject requirement:
	// In case of a syscall failure just exit with the code 1
	// (this should free up all allocated memory to the process)
	if (!ret)
		exit(1);
	// Copy the data from the provided node 'n' (on stack) to new node (on heap)
	*ret = n;
	return ret;
}

/* destroy_tree()
	Recursively frees the memory allocated for the AST
*/
void	destroy_tree(node *n)
{
	// base case: if the node is NULL, there's nothing to free, just return
	if (!n)
		return;
	// if the node is an operation (not a value), we free its children first
	if (n->type != VAL)
	{
		destroy_tree(n->l);
		destroy_tree(n->r);
	}
	// Finally free the node itself
	free(n);
}

/* unexpected()
	Prints an error message for an unexpected token
*/
void unexpected(char c)
{
	if (c)
		printf("Unexpected token '%c'\n", c);
	else
		// if 'c' is the NULL terminator => we reached the end unexpectedly
		printf("Unexpected end of input");
}

// Forward declarations for the parsing functions.
// This is necessary because the functions call each other recursively
node *parse_expr(char **s);
node *parse_term(char **s);
node *parse_factor(char **s);

/* parse_factor()
	This functions parses a "factor"
	In our grammar a factor is either a single number of an expression in parentheses.
	This is the highest level of precedence
*/
node *parse_factor(char **s)
{
	// If the current character is a digit
	if (isdigit(**s))
	{
		// ...create a new node of type VAL.
		// The value is the character converted to an integer
		// We then advance the string pointer to the next character
		return new_node((node){VAL, *(*s)++ - '0', NULL, NULL});
	}
	// If the current character is an opening parenthesis...
	if (**s == '(')
	{
		// ...consume the '('
		(*s)++;
		// ...and recursively parse the expression inside the parentheses.
		node *ret = parse_expr(s);
	}
}

/*            Let's examine *(*s)++

Step 1: Parsing with Precedence Rules
	- The compiler sees two operators trying to act on the operand (*s):
		the prefix * and the postfix ++.
	- The precedence table says: postfix ++ > prefix *.
	- Because of this, the compiler groups the expression as written with parentheses:
		*( (*s)++ )
	- This grouping means:
	"The dereference operator * will be applied to the result of the (*s)++ operation."

Step 2: Runtime Evaluation
	- Now the program has to execute *( (*s)++ ).
		To do that, it must first figure out the result of the inner part, (*s)++.
	- The postfix ++ operator evaluates to the variable's original value,
		and the increment happens as a side effect after the original value has been taken.

Conclusion
	- Your observation was that the dereference effectively happens before the increment.
		You are right.
	- Your conclusion was that this must mean * has higher precedence.
		This is the subtle error.
	- The reality is:
		++ has higher precedence, so it binds to (*s).
		The * is applied to the result of the ++ expression.
		But the special evaluation rule for postfix ++ is
		that its result is the value before it increments, which creates the behavior you saw.
*/

/* parse_term()
	This function parses a "term"
	In our grammar a term is a sequence of factors mltiplied togheter
	This handles the '*' operation.
*/
node *parse_term(char **s)
{
	// A term must start with a factor
	node *ret = parse_factor(s);
	if (!ret)
		return NULL;

	// After the first factor, we loop as long as we see multiplication operators.
	while (**s == '*')
	{
		// Consume the '*'
		(*s)++;
		// The right-hand side of the multiplication must be another factor
		node *right = parse_factor(s);
		if (!right)
		{
			destroy_tree(ret);  // Clean up on error
			return NULL;
		}
		// Create a new MULTI node. The current 'ret' becomes the left child,
		// and the newly parsed 'right' becomes the right child.
		ret = new_node((node){MULTI, 0, ret, right});
	}
	return ret;
}

/* parse_expr()
	This function parses and "expression".
	In our grammar the expression is a sequence of terms added together.
	This handles the '+' operation. This is the lowest level of precedence.
*/
node *parse_expr(char **s)
{
	// An expression must start with a term.
	node *ret = parse_term(s);
	if (!ret)
		return NULL;

	// After the first term, we loop as long as we see addition operators
	while (**s == '+')
	{
		// Consume the '+'
		(*s)++;
		// The right-hand side of the addition must be another term
		node *right = parse_term(s);
		if (!right)
		{
			destroy_tree(ret);
			return NULL;
		}
		// Create a new ADD node. The current 'ret' becomes the left child,
		// and the newly parsed 'right' becomes the right child.
		ret = new_node((node){ADD, 0, ret, right});
	}
	return ret;
}

/* parse_main()
	This is the main parsing function that kicks off the process
*/
node *parse_main(char *s)
{
	// Start by parsing the entire expression
	node *ret = parse_expr(&s);
	if (!ret)
		return NULL;

	// After parsing, we should be at the end of the string.
	// If there are any characters left, it's an error
	if (*s)
	{
		unexpected(*s);
		destroy_tree(ret);
		return NULL;
	}
	return ret;
}

// This function evaluates the result of the expression represented by the AST
// It does this by recursively traversing the tree
int eval_tree(node *tree)
{
	switch (tree->type)
	{
		// if the node is ADD, evaluate left and right and add the results
		case ADD:
			return (eval_tree(tree->l) + eval_tree(tree->r));
		// if node is MULTI, evaluate left and right and multiply the results
		case MULTI:
			return (eval_tree(tree->l) * eval_tree(tree->r));
		// if node is VAL, simply return the value. This is the base case for recursion.
		case VAL:
			return (tree->val);
	}
	return 0; // ideally this shouldn't be reached
}

// The main() function
int main(int argc, char **argv)
{
	// The program expects exactly one argument: the mathematical expression
	if (argc != 2)
		return 1;
	// Call the main parsing function to build the AST from the command line argument
	node *tree = parse_main(argv[1]);
	// If parsing fails, parse_main will return NULL and have already printed an error
	if (!tree)
		return 1;
	// If parsing is successful, evaluate the tree and print the result
	printf("%d\n", eval_tree(tree));
	// Free AST tree
	destroy_tree(tree);
	return 0;
}
