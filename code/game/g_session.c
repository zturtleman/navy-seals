// Copyright (C) 1999-2000 Id Software, Inc.
//

// every line of code that differs from the quake3:arena SDK
// is property of manfred nerurkar
// no commercial explotation allowed
// you are only allowed to use this code in navy seals: covert operations
// a quake3 arena modifiation
// defcon-x@ns-co.net

#include "g_local.h"


/*
=======================================================================

SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
    const char	*s;
    const char	*var;

    s = va("%i %i %i %i %i %i %i",
           client->sess.sessionTeam,
           client->sess.spectatorTime,
           client->sess.spectatorState,
           client->sess.spectatorClient,
           client->sess.wins,
           client->sess.losses,
           client->sess.teamLeader
          );

    var = va( "session%i", client - level.clients );

    trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
    char	s[MAX_STRING_CHARS];
    const char	*var;

    var = va( "session%i", client - level.clients );
    trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

    sscanf( s, "%i %i %i %i %i %i %i",
            &client->sess.sessionTeam,
            &client->sess.spectatorTime,
            &client->sess.spectatorState,
            &client->sess.spectatorClient,
            &client->sess.wins,
            &client->sess.losses,
            &client->sess.teamLeader
          );
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
    clientSession_t	*sess;
    const char		*value;

    //PrintMsg(NULL, S_COLOR_RED"BLUTENGEL DEBUG MESSAGE: g_session.c G_InitSessionData\n");

    sess = &client->sess;

    // initial team determination
    if ( g_gametype.integer >= GT_TEAM ) {
        if ( g_teamAutoJoin.integer ) {
            sess->sessionTeam = PickTeam( -1 );
            BroadcastTeamChange( client, -1 );
        } else {
            // always spawn as spectator in team games
            sess->sessionTeam = TEAM_SPECTATOR;
            sess->spectatorState = SPECTATOR_FREE;
            sess->waiting = qtrue;
        }
    } else {
        value = Info_ValueForKey( userinfo, "team" );
        if ( value[0] == 's' ) {
            // a willing spectator, not a waiting-in-line
            sess->sessionTeam = TEAM_SPECTATOR;
            //	sess->waiting = qtrue;
        } else {
            switch ( g_gametype.integer ) {
            default:
            case GT_FFA:
            case GT_TEAM:/*
                if ( g_maxGameClients.integer > 0 &&
                level.numNonSpectatorClients >= g_maxGameClients.integer ) {*/
                sess->sessionTeam = TEAM_SPECTATOR;
                sess->spectatorState = SPECTATOR_FREE;

                //		sess->waiting = qtrue;
                /*

                } else {
                sess->sessionTeam = TEAM_FREE;
                }*/
                break;
            }
        }
    }

    sess->spectatorState = SPECTATOR_FREE;
    sess->spectatorTime = level.time;

    G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
    char	s[MAX_STRING_CHARS];
    int			gt;


    trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
    gt = atoi( s );

    // if the gametype changed since the last session, don't use any
    // client sessions
    if ( g_gametype.integer >= GT_TEAM ) {
        level.newSession = qtrue;
        G_Printf( "Gametype changed, clearing session data.\n" );
    }
    else // update lastmap we saved
    {
        char configstring[1024];

        trap_GetConfigstring( CS_SEALINFO, configstring, sizeof(configstring) );
        trap_Cvar_VariableStringBuffer( "lastmap", level.lastmap, sizeof(level.lastmap) );

        if (strlen(level.lastmap) <= 0)
            Com_sprintf( level.lastmap, sizeof(level.lastmap), "Unknown") ;
        Info_SetValueForKey( configstring, "lastmap", level.lastmap );

    }

}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
    int		i;
    char info[1024];
    char mapname[128];

    trap_GetServerinfo( info, sizeof(info ) );

    strncpy(mapname, Info_ValueForKey( info, "mapname" ), sizeof(mapname)-1);

    // save lastmap
    trap_Cvar_Set( "lastmap" , mapname );

    trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

    for ( i = 0 ; i < level.maxclients ; i++ ) {
        if ( level.clients[i].pers.connected == CON_CONNECTED ) {
            G_WriteClientSessionData( &level.clients[i] );
        }
    }
}
