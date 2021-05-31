/**********************************************************************
 * $Workfile: etypes.h $
 * $Revision: 1.2 $
 *  $Modtime: $
 *
 * PURPOSE:
 * Base types that are used in machine independent APIs.
 * The types declared here are intended to describe variables of known sizes
 * so that there will be no ambiguity in setting up the stack for a function
 * call.
 * 
 * ASSUMPTIONS:
 * All pointers are FAR in Windows.  Please include the FAR macro for every
 * pointer reference.
 *
 * NOTE:
 * Sizes of types:
 * Types that have ambigious sizes are left out of this file.  You will not
 * find int nor float.  Types that may not be the same size on all platforms
 * also tend to drop out of this file (eg. short.)
 * whar_t presents a problem.  wchar_t is has the same size and alignment
 * as int, therefor it's size may vary from compiler to compiler!
 * We will represent pointers to wchar_t as pointers to uint8 
 *
 * Floats as return values from functions:
 * Microsoft and Borland do not seem to return floats and doubles
 * in the same way from functions.  As a result of this, please do
 * not construct a function as part of a public DLL API that returns
 * a float or double.  If the calling application was not made with a 
 * compiler that has the same assumptions about floating point returns,
 * a difficult to find error may develop. - becd.
 * 
 * COPYRIGHT:
 * Copyright (c) 1995, 1996, 1997 Barrett Davis, All rights reserved.
 * This source code and the binaries that result may be freely distributed,
 * used and modified as long as the above copyright notice remains intact.
 * 
 * WARRANTY:
 * The author of etypes.h (hereafter referred to as "the author") makes no
 * warranty of any kind, expressed or implied, including without limitation, 
 * any warranties of merchantability and/or fitness for a particular purpose.
 * The author shall not be liable for any damages, whether direct, indirect,
 * special, or consequential arising from a failure of this program to 
 * operate in the manner desired by the user.  The author shall not be liable
 * for any damage to data or property which may be caused directly or 
 * indirectly by use of the program.
 *
 * In no event will the author be liable to the user for any damages,
 * including any lost profits, lost savings, or other incidental or 
 * consequential damages arising out of the use or inability to use the 
 * program, or for any claim by any other party.
 *
 ************************************************************************/
#ifndef ETYPES_H
#define ETYPES_H

// ****************************************************************************
// The following will be the only types used in exported functions:
// uint8   FAR*
//  int32  
//  int32  FAR*
// uint32
// uint32  FAR*
// float64           Note that float64 will *not* be used as a return value.
// float64 FAR*   
// void    FAR*
// ****************************************************************************

typedef unsigned char   uint8;
typedef signed   long   int32;
typedef unsigned long  uint32;
typedef double        float64;

// Generic void function pointer.
typedef void (* VoidFunction)(void);

// Common derived types that can be used in exported functions:
typedef int32      bool32;   // 0 = FALSE, non zero is TRUE (typically 1)

// size_t is ususually defined in the i/o header files...
#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#endif // ETYPES_H

