/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2007 Team Mirage

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

#ifndef __CG_SYSCALLS_H__
#define __CG_SYSCALLS_H__

//
// system traps
// These functions are how the cgame communicates with the main game system
//

void trap_PumpEventLoop( void );

// print message on the local console
void        trap_Print( const char *fmt );

// abort the game
void        trap_Error( const char *fmt ) __attribute__( ( noreturn ) );

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int         trap_Milliseconds( void );
int         trap_RealTime( qtime_t *qtime );

// console variable interaction
void        trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void        trap_Cvar_Update( vmCvar_t *vmCvar );
void        trap_Cvar_Set( const char *var_name, const char *value );
void        trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void        trap_Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize );


// ServerCommand and ConsoleCommand parameter access
int         trap_Argc( void );
void        trap_Argv( int n, char *buffer, int bufferLength );
void        trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int         trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void        trap_FS_Read( void *buffer, int len, fileHandle_t f );
void        trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void        trap_FS_FCloseFile( fileHandle_t f );
int         trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
int         trap_FS_Delete( const char *filename );

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void        trap_SendConsoleCommand( const char *text );

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void        trap_AddCommand( const char *cmdName );

// send a string to the server over the network
void        trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void        trap_UpdateScreen( void );

// model collision
void        trap_CM_LoadMap( const char *mapname );
int         trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );      // 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs );
int         trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int         trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void        trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
							  const vec3_t mins, const vec3_t maxs,
							  clipHandle_t model, int brushmask );
void        trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
										 const vec3_t mins, const vec3_t maxs,
										 clipHandle_t model, int brushmask,
										 const vec3_t origin, const vec3_t angles );

void        trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
								  const vec3_t mins, const vec3_t maxs,
								  clipHandle_t model, int brushmask );
void        trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
											 const vec3_t mins, const vec3_t maxs,
											 clipHandle_t model, int brushmask,
											 const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int         trap_CM_MarkFragments( int numPoints, const vec3_t *points,
								   const vec3_t projection,
								   int maxPoints, vec3_t pointBuffer,
								   int maxFragments, markFragment_t *fragmentBuffer );

// ydnar: projects a decal onto brush model surfaces
void        trap_R_ProjectDecal( qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime );
void        trap_R_ClearDecals( void );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void        trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
//void		trap_S_StartSoundExtended( vec3_t origin, float volume, float rolloff, float pitch, int entityNum, int entchannel, sfxHandle_t sfx );
#define     trap_S_StartSoundExtended( origin, volume, rolloff, pitch, entityNum, entchannel, sfx ) do { trap_S_StartSound( origin, entityNum, entchannel, sfx ), (void)( volume ), (void)( rolloff ), (void)( pitch ); } while ( 0 )
void        trap_S_StartSoundVControl( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int volume );
void        trap_S_StartSoundEx( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags );
void        trap_S_StartSoundExVControl( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags, int volume );
void        trap_S_StopStreamingSound( int entnum );  // usually AI.  character is talking and needs to be shut up /now/
int         trap_S_GetSoundLength( sfxHandle_t sfx );
int         trap_S_GetCurrentSoundTime( void ); // ydnar

// a local sound is always played full volume
void        trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void        trap_S_ClearLoopingSounds( void );
void        trap_S_ClearSounds( qboolean killmusic );
void        trap_S_AddLoopingSound( const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume, int soundTime );
void        trap_S_AddRealLoopingSound( const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range, int volume, int soundTime );
void        trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// Ridah, talking animations
int         trap_S_GetVoiceAmplitude( int entityNum );
// done.

// repatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed );        // returns buzz if not found
void        trap_S_StartBackgroundTrack( const char *intro, const char *loop, int fadeupTime ); // empty name stops music
void        trap_S_FadeBackgroundTrack( float targetvol, int time, int num );
void        trap_S_StopBackgroundTrack( void );
int         trap_S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation );
void        trap_S_FadeAllSound( float targetvol, int time, qboolean stopsounds );

void        trap_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t   trap_R_RegisterModel( const char *name );           // returns rgb axis if not found
//qhandle_t	trap_R_RegisterPermanentModel( const char *name );			// returns rgb axis if not found
#define     trap_R_RegisterPermanentModel( x ) trap_R_RegisterModel( x )
qhandle_t   trap_R_RegisterSkin( const char *name );            // returns all white if not found
qhandle_t   trap_R_RegisterShader( const char *name );          // returns all white if not found
qhandle_t   trap_R_RegisterShaderNoMip( const char *name );         // returns all white if not found

