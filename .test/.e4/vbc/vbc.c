/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vbc.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 09:20:48 by anemet            #+#    #+#             */
/*   Updated: 2025/09/23 10:56:18 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*

Assignment name  : vbc
Expected files   : *.c *.h
Allowed functions: malloc, calloc, realloc, free, printf, isdigit, write
--------------------------------------------------------------------------------

Write a program that will print the result of a math expression given as
argument.
You must handle the operations + * and the parenthesis.
You don't have to handle whitespaces in the expression.
All the values in the expression will be between 0 and 9 included.
In case of unexpected symbol or inappropriate parenthesis, you will
print "Unexpected token '%c'\n" and exit with the code 1 (if the
symbol is the end of input you will print: "Unexpected end of input\n").
In case of a syscall failure you will just exit with the code 1.

You will find in this directory the beginning of the code you need to write.

For example this should work:
$> ./vbc '1' | cat -e
1$
$> ./vbc '2+3' | cat -e
5$
$> ./vbc '3*4+5' | cat -e
17$
$> ./vbc '3+4*5' | cat -e
23$
$> ./vbc '(3+4)*5' | cat -e
35$
$> ./vbc '(((((2+2)*2+2)*2+2)*2+2)*2+2)*2' | cat -e
188$
$> ./vbc '1+' | cat -e
Unexpected end of input$
$> ./vbc '1+2)' | cat -e
Unexpected token ')'$
$> ./vbc '1+2+3+4+5' | cat -e
15$
$> ./vbc '2*4+9+3+2*1+5+1+6+6*1*1+8*0+0+5+0*4*9*5*8+9*7+5*1+3+1+4*5*7*3+0*3+4*8+8+8+4*0*5*3+5+4+5*7+9+6*6+7+9*2*6*9+2+1*3*7*1*1*5+1+2+7+4+3*4*2+0+4*4*2*2+6+7*5+9+0+8*4+6*7+5+4*4+2+5*5+1+6+3*5*9*9+7*4*3+7+4*9+3+0+1*8+1+2*9*4*5*1+0*1*9+5*3*5+9*6+5*4+5+5*8*6*4*9*2+0+0+1*5*3+6*8*0+0+2*3+7*5*6+8+6*6+9+3+7+0*0+5+2*8+2*7*2+3+9*1*4*8*7*9+2*0+1*6*4*2+8*8*3*1+8+2*4+8*3+8*3+9*5+2*3+9*5*6*4+3*6*6+7+4*8+0+2+9*8*0*6*8*1*2*7+0*5+6*5+0*2+7+2+3+8*7+6+1*3+5+4*5*4*6*1+4*7+9*0+4+9*8+7+5+6+2+6+1+1+1*6*0*9+7+6*2+4*4+1*6*2*9+3+0+0*1*8+4+6*2+6+2*7+7+0*9+6+2*1+6*5*2*3*5*2*6*4+2*9*2*4*5*2*2*3+8+8*3*2*3+0*5+9*6+8+3*1+6*9+8+9*2*0+2' | cat -e
94305$
$> ./vbc '(1)' | cat -e
1$
$> ./vbc '(((((((3)))))))' | cat -e
3$
$> ./vbc '(1+2)*3' | cat -e
9$
$> ./vbc '((6*6+7+5+8)*(1+0+4*8+7)+2)+4*(1+2)' | cat -e
2254$
$> ./vbc '((1+3)*12+(3*(2+6))' | cat -e
Unexpected token '2'$

*/


// GIVEN CODE


/* ********** vbc.h **********

#ifndef VBC_H
# define VBC_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct node {
    enum {
        ADD,
        MULTI,
        VAL
    }   type;
    int val;
    struct node *l;
    struct node *r;
}   node;

// Function prototypes
node    *new_node(node n);
void    destroy_tree(node *n);
void    unexpected(char c);
int     accept(char **s, char c);
int     expect(char **s, char c);
node    *parse_expr(char *s);
node    *parse_expression(char **s);
node    *parse_term(char **s);
node    *parse_factor(char **s);
int     eval_tree(node *tree);

#endif
*/

