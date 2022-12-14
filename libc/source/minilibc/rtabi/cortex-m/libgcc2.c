/* More subroutines needed by GCC output code on some machines.  */
/* Compile this one with gcc.  */
/* Copyright (C) 1989-2020 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#include "libgcc2.h"

UDWtype __fixunsdfdi (DFtype a)
{
    UDWtype result;
    /* Get high part of result.  The division here will just moves the radix
       point and will not cause any rounding.  Then the conversion to integral
       type chops result as desired.  */
    const UWtype hi = a / Wtype_MAXp1_F;

    /* Get low part of result.  Convert `hi' to floating type and scale it back,
       then subtract this from the number being converted.  This leaves the low
       part.  Convert that to integral type.  */
    const UWtype lo = a - (DFtype) hi * Wtype_MAXp1_F;
    /* Assemble result from the two parts.  */
    result = ((UDWtype) hi << W_TYPE_SIZE) | lo;
    return result;
}

DWtype __fixdfdi (DFtype a)
{
    if (a < 0)
        return - __fixunsdfdi (-a);
    return __fixunsdfdi (a);
}

UDWtype __fixunssfdi (SFtype a)
{
    /* Convert the SFtype to a DFtype, because that is surely not going
       to lose any bits.  Some day someone else can write a faster version
       that avoids converting to DFtype, and verify it really works right.  */
    const DFtype dfa = a;
    
    return __fixunsdfdi (dfa);
}


DWtype __fixsfdi (SFtype a)
{
    /* Convert the SFtype to a DFtype, because that is surely not going
       to lose any bits.  Some day someone else can write a faster version
       that avoids converting to DFtype, and verify it really works right.  */
    const DFtype dfa = a;

    return __fixdfdi (dfa);
}

UDWtype __udivmoddi4 (UDWtype n, UDWtype d, UDWtype *rp)
{
    UDWtype q = 0, r = n, y = d;
    UWtype lz1, lz2, i, k;

    /* Implements align divisor shift dividend method. This algorithm
       aligns the divisor under the dividend and then perform number of
       test-subtract iterations which shift the dividend left. Number of
       iterations is k + 1 where k is the number of bit positions the
       divisor must be shifted left  to align it under the dividend.
       quotient bits can be saved in the rightmost positions of the dividend
       as it shifts left on each test-subtract iteration. */

    if (y <= r)
    {
        lz1 = __builtin_clzll (d);
        lz2 = __builtin_clzll (n);

        k = lz1 - lz2;
        y = (y << k);

        /* Dividend can exceed 2 ^ (width ??? 1) ??? 1 but still be less than the
           aligned divisor. Normal iteration can drops the high order bit
           of the dividend. Therefore, first test-subtract iteration is a
           special case, saving its quotient bit in a separate location and
           not shifting the dividend. */
        if (r >= y)
        {
            r = r - y;
            q =  (1ULL << k);
        }

        if (k > 0)
        {
            y = y >> 1;

            /* k additional iterations where k regular test subtract shift
               dividend iterations are done.  */
            i = k;
            do
            {
                if (r >= y)
                    r = ((r - y) << 1) + 1;
                else
                    r =  (r << 1);
                
                i = i - 1;
            } while (i != 0);

            /* First quotient bit is combined with the quotient bits resulting
               from the k regular iterations.  */
            q = q + r;
            r = r >> k;
            q = q - (r << k);
        }
    }

    if (rp)
        *rp = r;
    return q;
}

