/* Copyright (c) 2017-2018, The Regents of the University of California (Regents).
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Regents nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 * SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 * OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
 * BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
 * HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdint.h>
#include <stdarg.h>
#include "syscall.h"
#include "string.h"
#include "tinyprintf.h"

static char buffer[512];

#define OCALL_PRINT_BUFFER 1

int debug_printf(char *fmt, ...) {
    unsigned long retval;
    va_list args;
    
    va_start(args, fmt);
    tfp_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    ocall(OCALL_PRINT_BUFFER , buffer, strlen(buffer)+1, &retval, sizeof(unsigned long));
    
    return (int)retval;
}

#endif  // _DEBUG_H_