#include "vbc.h"

// reserve node n in heap
// copy n from stack to heap
node    *new_node(node n)
{
    node *ret = calloc(1, sizeof(n));
    if (!ret)
        return (NULL);
    *ret = n;
    return (ret);
}

// destroy tree recursively
void    destroy_tree(node *n)
{
    if (!n)          // base case
        return ;
    if (n->type != VAL)
    {
        destroy_tree(n->l);
        destroy_tree(n->r);
    }
    free(n);
}

// print unexpected token/EOF
void    unexpected(char c)
{
    if (c)
        printf("Unexpected token '%c'\n", c);
    else
        printf("Unexpected end of input\n");
}

// if c is next char, consume it and return 1
// otherwise return 0
int accept(char **s, char c)
{
    if (**s == c)
    {
        (*s)++;
        return (1);
    }
    return (0);
}

// if c is next char, consume it and return 1
// otherwise print unexpected c/EOF and return 0
int expect(char **s, char c)
{
    if (accept(s, c))
        return (1);
    unexpected(**s);
    return (0);
}

// forward definition of parse_expression()
// node *parse_expression(char **s);

/* parse_factor()
	- expression is a sum of terms
	- term is a multiplication of factors
	> FACTOR is an int value or an expression in ()
*/
node *parse_factor(char **s)
{
	// check for INT
	if (isdigit(**s))
	{
		// add new VAL node
		node *ret = new_node((node){VAL, **s - '0', NULL, NULL});
		if (!ret)
			return NULL;
		(*s)++;  // consume digit
		return ret;
	}
	// check for '('
	if (accept(s, '('))
	{
		// evaluate expression
		node *ret = parse_expression(s);
		if (!ret)
			return NULL;
		// check for closing ')'
		if (!expect(s, ')'))
			return NULL;
		return ret;
	}
	// if neither digit, neither '(' then unexpected token
	unexpected(**s);
	return NULL;
}


/* parse_term()
	- expression is a sum of terms
	> TERM is a multiplication of factors
	- factor is an int value or an expression in ()
*/
node *parse_term(char **s)
{
	// starts with a factor
	node *left = parse_factor(s);
	if (!left)
		return NULL;

	// checking for more factors on the right
	while (accept(s, '*'))  // if next c is '*' consume it and parse right
	{
		node *right = parse_factor(s);
		if (!right)
		{
			destroy_tree(left);
			return NULL;
		}
		// create new MULTI node as left factor
		left = new_node((node){MULTI, 0, left, right});
		if (!left)
		{
			destroy_tree(right);
			return NULL;
		}
	}
	return left;
}

/* parse_expression()
	> EXPRESSION is a sum of terms
	- terms is a multiplication of factors
	- factor is an int value or an expression in ()
*/
node *parse_expression(char **s)
{
	// starts with a term
	node *left = parse_term(s);
	if (!left)
		return NULL;

	// checking for more terms on the right
	while (accept(s, '+'))      // if next c is '+' consume it and parse right
	{
		node *right = parse_term(s);
		if (!right)
		{
			destroy_tree(left);
			return NULL;
		}
		// create new ADD node as a left term
		left = new_node((node){ADD, 0, left, right});
		if (!left)
		{
			destroy_tree(right);
			return NULL;
		}
	}
	return left;
}


node    *parse_expr(char *s)
{
    node *ret = parse_expression(&s);
	if (!ret)
		return NULL;

// if there are characters left in s after parsing -> gempa
    if (*s)
    {
		unexpected(*s);
        destroy_tree(ret);
        return (NULL);
    }
    return (ret);
}

int eval_tree(node *tree)
{
    switch (tree->type)
    {
        case ADD:
            return (eval_tree(tree->l) + eval_tree(tree->r));
        case MULTI:
            return (eval_tree(tree->l) * eval_tree(tree->r));
        case VAL:
            return (tree->val);
    }
	return 0; // should not be reached
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return (1);
    node *tree = parse_expr(argv[1]);
    if (!tree)
        return (1);
    printf("%d\n", eval_tree(tree));
    destroy_tree(tree);
}
