/* Header file for libgcc2.c.  */
/* Copyright (C) 2000-2020 Free Software Foundation, Inc.

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

#ifndef GCC_LIBGCC2_H
#define GCC_LIBGCC2_H

/* In the first part of this file, we are interfacing to calls generated
   by the compiler itself.  These calls pass values into these routines
   which have very specific modes (rather than very specific types), and
   these compiler-generated calls also expect any return values to have
   very specific modes (rather than very specific types).  Thus, we need
   to avoid using regular C language type names in this part of the file
   because the sizes for those types can be configured to be anything.
   Instead we use the following special type names.  */

typedef 	     int SItype	    __attribute__ ((mode (SI)));
typedef unsigned int USItype	__attribute__ ((mode (SI)));
typedef		     int DItype  	__attribute__ ((mode (DI)));
typedef unsigned int UDItype	__attribute__ ((mode (DI)));
typedef 	   float SFtype 	__attribute__ ((mode (SF)));
typedef		   float DFtype	    __attribute__ ((mode (DF)));
#define W_TYPE_SIZE    (4 * __CHAR_BIT__)
#define Wtype	        SItype
#define UWtype	        USItype
#define DWtype	        DItype
#define UDWtype	        UDItype
#define Wtype_MAXp1_F	0x1p32f

extern UDWtype __udivmoddi4 (UDWtype, UDWtype, UDWtype *);

#endif /* ! GCC_LIBGCC2_H */