qboolean    trap_R_GetSkinModel( qhandle_t skinid, const char *type, char *name );          //----(SA) added
qhandle_t   trap_R_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap );  //----(SA)	added

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void        trap_R_ClearScene( void );
void        trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void        trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void        trap_R_AddPolyBufferToScene( polyBuffer_t* pPolyBuffer );
// Ridah
void        trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
// done.
void        trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags );
void        trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible );
void        trap_R_RenderScene( const refdef_t *fd );
void        trap_R_SetColor( const float *rgba );   // NULL = 1,1,1,1
void        trap_R_DrawStretchPic( float x, float y, float w, float h,
								   float s1, float t1, float s2, float t2, qhandle_t hShader );
void        trap_R_DrawRotatedPic( float x, float y, float w, float h,
								   float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );    // NERVE - SMF
void        trap_R_DrawStretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType );
void        trap_R_Add2dPolys( polyVert_t* verts, int numverts, qhandle_t hShader );
void        trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int         trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void        trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

// Save out the old render info so we don't kill the LOD system here
void trap_R_SaveViewParms( void );

// Reset the view parameters
void trap_R_RestoreViewParms( void );

// Set fog
void    trap_R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density );
void    trap_R_SetGlobalFog( qboolean restore, int duration, float r, float g, float b, float depthForOpaque );

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void        trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void        trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void        trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean    trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean    trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int         trap_GetCurrentCmdNumber( void );

qboolean    trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon/holdable select and zoom
void        trap_SetUserCmdValue( int stateValue, int flags, float sensitivityScale, int mpIdentClient );
void        trap_SetClientLerpOrigin( float x, float y, float z );      // DHM - Nerve

// aids for VM testing
void        testPrintInt( char *string, int i );
void        testPrintFloat( char *string, float f );

int         trap_MemoryRemaining( void );
void        trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font );
qboolean    trap_Key_IsDown( int keynum );
int         trap_Key_GetCatcher( void );
void        trap_Key_SetCatcher( int catcher );
void        trap_Key_KeysForBinding( const char* binding, int* key1, int* key2 );
int         trap_Key_GetKey( const char *binding );
qboolean    trap_Key_GetOverstrikeMode( void );
void        trap_Key_SetOverstrikeMode( qboolean state );

// RF
void trap_SendMoveSpeedsToGame( int entnum, char *movespeeds );

//void trap_UI_Popup(const char *arg0);	//----(SA)	added
void trap_UI_Popup( int arg0 );

// NERVE - SMF
qhandle_t getTestShader( void ); // JPW NERVE shhh
void trap_UI_ClosePopup( const char *arg0 );
void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void trap_Key_SetBinding( int keynum, const char *binding );
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
// -NERVE - SMF

void trap_TranslateString( const char *string, char *buf );     // NERVE - SMF - localization

int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status trap_CIN_StopCinematic( int handle );
e_status trap_CIN_RunCinematic( int handle );
void trap_CIN_DrawCinematic( int handle );
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h );

void trap_SnapVector( float *v );

qboolean    trap_GetEntityToken( char *buffer, int bufferSize );
qboolean    trap_R_inPVS( const vec3_t p1, const vec3_t p2 );
void        trap_GetHunkData( int* hunkused, int* hunkexpected );

//zinx - binary message channel
void        trap_SendMessage( char *buf, int buflen );
messageStatus_t trap_MessageStatus( void );

//bani - dynamic shaders
qboolean    trap_R_LoadDynamicShader( const char *shadername, const char *shadertext );
//fretn - render to texture
void    trap_R_RenderToTexture( int textureid, int x, int y, int w, int h );
int trap_R_GetTextureId( const char *name );
//bani - flush rendering buffer
void    trap_R_Finish( void );

// Duffy, camera stuff
#define CAM_PRIMARY 0   // the main camera for cutscenes, etc.
qboolean    trap_loadCamera( int camNum, const char *name );
void        trap_startCamera( int camNum, int time );
void        trap_stopCamera( int camNum );
qboolean    trap_getCameraInfo( int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov );

#endif
