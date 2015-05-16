/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Navy SEALs: Covert Operations source code.

Navy SEALs: Covert Operations source code is free software; you can
redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either version 2 of
the License, or (at your option) any later version.

Navy SEALs: Covert Operations source code is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Navy SEALs: Covert Operations source code; if not, write to the
Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
02110-1301  USA
===========================================================================
*/
//
//
// cg_mem.c - dynamic memory pool written for regexp
//

#include "cg_local.h"


#define POOLSIZE    ( 256 * 1024 )

static char cg_memoryPool[POOLSIZE];
static int cg_allocPoint;

vmCvar_t cg_debugAlloc;

void *CG_Alloc( int size ) {
	char    *p;

	if ( cg_debugAlloc.integer ) {
		CG_Printf( "CG_Alloc of 7%i bytes (%i left)\n", size, POOLSIZE - cg_allocPoint - ( ( size + 31 ) & ~31 ) );
	}

	if ( cg_allocPoint + size > POOLSIZE ) {
		CG_Error( "CG_Alloc: failed on allocation of %u bytes\n", size );
		return NULL;
	}

	p = &cg_memoryPool[cg_allocPoint];

	cg_allocPoint += ( size + 31 ) & ~31;

	return p;
}

void CG_InitMemory( void ) {
	cg_allocPoint = 0;
}

void CG_GameMem_f( void ) {
	CG_Printf( "CGame memory status: %i out of %i bytes allocated\n", cg_allocPoint, POOLSIZE );
}
