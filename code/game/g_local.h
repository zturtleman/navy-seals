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
//
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_public.h"
#include "g_syscalls.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
#define GAMEVERSION "seals"

#define BODY_QUEUE_SIZE     MAX_CLIENTS

#define INFINITE            1000000

#define FRAMETIME           100                 // msec
#define EVENT_VALID_MSEC    300
#define CARNAGE_REWARD_TIME 3000
#define REWARD_SPRITE_TIME  2000

#define INTERMISSION_DELAY_TIME 1000
#define SP_INTERMISSION_DELAY_TIME  5000

// gentity->flags
#define FL_GODMODE              0x00000010
#define FL_NOTARGET             0x00000020
#define FL_TEAMSLAVE            0x00000400  // not the first on the team
#define FL_NO_KNOCKBACK         0x00000800
#define FL_DROPPED_ITEM         0x00001000
#define FL_NO_BOTS              0x00002000  // spawn point not for bot use
#define FL_NO_HUMANS            0x00004000  // spawn point just for bots
#define FL_FORCE_GESTURE        0x00008000  // force gesture on client
#define FL_OPENDOOR             0x00016000
#define FL_ROTATE               0x00032000

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1,
	MOVER_STOP_1TO2,
	MOVER_STOP_2TO1
} moverState_t;

#define SP_PODIUM_MODEL     "models/mapobjects/podium/podium4.md3"


typedef enum {

	ACTOR_IDLE,
	ACTOR_CHASE,
	ACTOR_OUTOFSIGHT,
	ACTOR_PANIC,
	ACTOR_DEAD,

	MAX_ACTOR_STATES
} actorState_t;

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s {
	entityState_t s;                // communicated by server to clients
	entityShared_t r;               // shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s    *client;            // NULL if not a client
	qboolean inuse;

	char        *classname;         // set in QuakeEd
	int spawnflags;             // set in QuakeEd

	qboolean neverFree;             // if true, FreeEntity will only unlink
	// bodyque uses this

	int flags;                      // FL_* variables

	char        *model;
	char        *model2;
	int freetime;                   // level.time when the object was freed

	int eventTime;                  // events will be cleared EVENT_VALID_MSEC after set
	qboolean freeAfterEvent;
	qboolean unlinkAfterEvent;

	qboolean physicsObject;         // if true, it can be pushed by movers and fall off edges
	// all game items are physicsObjects,
	float physicsBounce;        // 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask;         // brushes with this content value will be collided against
	// when moving.  items and corpses do not collide against
	// players, for instance

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;
	gentity_t   *parent;
	gentity_t   *nextTrain;
	gentity_t   *prevTrain;
	vec3_t pos1, pos2;

	char        *message;

	int timestamp;              // body queue sinking, etc

	float angle;                // set in editor, -1 = up, -2 = down
	char        *target;
	char        *targetname;
	char        *team;
	char        *targetShaderName;
	char        *targetShaderNewName;
	gentity_t   *target_ent;

	float speed;
	vec3_t movedir;

	int nextthink;
	void ( *think )( gentity_t *self );
	void ( *reached )( gentity_t *self );       // movers call this when hitting endpoint
	void ( *blocked )( gentity_t *self, gentity_t *other );
	void ( *touch )( gentity_t *self, gentity_t *other, trace_t *trace );
	void ( *use )( gentity_t *self, gentity_t *other, gentity_t *activator );
	void ( *pain )( gentity_t *self, gentity_t *attacker, int damage );
	void ( *die )( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );

	int pain_debounce_time;
	int fly_sound_debounce_time;            // wind tunnel
	int last_move_time;

	int health;

	qboolean takedamage;

	int damage;
	int splashDamage;           // quad will increase this without increasing radius
	int splashRadius;
	int methodOfDeath;
	int splashMethodOfDeath;

	int count;

	gentity_t   *chain;
	gentity_t   *enemy;
	gentity_t   *activator;
	gentity_t   *teamchain;     // next entity in team
	gentity_t   *teammaster;        // master of the team

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;

	gitem_t     *item;          // for bonus items

	// Navy Seals ++
	int ns_team;         //team for brushes
	int ns_flags;         // special entity flags
	vec3_t move_origin;
#if 1
	char        *ns_brushname;
	int ns_linkedClientsTime[MAX_CLIENTS];
#endif
	int bot_chattime;
	int spec_updatetime;

	gentity_t   *moverent;

	int elevator_index;
	actorState_t actorState;
	// Navy Seals --
};


typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,     // Beginning a team game, spawn at base
	TEAM_ACTIVE     // Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t state;

	int location;

	int captures;
	int basedefense;
	int carrierdefense;
	int flagrecovery;
	int fragcarrier;
	int assists;

	float lasthurtcarrier;
	float lastreturnedflag;
	float flagsince;
	float lastfraggedcarrier;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define FOLLOW_ACTIVE1  -1
#define FOLLOW_ACTIVE2  -2

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t sessionTeam;
	int spectatorTime;              // for determining next-in-line to play
	spectatorState_t spectatorState;
	int spectatorClient;            // for chasecam and follow mode
	int wins, losses;               // tournament stats
	qboolean teamLeader;                // true when this client is a team leader
	// Navy Seals ++
	int waiting;
	// Navy Seals --
} clientSession_t;
// Navy Seals ++
typedef struct weapon_modification_s
{
	qboolean lasersight;    // got a lasersight??
	qboolean silencer;      // got a silencer?
	qboolean scope;     // if gun got a scope (max 2x for smg / rifles)
	qboolean bayonet;       // gun got a bayonet attached?
	qboolean gl;
	qboolean duckbill;
	qboolean flashlight;
} weapon_modification_t;


// Seal Skins
typedef enum {

	SKIN_S_CURTIS,
	SKIN_S_BRUCE,

	SKIN_S_MAXSKINS
} sealSkins_t;

typedef enum {
	SKIN_T_JAMAL,
	SKIN_T_MUSTAFA,

	SKIN_T_MAXSKINS
} tangoSkins_t;


// this is the inventory of the client (persistant!)
typedef struct {
	int primaryweapon;          // selected primary weapon
	int secondaryweapon;        // selected secondary weapon
	int powerups[MAX_POWERUPS];     // selected powerups
	int ammo[WP_NUM_WEAPONS];       // selected ammo

	int faceskin;
	qboolean face_camo;         // only for seals
	qboolean face_mask;
	/* qboolean face_tatoo; */ // only for tangos
	int headstyle;

	weapon_modification_t weapon_mods[WP_NUM_WEAPONS];  // weapon modifications
} ns_inventory_t;


typedef struct {
	int technical;
	int strength;
	int stealth;
	int stamina;
	int speed;
	int accuracy;
	int playerclass;
	int xp;
	int entire_xp;
	int style;

	qboolean is_defconx_cap;
	qboolean is_defconx_hat;

	int is_democritus;
	int is_ogun;
	int is_hoak;
} playerclass_t;

// Navy Seals --
//
#define MAX_NETNAME         36
#define MAX_VOTE_COUNT      4
#define MAX_QUITMSG         128

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t connected;
	usercmd_t cmd;                  // we would lose angles if not persistant
	qboolean localClient;               // true if "ip" info key is "localhost"
	qboolean initialSpawn;              // the first spawn should be at a cool location
	//qboolean	predictItemPickup;	// based on cg_predictItems userinfo
	qboolean pmoveFixed;            //
	char netname[MAX_NETNAME];
	int maxHealth;                  // for handicapping
	int enterTime;                  // level.time the client entered the game
	playerTeamState_t teamState;                // status in teamplay games
	int voteCount;                  // to prevent people from constantly calling votes
	int lmt;                            // level time of the last message
	qboolean teamInfo;                      // send team overlay updates?
	// Navy Seals ++
	int activeMenu;
	ns_inventory_t nsInven;
	playerclass_t nsPC;
	int teamKills;
	int lastTeamKill;
	qboolean antiLag;                               // anti-lag on or off
	qboolean autoReload;                            // atuo-reload
	qboolean useBandage;                            // use bandage
	char quitMsg[MAX_QUITMSG];                          // quitmessage
	char character[MAX_NETNAME];                        //
	int radarUpdateTime;
	int nextRadarUpdate;
	int lockedPlayer;                   // will not spawn for X rounds
	int killedByMate;
	int killedByMateClientNum;
	// Navy Seals --
} clientPersistant_t;

