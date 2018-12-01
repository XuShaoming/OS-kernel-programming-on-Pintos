#include "threads/fixedpoint.h"
#include <stdint.h>

/* 
  Convert an interger to a fixed point value
*/
fix_p
int_to_fix_p (int n)
{
	return n * DIFF;
}

/*
  Convert a fixed point to an integer 
  (rounding toward zero)
*/
int 
fix_p_to_int (fix_p x) 
{
	return x / DIFF;
}

/*
  Convert a fixed point to an integer
  (rounding to nearest)
*/
int 
fix_p_to_int_round (fix_p x)
{
	if (x >= 0) {
		return (x + DIFF / 2) / DIFF;
	} else {
		return (x - DIFF / 2) / DIFF;
	}
}

/* Add two fixed point */
fix_p 
add_two_fix_p (fix_p x, fix_p y)
{
	return x + y;
}

/* Subtract y from x */
fix_p 
subtract_two_fix_p (fix_p x, fix_p y) 
{
	return x - y;
}

/* Add an integer to a fixed point */
fix_p
add_fix_p_int (fix_p x, int n)
{
	return x + n * DIFF;
}

/* Subtract a int from fixed point */
fix_p
subtract_fix_p_int (fix_p x, int n)
{
	return x - n * DIFF;
}

/* Multiple a fixed point to a fixed point */
fix_p
multiple_two_fix_p (fix_p x, fix_p y)
{
	return ((int64_t)x) * y / DIFF;
}

/* Multiple a int to a fixed point */
fix_p
multiple_fix_p_int (fix_p x, int n)
{
	return x * n;
}

/* Divide a fixed point by a fixed point */
fix_p
divide_two_fix_p (fix_p x, fix_p y)
{
	return ((int64_t)x) * DIFF / y;
}

/* Divide a fixed point by an integer */
fix_p
divide_fix_p_int (fix_p x, int n)
{
	return x / n;
}
