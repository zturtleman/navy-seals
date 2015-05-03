/*
===========================================================================
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

#include "g_local.h"

void Pickup_Briefcase( gentity_t *ent, gentity_t *other ) {
    other->client->ps.powerups[PW_BRIEFCASE] = 1;

    G_LogPrintf( "OBJECTIVE: [%i] \"%s\" picked up a briefcase\n", other->client->ps.clientNum, other->client->pers.netname );

    // play sound
    /*
    G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

    if ( other->client->sess.sessionTeam == TEAM_RED ) // only play sound if the otherteam
    {
    gentity_t *te;

    te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );

    te->s.eventParm = GTS_BRIEFCASE_TAKEN;
    te->r.svFlags |= SVF_BROADCAST;

    PrintMsg(NULL,"%s got the Tango Briefcase!\n",other->client->pers.netname);
    G_LogPrintf(S_COLOR_RED"%s"S_COLOR_WHITE" got the tango briefcase.\n", other->client->pers.netname);
    } */
}

qboolean NS_BriefCaseExist( void )
{
    gentity_t *ent;
    int	count = 0;

    ent = NULL;
    while ((ent = G_Find (ent, FOFS(classname), "team_briefcase")) != NULL) {
        count++;
    }

    return count;
}

void Reset_Briefcase( void )
{
    gentity_t *ent, *rent = NULL;


    ent = NULL;
    while ((ent = G_Find (ent, FOFS(classname), "team_briefcase")) != NULL) {
        if (ent->flags & FL_DROPPED_ITEM)
            G_FreeEntity(ent);
        else {
            rent = ent;
            RespawnItem(ent);
        }
    }

    // should be done over radio
    //	Team_SetFlagStatus( team, FLAG_ATBASE );

}
void Return_Briefcase( gentity_t *ent, gentity_t *other )
{
    gentity_t	*te;

    if (!other->client->ps.powerups[PW_BRIEFCASE])
        return;

    if (other->client->sess.sessionTeam != TEAM_RED )
        return;

    other->client->ps.powerups[PW_BRIEFCASE] = 0;

    PrintMsg(NULL,"%s"S_COLOR_WHITE" captured the tango briefcase!\n",other->client->pers.netname);

    G_LogPrintf( "OBJECTIVE: [%i] \"%s\" captured a briefcase\n", other->client->ps.clientNum, other->client->pers.netname  );

    // if you've already caputred it
    if ( other->client->ns.rewards & REWARD_BRIEFCASE_CAPTURE )
        other->client->ns.rewards |= REWARD_BRIEFCASE_2ND;
    else
        other->client->ns.rewards |= REWARD_BRIEFCASE_CAPTURE;
    //	CalculateRanks();

    // play sound
    te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );

    te->s.eventParm = GTS_RED_BRIEFCASE_RETURN;
    te->r.svFlags |= SVF_BROADCAST;

    level.done_objectives[TEAM_RED]++;

}

