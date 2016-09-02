/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */


#ifndef _SYSTEM_DOMAIN_H_
#define _SYSTEM_DOMAIN_H_


#include <sys/appleapiopts.h>

/* Kernel Events Protocol */ 
#define SYSPROTO_EVENT 		1	/* kernel events protocol */

/* Kernel Control Protocol */
#define SYSPROTO_CONTROL       	2	/* kernel control protocol */
#define AF_SYS_CONTROL		2	/* corresponding sub address type */

/* System family socket address */
struct sockaddr_sys
{
    u_char	ss_len;		/* sizeof(struct sockaddr_sys) */
    u_char	ss_family;	/* AF_SYSTEM */
    u_int16_t 	ss_sysaddr; 	/* protocol address in AF_SYSTEM */
    u_int32_t 	ss_reserved[7]; /* reserved to the protocol use */
};


#ifdef KERNEL
#ifdef __APPLE_API_PRIVATE

extern struct domain systemdomain;

/* built in system domain protocols init function */
int kern_event_init();
int kern_control_init();

#endif /* __APPLE_API_PRIVATE */
#endif /* KERNEL */

#endif /* _SYSTEM_DOMAIN_H_ */


