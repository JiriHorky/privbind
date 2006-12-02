/*
 * privbind - allow unpriviledged apps to bind to priviledged ports
 * Copyright (C) 2006 Shachar Shemesh
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef STUB_H
#define STUB_H

#include <dlfcn.h>

#define FUNCREDIR0( name, ret ) \
static ret stub_##name( void );\
static ret (* _##name)( void )=stub_##name;\
static ret stub_##name( void ) { _##name=dlsym( RTLD_NEXT, #name ); return _##name(); }

#define FUNCREDIR1( name, ret, type1 ) \
static ret stub_##name( type1 arg1 );\
static ret (* _##name)( type1 )=stub_##name;\
static ret stub_##name( type1 arg1 ) { _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1 ); }

#define FUNCREDIR2( name, ret, type1, type2 ) \
static ret stub_##name( type1 arg1, type2 arg2 );\
static ret (* _##name)( type1, type2 )=stub_##name;\
static ret stub_##name( type1 arg1, type2 arg2 ) { _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1, arg2 ); }

#define FUNCREDIR3( name, ret, type1, type2, type3 ) \
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3 );\
static ret (* _##name)( type1, type2, type3 )=stub_##name;\
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3 ) { _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1, arg2, arg3 ); }

#define FUNCREDIR4( name, ret, type1, type2, type3, type4 ) \
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4 );\
static ret (* _##name)( type1, type2, type3, type4 )=stub_##name;\
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4 )\
{ _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1, arg2, arg3, arg4 ); }

#define FUNCREDIR5( name, ret, type1, type2, type3, type4, type5 ) \
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5 );\
static ret (* _##name)( type1, type2, type3, type4, type5 )=stub_##name;\
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5 )\
{ _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1, arg2, arg3, arg4, arg5 ); }

#define FUNCREDIR6( name, ret, type1, type2, type3, type4, type5, type6 ) \
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6 );\
static ret (* _##name)( type1, type2, type3, type4, type5, type6 )=stub_##name;\
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6 )\
{ _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1, arg2, arg3, arg4, arg5, arg6 ); }

#define FUNCREDIR9( name, ret, type1, type2, type3, type4, type5, type6, type7, type8, type9 ) \
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8,\
	type9 arg9 );\
static ret (* _##name)( type1, type2, type3, type4, type5, type6, type7, type8, type9 )=stub_##name;\
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8,\
	type9 arg9 )\
{ _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ); }

#define FUNCREDIR10( name, ret, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10) \
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8,\
	type9 arg9, type10 arg10);\
static ret (* _##name)( type1, type2, type3, type4, type5, type6, type7, type8, type9, type10)=stub_##name;\
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8,\
	type9 arg9, type10 arg10)\
{ _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); }

#define FUNCREDIR12( name, ret, type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12 ) \
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8,\
	type9 arg9, type10 arg10, type11 arg11, type12 arg12 );\
static ret (* _##name)( type1, type2, type3, type4, type5, type6, type7, type8, type9, type10, type11, type12 )=stub_##name;\
static ret stub_##name( type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8,\
	type9 arg9, type10 arg10, type11 arg11, type12 arg12 )\
{ _##name=dlsym( RTLD_NEXT, #name ); return _##name( arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12 ); }

#endif // STUB_H