#include "g_seals.h"

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t ps;               // communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t sess;

	qboolean readyToExit;           // wishes to leave the intermission

	qboolean noclip;

	int lastCmdTime;                // level.time of last usercmd_t, for EF_CONNECTION
	int lastChatTime;                   // level.time of last chat command
	// we can't just use pers.lastCommand.time, because
	// of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_armor;               // damage absorbed by armor
	int damage_blood;               // damage taken out of health
	int damage_knockback;           // impact damage
	vec3_t damage_from;                 // origin for vector calculation
	qboolean damage_fromWorld;              // if true, don't use the damage_from vector

	int accuracy_shots;             // total number of shots
	int accuracy_hits;              // total number of hits



	//
	int lastkilled_client;          // last client that this client killed
	int lasthurt_client;            // last client that damaged this client
	int lasthurt_mod;               // type of damage the client did

	// timers
	int respawnTime;                // can respawn when time > this, force after g_forcerespwan
	int inactivityTime;             // kick players when time > this
	qboolean inactivityWarning;             // qtrue if the five seoond warning has been given
	int rewardTime;             // clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int airOutTime;

	int lastKillTime;               // for multiple kill rewards

	qboolean fireHeld;              // used for hook
	gentity_t   *hook;              // grapple hook if out

	int switchTeamTime;             // time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

#ifdef MISSIONPACK
	gentity_t   *persistantPowerup;
	int portalID;
	int ammoTimes[WP_NUM_WEAPONS];
	int invulnerabilityTime;
#endif

	char        *areabits;
	// Navy Seals ++
	// i promise i'll do this clientside once
	int bleed_num;         // number of wounds
	vec3_t bleed_loc[32];          // bleed @?
	float bleed_delay[32];         // how long until next bleed for wound
	int bleed_point[32];         // these keep , once setted
	int bleed_methodOfDeath[32];         // so we know what killed me!
	gentity_t *bleed_causer[32];       // who killed me?
	int bleeding;          // bleeding?

	navyseals_t ns;
	gentity_t   *pTarget;
	int iMode;
	int iCameraNextUpdate;
	vec3_t vWatchOrigin;

	int weaponangle[3];
	qboolean weaponangle_changed[3];
	int weaponanglestate[3];
	int weaponshots;

	int knife_kills;
	int linkedto;


	qboolean crosshairFinishedChange;
	qboolean crosshairFadeIn;
	qboolean crosshairState;
	int crosshairTime;

	qboolean unstuck;

	// Anti-lag information
	gantilag_t antilag[MAX_ANTILAG];
	gantilag_t antilagUndo;
	int antilagHead;

	int legdamageTime;
	int legdamage;

	// Navy Seals --
};



void Team_SetFlagStatus( int team, flagStatus_t status );

//
// this structure is cleared as each map is entered
//
#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048

