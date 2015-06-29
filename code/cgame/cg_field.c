/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2013-2015 Zack Middleton

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
#include "cg_local.h"
#include "../ui/keycodes.h"
#ifdef UNICODE_MFIELD
#include "../qcommon/q_unicode.h"
#endif

/*
===================
MField_Draw

Handles horizontal scrolling and cursor blinking
x, y, charWidth, charHeight, are in 640*480 virtual screen size
===================
*/
void MField_Draw( mfield_t *edit, int x, int y, int style, qboolean drawCursor ) {
#ifdef UNICODE_MFIELD
	int		i;
#endif
	int		len;
	int		drawLen;
	int		prestep;
	int		cursorChar;
	char	str[MAX_STRING_CHARS];

	drawLen = edit->widthInChars;
	len     = edit->len + 1;

	// guarantee that cursor will be visible
	if ( len <= drawLen ) {
		prestep = 0;
	} else {
		if ( edit->scroll + drawLen > len ) {
			edit->scroll = len - drawLen;
			if ( edit->scroll < 0 ) {
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if ( prestep + drawLen > len ) {
		drawLen = len - prestep;
	}

	// extract <drawLen> characters from the field at <prestep>
	if ( drawLen >= MAX_STRING_CHARS ) {
		trap_Error( "drawLen >= MAX_STRING_CHARS" );
	}

	str[0] = 0;
#ifdef UNICODE_MFIELD
	for ( i = 0; i < drawLen; i++ ) {
		Q_strcat( str, sizeof( str ), Q_UTF8_Encode( edit->buffer[prestep+i] ) );
	}
#else
	memcpy( str, edit->buffer + prestep, drawLen );
	str[ drawLen ] = 0;
#endif

	if ( drawCursor ) {
		if ( trap_Key_GetOverstrikeMode() ) {
			cursorChar = 11; // full block
		} else {
			cursorChar = 10; // full width low line
		}
	} else {
		cursorChar = -1;
	}

	CG_DrawBigString( x, y, str, 1.0f );

	if ( cursorChar != -1 && !((cg.time/BLINK_DIVISOR) & 1) ) {
		str[0] = cursorChar;
		str[1] = 0;
		CG_DrawBigString( x + ( edit->cursor - prestep ) * BIGCHAR_WIDTH, y, str, 1.0f );
	}
}

/*
================
MField_Buffer

Returns a UTF-8 encoded string.
================
*/
const char *MField_Buffer( mfield_t *edit ) {
	static char	str[MAX_STRING_CHARS];
#ifdef UNICODE_MFIELD
	int i;

	str[0] = 0;
	for ( i = 0; i < edit->len; i++ ) {
		Q_strcat( str, sizeof( str ), Q_UTF8_Encode( edit->buffer[i] ) );
	}
#else
	Q_strncpyz( str, edit->buffer, sizeof ( str ) );
#endif

	return str;
}

/*
================
MField_AddText
================
*/
void MField_AddText( mfield_t *edit, const char *text ) {
	int codePoint;

	while ( *text ) {
#ifdef UNICODE_MFIELD
		codePoint = Q_UTF8_CodePoint( &text );
#else
		codePoint = *text;
		text++;
#endif

		if ( codePoint != 0 ) {
			MField_CharEvent( edit, codePoint );
		}
	}
}

/*
================
MField_SetText
================
*/
void MField_SetText( mfield_t *edit, const char *text ) {
	MField_Clear( edit );
	MField_AddText( edit, text );
}

/*
================
MField_Paste
================
*/
void MField_Paste( mfield_t *edit ) {
#if 0 // The system call is not available in ET's CGame
	char	pasteBuffer[MAX_EDIT_LINE];

	trap_GetClipboardData( pasteBuffer, MAX_EDIT_LINE );

	MField_AddText( edit, pasteBuffer );
#endif
}

/*
=================
MField_KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void MField_KeyDownEvent( mfield_t *edit, int key ) {
	// shift-insert is paste
	if ( ( ( key == K_INS ) || ( key == K_KP_INS ) ) && trap_Key_IsDown( K_SHIFT ) ) {
		MField_Paste( edit );
		return;
	}

	if ( key == K_DEL || key == K_KP_DEL ) {
		int i;

		if ( edit->cursor < edit->len ) {
			for ( i = edit->cursor; i < edit->len; i++ ) {
				edit->buffer[i] = edit->buffer[i+1];
			}
			edit->len--;
		}
		return;
	}

	if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) 
	{
		if ( edit->cursor < edit->len ) {
			edit->cursor++;
		}
		if ( edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= edit->len )
		{
			edit->scroll++;
		}
		return;
	}

	if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) 
	{
		if ( edit->cursor > 0 ) {
			edit->cursor--;
		}
		if ( edit->cursor < edit->scroll )
		{
			edit->scroll--;
		}
		return;
	}

	if ( key == K_HOME || key == K_KP_HOME || ( tolower(key) == 'a' && trap_Key_IsDown( K_CTRL ) ) ) {
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if ( key == K_END || key == K_KP_END || ( tolower(key) == 'e' && trap_Key_IsDown( K_CTRL ) ) ) {
		edit->cursor = edit->len;
		edit->scroll = edit->len - edit->widthInChars + 1;
		if (edit->scroll < 0)
			edit->scroll = 0;
		return;
	}

	if ( key == K_INS || key == K_KP_INS ) {
		trap_Key_SetOverstrikeMode( !trap_Key_GetOverstrikeMode() );
		return;
	}
}

/*
==================
MField_CharEvent
==================
*/
void MField_CharEvent( mfield_t *edit, int ch ) {
	int i;

	if ( ch == 'v' - 'a' + 1 ) {	// ctrl-v is paste
		MField_Paste( edit );
		return;
	}

	if ( ch == 'c' - 'a' + 1 ) {	// ctrl-c clears the field
		MField_Clear( edit );
		return;
	}

	if ( ch == 'h' - 'a' + 1 )	{	// ctrl-h is backspace
		if ( edit->cursor > 0 ) {
			for ( i = edit->cursor; i < edit->len+1; i++ ) {
				edit->buffer[i-1] = edit->buffer[i];
			}

			edit->cursor--;
			if ( edit->cursor < edit->scroll )
			{
				edit->scroll--;
			}
			edit->len--;
		}
		return;
	}

	if ( ch == 'a' - 'a' + 1 ) {	// ctrl-a is home
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if ( ch == 'e' - 'a' + 1 ) {	// ctrl-e is end
		edit->cursor = edit->len;
		edit->scroll = edit->cursor - edit->widthInChars + 1;
		if (edit->scroll < 0)
			edit->scroll = 0;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if ( ch < 32 ) {
		return;
	}

	if ( trap_Key_GetOverstrikeMode() ) {
		if ((edit->cursor == MAX_EDIT_LINE - 1) || (edit->maxchars && edit->cursor >= edit->maxchars))
			return;
	} else {
		// insert mode
		if (( edit->len == MAX_EDIT_LINE - 1 ) || (edit->maxchars && edit->len >= edit->maxchars))
			return;

		for ( i = edit->len + 1; i >= edit->cursor; i-- ) {
			edit->buffer[i+1] = edit->buffer[i];
		}

		edit->len++;
	}

	edit->buffer[edit->cursor] = ch;
	if (!edit->maxchars || edit->cursor < edit->maxchars-1)
		edit->cursor++;

	if ( edit->cursor >= edit->widthInChars )
	{
		edit->scroll++;
	}

	if ( edit->cursor == edit->len + 1) {
		edit->buffer[edit->cursor] = 0;
		edit->len++;
	}
}

/*
==================
MField_Clear
==================
*/
void MField_Clear( mfield_t *edit ) {
	edit->buffer[0] = 0;
	edit->len = 0;
	edit->cursor = 0;
	edit->scroll = 0;
}

