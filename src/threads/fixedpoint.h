#ifndef _FIXED_POINT_H_
#define _FIXED_POINT_H_

typedef int fix_p;

#define DIFF 16384

fix_p int_to_fix_p (int);

int fix_p_to_int (fix_p);

int fix_p_to_int_round (fix_p);

fix_p add_two_fix_p (fix_p x, fix_p y);

fix_p subtract_two_fix_p (fix_p x, fix_p y);

fix_p add_fix_p_int (fix_p, int);

fix_p subtract_fix_p_int (fix_p, int);

fix_p multiple_two_fix_p (fix_p, fix_p);

fix_p multiple_fix_p_int (fix_p, int);

fix_p divide_two_fix_p (fix_p, fix_p);

fix_p divide_fix_p_int (fix_p, int);

#endif