typedef struct {
	struct gclient_s    *clients;       // [maxclients]

	struct gentity_s    *gentities;
	int gentitySize;
	int num_entities;               // current number, <= MAX_GENTITIES

	int warmupTime;                 // restart match at this time

	fileHandle_t logFile;

	// store latched cvars here that we want to get at often
	int maxclients;

	int framenum;
	int time;                           // in msec
	int previousTime;                       // so movers can back up when blocked
	int frameStartTime;                     // the time the frame started.

	int startTime;                      // level.time the map was started

	int teamScores[TEAM_NUM_TEAMS];
	int lastTeamLocationTime;               // last time of client team location update

	qboolean newSession;                // don't use any old session data, because
	// we changed gametype

	qboolean restarted;                 // waiting for a map_restart to fire

	int numConnectedClients;
	int numNonSpectatorClients;         // includes connecting clients
	int numPlayingClients;          // connected, non-spectators
	int sortedClients[MAX_CLIENTS];         // sorted by score
	int follow1, follow2;           // clientNums for auto-follow spectators

	int snd_fry;                        // sound index for standing in lava

	int warmupModificationCount;            // for detecting if g_warmup is changed

	// voting state
	char voteString[MAX_STRING_CHARS];
	char voteDisplayString[MAX_STRING_CHARS];
	int voteTime;                       // level.time vote was called
	int voteExecuteTime;                    // time the vote is executed
	int voteYes;
	int voteNo;
	int numVotingClients;               // set by CalculateRanks

	// team voting state
	char teamVoteString[2][MAX_STRING_CHARS];
	int teamVoteTime[2];                // level.time vote was called
	int teamVoteYes[2];
	int teamVoteNo[2];
	int numteamVotingClients[2];        // set by CalculateRanks

	// spawn variables
	qboolean spawning;                  // the G_Spawn*() functions are valid
	int numSpawnVars;
	char        *spawnVars[MAX_SPAWN_VARS][2];  // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int intermissionQueued;             // intermission was qualified, but
	// wait INTERMISSION_DELAY_TIME before
	// actually going there so the last
	// frag can be watched.  Disable future
	// kills during this delay
	int intermissiontime;               // time the intermission was started
	char        *changemap;
	qboolean readyToExit;               // at least one client wants to exit
	int exitTime;
	vec3_t intermission_origin;         // also used for spectator spawns
	vec3_t intermission_angle;

	qboolean locationLinked;            // target_locations get linked
	gentity_t   *locationHead;          // head of the location list
	int bodyQueIndex;           // dead bodies
	gentity_t   *bodyQue[BODY_QUEUE_SIZE];
#ifdef MISSIONPACK
	int portalSequence;
#endif
	// Navy Seals ++
	int roundstartTime;
	int num_objectives[TEAM_NUM_TEAMS];
	int done_objectives[TEAM_NUM_TEAMS];
	int bombs[TEAM_NUM_TEAMS];
	qboolean vip[TEAM_NUM_TEAMS];
	int vipTime;
	int winTime;
	int fields[TEAM_NUM_TEAMS];
	int xpTime;
	char lastmap[MAX_QPATH];
	char nextmap[TEAM_NUM_TEAMS][MAX_QPATH];
	int looseCount;         // how often the team lost
	team_t lastLooser;          // which team lost
	int drawWinner;

	int breathsnd_male;
	int breathsnd_female;
	int breathsnd_injured;

	gentity_t   *head_bbox[MAX_CLIENTS]; // these represent the head bounding box

	int nextupdatetime; // next time the irc-cvars get updated

#define MAX_MAPCYCLE        64
	int mapCycleNumMaps;
	char mapCycleMaps[MAX_MAPCYCLE][128];

	headgear_t sealHeadgear;
	headgear_t tangoHeadgear;

	int squadRespawnTime;
	// Navy Seals --
} level_locals_t;

//
// g_gameentities.c
//
void        assault_link_all( qboolean unlink );

//
// g_seals.c
//
void        NS_CorrectWeaponAim( gentity_t *ent );
void        NS_BackupWeaponAim( gentity_t *ent );

//
// g_spawn.c
//
qboolean    G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean    G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean    G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean    G_SpawnVector( const char *key, const char *defaultString, float *out );
void        G_SpawnEntitiesFromString( void );
char *G_NewString( const char *string );

//
// g_cmds.c
//
void Cmd_Score_f( gentity_t *ent );
void StopFollowing( gentity_t *ent );
void BroadcastTeamChange( gclient_t *client, int oldTeam );
void SetTeam( gentity_t *ent, char *s );
void Cmd_FollowCycle_f( gentity_t *ent, int dir );

//
// g_items.c
//
void G_CheckTeamItems( void );
void G_RunItem( gentity_t *ent );
void RespawnItem( gentity_t *ent );

void UseHoldableItem( gentity_t *ent );
void PrecacheItem( gitem_t *it );
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle );
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity );
void SetRespawn( gentity_t *ent, float delay );
void G_SpawnItem( gentity_t *ent, gitem_t *item );
void FinishSpawningItem( gentity_t *ent );
void Think_Weapon( gentity_t *ent );
int ArmorIndex( gentity_t *ent );
void    Add_Ammo( gentity_t *ent, int weapon, int count );
void Touch_Item( gentity_t *ent, gentity_t *other, trace_t *trace );

void ClearRegisteredItems( void );
void RegisterItem( gitem_t *item );
void SaveRegisteredItems( void );

//
// g_utils.c
//
int G_ModelIndex( char *name );
int     G_SoundIndex( char *name );
void    G_TeamCommand( team_t team, char *cmd );
void    G_KillBox( gentity_t *ent );
gentity_t *G_Find( gentity_t *from, int fieldofs, const char *match );
gentity_t *G_PickTarget( char *targetname );
void    G_UseTargets( gentity_t *ent, gentity_t *activator );
void    G_SetMovedir( vec3_t angles, vec3_t movedir );

void    G_InitGentity( gentity_t *e );
gentity_t   *G_Spawn( void );
gentity_t *G_TempEntity( vec3_t origin, int event );
void    G_Sound( gentity_t *ent, int channel, int soundIndex );
void    G_FreeEntity( gentity_t *e );
qboolean    G_EntitiesFree( void );

void    G_TouchTriggers( gentity_t *ent );
void    G_TouchSolids( gentity_t *ent );

float   *tv( float x, float y, float z );
char    *vtos( const vec3_t v );

float vectoyaw( const vec3_t vec );

void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void G_AddEvent( gentity_t *ent, int event, int eventParm );
void G_SetOrigin( gentity_t *ent, vec3_t origin );
void AddRemap( const char *oldShader, const char *newShader, float timeOffset );
const char *BuildShaderStateConfig();

//
// g_combat.c
//
qboolean CanDamage( gentity_t *targ, vec3_t origin );
// Navy Seals ++
// +++++ NS:  returns the amount of damage inflicted now!
int G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod );
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius,
						 gentity_t *ignore, int mod );
// Navy Seals --

int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir );
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void TossClientItems( gentity_t *self );
#ifdef MISSIONPACK
void TossClientPersistantPowerups( gentity_t *self );
#endif
void TossClientCubes( gentity_t *self );

// damage flags
#define DAMAGE_RADIUS               0x00000001  // damage was indirect
#define DAMAGE_NO_ARMOR             0x00000002  // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK         0x00000004  // do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION        0x00000008  // armor, shields, invulnerability, and godmode have no effect
#ifdef MISSIONPACK
#define DAMAGE_NO_TEAM_PROTECTION   0x00000010  // armor, shields, invulnerability, and godmode have no effect
#endif
#define DAMAGE_NO_BLEEDING      0x00000040  // don't cause bleeding

//
// g_missile.c
//
void G_RunMissile( gentity_t *ent );

gentity_t *fire_blaster( gentity_t *self, vec3_t start, vec3_t aimdir );
gentity_t *fire_plasma( gentity_t *self, vec3_t start, vec3_t aimdir );
gentity_t *fire_grenade( gentity_t *self, vec3_t start, vec3_t aimdir, int firestrength );
gentity_t *fire_rocket( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_bfg( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_grapple( gentity_t *self, vec3_t start, vec3_t dir );
#ifdef MISSIONPACK
gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t aimdir );
#endif


//
// g_mover.c
//
void G_RunMover( gentity_t *ent );
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace );

//
// g_trigger.c
//
void trigger_teleporter_touch( gentity_t *self, gentity_t *other, trace_t *trace );


//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );
#ifdef MISSIONPACK
void DropPortalSource( gentity_t *ent );
void DropPortalDestination( gentity_t *ent );
#endif


//
// g_weapon.c
//
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void CalcMuzzlePoint( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
void SnapVectorTowards( vec3_t v, vec3_t to );
qboolean CheckMeleeAttack( gentity_t *ent, qboolean longrange );
//void Weapon_HookFree (gentity_t *ent);
void Weapon_HookThink( gentity_t *ent );


//
// g_client.c
//
team_t TeamCount( int ignoreClientNum, int team );
int TeamLeader( int team );
team_t PickTeam( int ignoreClientNum );
void SetClientViewAngle( gentity_t *ent, vec3_t angle );
gentity_t *SelectSpawnPoint( vec3_t avoidPoint, vec3_t origin, vec3_t angles );
void CopyToBodyQue( gentity_t *ent );
void respawn( gentity_t *ent );
void BeginIntermission( void );
void InitClientPersistant( gclient_t *client );
void InitClientResp( gclient_t *client );
void InitBodyQue( void );
void ClientSpawn( gentity_t *ent );
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );
void AddScore( gentity_t *ent, vec3_t origin, int score );
void CalculateRanks( void );
qboolean SpotWouldTelefrag( gentity_t *spot );

//
// g_svcmds.c
//
qboolean    ConsoleCommand( void );
void G_ProcessIPBans( void );
qboolean G_FilterPacket( char *from );

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent );
#ifdef MISSIONPACK
void G_StartKamikaze( gentity_t *ent );
#endif

//
// p_hud.c
//
void MoveClientToIntermission( gentity_t *client );
void G_SetStats( gentity_t *ent );
void DeathmatchScoreboardMessage( gentity_t *client );

//
// g_main.c
//
void FindIntermissionPoint( void );
void SetLeader( int team, int client );
void CheckTeamLeader( int team );
void G_RunThink( gentity_t *ent );
void QDECL G_LogPrintf( const char *fmt, ... );
void SendScoreboardMessageToAllClients( void );
void QDECL G_Printf( const char *fmt, ... );
void QDECL G_Error( const char *fmt, ... );

//
// g_client.c
//
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot );
void ClientUserinfoChanged( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum );
void ClientCommand( int clientNum );

//
// g_active.c
//
void ClientThink( int clientNum );
void ClientEndFrame( gentity_t *ent );
void G_RunClient( gentity_t *ent );

//
// g_team.c
//
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
void Team_CheckDroppedItem( gentity_t *dropped );
qboolean CheckObeliskAttack( gentity_t *obelisk, gentity_t *attacker );

//
// g_mem.c
//
void *G_Alloc( int size );
void G_InitMemory( void );
void Svcmd_GameMem_f( void );

//
// g_session.c
//
void G_ReadSessionData( gclient_t *client );
void G_InitSessionData( gclient_t *client, char *userinfo );

void G_InitWorldSession( void );
void G_WriteSessionData( void );

//
// g_arenas.c
//
void UpdateTournamentInfo( void );
void SpawnModelsOnVictoryPads( void );
void Svcmd_AbortPodium_f( void );

#include "g_team.h" // teamplay specific stuff


extern level_locals_t level;
extern gentity_t g_entities[MAX_GENTITIES];

#define FOFS( x ) ( (int)&( ( (gentity_t *)0 )->x ) )

extern vmCvar_t g_gametype;
extern vmCvar_t g_dedicated;
extern vmCvar_t g_cheats;
extern vmCvar_t g_maxclients;               // allow this many total, including spectators
extern vmCvar_t g_maxGameClients;           // allow this many active
extern vmCvar_t g_restarted;

extern vmCvar_t g_dmflags;
extern vmCvar_t g_fraglimit;
extern vmCvar_t g_timelimit;
extern vmCvar_t g_capturelimit;
extern vmCvar_t g_friendlyFire;
extern vmCvar_t g_password;
extern vmCvar_t g_needpass;
extern vmCvar_t g_gravity;
extern vmCvar_t g_speed;
extern vmCvar_t g_knockback;
extern vmCvar_t g_forcerespawn;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugDamage;
extern vmCvar_t g_weaponRespawn;
extern vmCvar_t g_weaponTeamRespawn;
extern vmCvar_t g_synchronousClients;
extern vmCvar_t g_motd;
extern vmCvar_t g_warmup;
extern vmCvar_t g_doWarmup;
extern vmCvar_t g_blood;
extern vmCvar_t g_allowVote;
extern vmCvar_t g_teamAutoJoin;
extern vmCvar_t g_teamForceBalance;
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;
extern vmCvar_t g_obeliskHealth;
extern vmCvar_t g_obeliskRegenPeriod;
extern vmCvar_t g_obeliskRegenAmount;
extern vmCvar_t g_obeliskRespawnDelay;
extern vmCvar_t g_cubeTimeout;
extern vmCvar_t g_redteam;
extern vmCvar_t g_blueteam;
extern vmCvar_t g_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;
extern vmCvar_t g_rankings;
extern vmCvar_t g_enableDust;
extern vmCvar_t g_enableBreath;
extern vmCvar_t g_singlePlayer;
extern vmCvar_t g_proxMineTimeout;
// Navy Seals ++
extern vmCvar_t g_logradio;
extern vmCvar_t g_allowKnifes;
extern vmCvar_t g_maxTeamKill;
extern vmCvar_t g_roundTime;
extern vmCvar_t g_baseXp;
extern vmCvar_t g_teamXp;
extern vmCvar_t g_teamRespawn;

extern vmCvar_t g_leedvelocity;
extern vmCvar_t g_shotgunleedvelocity;

extern vmCvar_t g_teamlockcamera;
extern vmCvar_t g_testSmoke;

extern vmCvar_t g_minPlayers;
extern vmCvar_t g_squadAssault;
extern vmCvar_t g_antilag;

extern int i_sNextWaitPrint;
extern qboolean b_sWaitingForPlayers;
extern int i_sCountDown;
extern int GameState;
extern int LTS_Rounds;
// Navy Seals --

