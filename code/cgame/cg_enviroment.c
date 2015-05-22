/*
**      base by the Q3F development Team (www.q3f.com).
**
**      cg_enviroment.c
**
**      Add atmospheric effects to view.
**
**      Current supported effects are rain and snow.
*/


#include "cg_local.h"


vec3_t currvec;

#define MAX_ATMOSPHERIC_PARTICLES       2000    // maximum # of particles
#define MAX_ATMOSPHERIC_DISTANCE        1000    // maximum distance from refdef origin that particles are visible
#define MAX_ATMOSPHERIC_HEIGHT              4096    // maximum world height (FIXME: since 1.27 this should be 65536)
#define MIN_ATMOSPHERIC_HEIGHT              -4096   // minimum world height (FIXME: since 1.27 this should be -65536)
#define MAX_ATMOSPHERIC_EFFECTSHADERS   6       // maximum different effectshaders for an atmospheric effect
#define ATMOSPHERIC_DROPDELAY           100
#define ATMOSPHERIC_CUTHEIGHT           800

#define ATMOSPHERIC_RAIN_SPEED          1.1f * DEFAULT_GRAVITY
#define ATMOSPHERIC_RAIN_HEIGHT         150

#define ATMOSPHERIC_SNOW_SPEED          0.1f * DEFAULT_GRAVITY
#define ATMOSPHERIC_SNOW_HEIGHT         3

typedef struct cg_atmosphericParticle_s {
	vec3_t pos, delta, deltaNormalized, colour, surfacenormal;
	float height, minz, weight;
	qboolean active;
	int contents, surface, nextDropTime;
	qhandle_t *effectshader;
} cg_atmosphericParticle_t;

typedef struct cg_atmosphericEffect_s {
	cg_atmosphericParticle_t particles[MAX_ATMOSPHERIC_PARTICLES];
	qhandle_t effectshaders[MAX_ATMOSPHERIC_EFFECTSHADERS];
	qhandle_t effectwatershader, effectlandshader;
	int lastRainTime, numDrops;
	int gustStartTime, gustEndTime;
	int baseStartTime, baseEndTime;
	int gustMinTime, gustMaxTime;
	int changeMinTime, changeMaxTime;
	int baseMinTime, baseMaxTime;
	float baseWeight, gustWeight;
	int baseDrops, gustDrops;
	float height;
	int numEffectShaders;
	qboolean waterSplash, landSplash;
	vec3_t baseVec, gustVec;

	qboolean ( *ParticleCheckVisible )( cg_atmosphericParticle_t *particle );
	qboolean ( *ParticleGenerate )( cg_atmosphericParticle_t *particle,   float currweight );
	void ( *ParticleRender )( cg_atmosphericParticle_t *particle );
} cg_atmosphericEffect_t;

static cg_atmosphericEffect_t cg_atmFx;
qboolean CG_AtmosphericKludge( void );

/*
**  Render utility functions
*/

void CG_EffectMark( qhandle_t markShader, const vec3_t origin, const vec3_t dir, float alpha, float radius ) {
	// 'quick' version of the CG_ImpactMark function

	vec3_t axis[3];
	float texCoordScale;
	vec3_t originalPoints[4];
	byte colors[4];
	int i;
	polyVert_t          *v;
	polyVert_t verts[4];

	if ( !cg_addMarks.integer ) {
		return;
	}

	if ( radius <= 0 ) {
		CG_Error( "CG_EffectMark called with <= 0 radius" );
	}

	// create the texture axis
	VectorNormalize2( dir, axis[0] );
	PerpendicularVector( axis[1], axis[0] );
	VectorSet( axis[2], 1, 0, 0 );              // This is _wrong_, but the function is for water anyway (i.e. usually flat)
	CrossProduct( axis[0], axis[2], axis[1] );

	texCoordScale = 0.5 * 1.0 / radius;

	// create the full polygon
	for ( i = 0 ; i < 3 ; i++ ) {
		originalPoints[0][i] = origin[i] - radius * axis[1][i] - radius * axis[2][i];
		originalPoints[1][i] = origin[i] + radius * axis[1][i] - radius * axis[2][i];
		originalPoints[2][i] = origin[i] + radius * axis[1][i] + radius * axis[2][i];
		originalPoints[3][i] = origin[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	colors[0] = 127;
	colors[1] = 127;
	colors[2] = 127;
	colors[3] = alpha * 255;

	for ( i = 0, v = verts ; i < 4 ; i++, v++ ) {
		vec3_t delta;

		VectorCopy( originalPoints[i], v->xyz );

		VectorSubtract( v->xyz, origin, delta );
		v->st[0] = 0.5 + DotProduct( delta, axis[1] ) * texCoordScale;
		v->st[1] = 0.5 + DotProduct( delta, axis[2] ) * texCoordScale;
		*(int *)v->modulate = *(int *)colors;
	}

	trap_R_AddPolyToScene( markShader, 4, verts );
}

/*
**      Raindrop management functions
*/

static qboolean CG_RainParticleCheckVisible( cg_atmosphericParticle_t *particle ) {
	// Check the raindrop is visible and still going, wrapping if necessary.

	float moved;
	vec3_t distance;

	if ( !particle || !particle->active ) {
		return( qfalse );
	}

	moved = ( cg.time - cg_atmFx.lastRainTime ) * 0.001;      // Units moved since last frame
	VectorMA( particle->pos, moved, particle->delta, particle->pos );
	if ( particle->pos[2] + ATMOSPHERIC_CUTHEIGHT < particle->minz ) {
		return( particle->active = qfalse );
	}

	VectorSubtract( cg.refdef.vieworg, particle->pos, distance );
	if ( sqrt( distance[0] * distance[0] + distance[1] * distance[1] ) > MAX_ATMOSPHERIC_DISTANCE ) {
		return( particle->active = qfalse );
	}

	return( qtrue );
}

static qboolean CG_RainParticleGenerate( cg_atmosphericParticle_t *particle,  float currweight ) {
	// Attempt to 'spot' a raindrop somewhere below a sky texture.

	float angle, distance, origz;
	vec3_t testpoint, testend;
	trace_t tr;

	memset( &tr, 0, sizeof( tr ) );

	angle = random() * 2 * M_PI;
	distance = 20 + MAX_ATMOSPHERIC_DISTANCE * random();

	testpoint[0] = testend[0] = cg.refdef.vieworg[0] + sin( angle ) * distance;
	testpoint[1] = testend[1] = cg.refdef.vieworg[1] + cos( angle ) * distance;
	testpoint[2] = origz = cg.refdef.vieworg[2];
	testend[2] = testpoint[2] + MAX_ATMOSPHERIC_HEIGHT;

	while ( 1 )
	{
		if ( testpoint[2] >= MAX_ATMOSPHERIC_HEIGHT ) {
			return( qfalse );
		}
		if ( testend[2] >= MAX_ATMOSPHERIC_HEIGHT ) {
			testend[2] = MAX_ATMOSPHERIC_HEIGHT - 1;
		}
		CG_Trace( &tr, testpoint, NULL, NULL, testend, ENTITYNUM_NONE, MASK_SOLID | MASK_WATER );
		if ( tr.startsolid ) {           // Stuck in something, skip over it.
			testpoint[2] += 64;
			testend[2] = testpoint[2] + MAX_ATMOSPHERIC_HEIGHT;
		} else if ( tr.fraction == 1 ) {       // Didn't hit anything, we're (probably) outside the world
			return( qfalse );
		} else if ( tr.surfaceFlags & SURF_SKY ) { // Hit sky, this is where we start.
			break;
		} else { return( qfalse ); }
	}

	particle->active = qtrue;
	particle->colour[0] = 0.6 + 0.2 * random();
	particle->colour[1] = 0.6 + 0.2 * random();
	particle->colour[2] = 0.6 + 0.2 * random();
	VectorCopy( tr.endpos, particle->pos );
	VectorCopy( currvec, particle->delta );
	particle->delta[2] += crandom() * 100;
	VectorNormalize2( particle->delta, particle->deltaNormalized );
	particle->height = cg_atmFx.height + crandom() * 100;
	particle->weight = currweight;
	particle->effectshader = &cg_atmFx.effectshaders[0];

	distance =      ( (float)( tr.endpos[2] - MIN_ATMOSPHERIC_HEIGHT ) ) / -particle->delta[2];
	VectorMA( tr.endpos, distance, particle->delta, testend );

	CG_Trace( &tr, particle->pos, NULL, NULL, testend, ENTITYNUM_NONE, MASK_SOLID | MASK_WATER );
	particle->minz = tr.endpos[2];
	tr.endpos[2]--;
	VectorCopy( tr.plane.normal, particle->surfacenormal );
	particle->surface = tr.surfaceFlags;
	particle->contents = CG_PointContents( tr.endpos, ENTITYNUM_NONE );

	return( qtrue );
}

static void CG_RainParticleRender( cg_atmosphericParticle_t *particle ) {
	// Draw a raindrop

	vec3_t forward, right;
	polyVert_t verts[4];
	vec2_t line;
	float len, frac;
	vec3_t start, finish;

	if ( !particle->active ) {
		return;
	}

	VectorCopy( particle->pos, start );
	len = particle->height;
	if ( start[2] <= particle->minz ) {
		// Stop rain going through surfaces.
		len = particle->height - particle->minz + start[2];
		frac = start[2];
		VectorMA( start, len - particle->height, particle->deltaNormalized, start );

		if ( !cg_lowEffects.integer ) {
			frac = ( ATMOSPHERIC_CUTHEIGHT - particle->minz + frac ) / (float) ATMOSPHERIC_CUTHEIGHT;
			// Splash effects on different surfaces
			if ( particle->contents & CONTENTS_WATER  ) {
				// Water splash
				if ( cg_atmFx.effectwatershader && frac > 0 && frac < 1 ) {
					CG_EffectMark( cg_atmFx.effectwatershader, start, particle->surfacenormal, frac * 0.5, 8 - frac * 8 );
				}
			} else if ( !( particle->surface & ( SURF_NODAMAGE | SURF_NOIMPACT | SURF_NOMARKS | SURF_SKY ) ) ) {
				// Solid splash
				if ( cg_atmFx.effectlandshader && frac > 0 && frac < 1  ) {
					CG_EffectMark( cg_atmFx.effectlandshader, start, particle->surfacenormal, frac * 0.5, 8 - frac * 8 );
				}
			}


		}
	}
	if ( len <= 0 ) {
		return;
	}

	VectorCopy( particle->deltaNormalized, forward );
	VectorMA( start, -len, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, particle->weight, right, verts[0].xyz );
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 0;

	VectorMA( finish, -particle->weight, right, verts[1].xyz );
	verts[1].st[0] = 0;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 0;

	VectorMA( start, -particle->weight, right, verts[2].xyz );
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 127;

	VectorMA( start, particle->weight, right, verts[3].xyz );
	verts[3].st[0] = 1;
	verts[3].st[1] = 1;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 127;

	trap_R_AddPolyToScene( *particle->effectshader, 4, verts );
}

#if 1
/*
**      Snow management functions
*/

static qboolean CG_SnowParticleGenerate( cg_atmosphericParticle_t *particle,  float currweight ) {
	// Attempt to 'spot' a raindrop somewhere below a sky texture.

	float angle, distance, origz;
	vec3_t testpoint, testend;
	trace_t tr;

	angle = random() * 2 * M_PI;
	distance = 20 + MAX_ATMOSPHERIC_DISTANCE * random();

	testpoint[0] = testend[0] = cg.refdef.vieworg[0] + sin( angle ) * distance;
	testpoint[1] = testend[1] = cg.refdef.vieworg[1] + cos( angle ) * distance;
	testpoint[2] = origz = cg.refdef.vieworg[2];
	testend[2] = testpoint[2] + MAX_ATMOSPHERIC_HEIGHT;

	while ( 1 )
	{
		if ( testpoint[2] >= MAX_ATMOSPHERIC_HEIGHT ) {
			return( qfalse );
		}
		if ( testend[2] >= MAX_ATMOSPHERIC_HEIGHT ) {
			testend[2] = MAX_ATMOSPHERIC_HEIGHT - 1;
		}
		CG_Trace( &tr, testpoint, NULL, NULL, testend, ENTITYNUM_NONE, MASK_SOLID | MASK_WATER );
		if ( tr.startsolid ) {           // Stuck in something, skip over it.
			testpoint[2] += 64;
			testend[2] = testpoint[2] + MAX_ATMOSPHERIC_HEIGHT;
		} else if ( tr.fraction == 1 ) {       // Didn't hit anything, we're (probably) outside the world
			return( qfalse );
		} else if ( tr.surfaceFlags & SURF_SKY ) { // Hit sky, this is where we start.
			break;
		} else { return( qfalse ); }
	}

	particle->active = qtrue;
	particle->colour[0] = 0.6 + 0.2 * random();
	particle->colour[1] = 0.6 + 0.2 * random();
	particle->colour[2] = 0.6 + 0.2 * random();
	VectorCopy( tr.endpos, particle->pos );
	VectorCopy( currvec, particle->delta );
	particle->delta[2] += -200 - ( random() * 50 );
	VectorNormalize2( particle->delta, particle->deltaNormalized );
	particle->height = 1.5f + random() * 2;
	particle->weight = random() / 2;


	particle->effectshader = &cg_atmFx.effectshaders[0];

	distance =      ( (float)( tr.endpos[2] - MIN_ATMOSPHERIC_HEIGHT ) ) / -particle->delta[2];
	VectorMA( tr.endpos, distance, particle->delta, testend );

	CG_Trace( &tr, particle->pos, NULL, NULL, testend, ENTITYNUM_NONE, MASK_SOLID | MASK_WATER );
	particle->minz = tr.endpos[2];
	tr.endpos[2]--;
	VectorCopy( tr.plane.normal, particle->surfacenormal );
	particle->surface = tr.surfaceFlags;
	particle->contents = CG_PointContents( tr.endpos, ENTITYNUM_NONE );

	{
		float dist = Distance( cg.refdef.vieworg, particle->pos );

		if ( dist < 400 ) {
			particle->pos[2] += 400 - dist;
		}
	}
	return( qtrue );
}
#endif
static void CG_SnowParticleRender( cg_atmosphericParticle_t *particle ) {
	// Draw a raindrop

	vec3_t forward, right;
	polyVert_t verts[4];
	vec2_t line;
	float len, frac;
	vec3_t start, finish;
	byte color;

	if ( !particle->active ) {
		return;
	}

	VectorCopy( particle->pos, start );
	len = particle->height;

	if ( start[2] <= particle->minz ) {
		// Stop rain going through surfaces.
		len = particle->height - particle->minz + start[2];
		frac = start[2];
		VectorMA( start, len - particle->height, particle->deltaNormalized, start );

		frac = ( ATMOSPHERIC_CUTHEIGHT - particle->minz + frac ) / (float)ATMOSPHERIC_CUTHEIGHT;

		// Solid splash
		/*	if( cg_atmFx.effectlandshader && frac > 0 && frac < 1  )
		{
		if ( !(particle->surface & SURF_SNOWSTEPS) && random() < 0.3  )
		CG_ImpactMark( cgs.media.burnMarkShaders[(int)(random()*2)], start, particle->surfacenormal, frac * 0.5,1,1,1,1,qfalse, 4,qfalse );

		cg_atmFx.ParticleGenerate( particle, 1 );
		return;
		}
		*/

	}

	color = 255;
	if ( len <= 0 ) {
		int moo = len;
		float alpha = 1;

		len = 4;

		if ( moo <= 0 ) {
			cg_atmFx.ParticleGenerate( particle, 1 );
			return;
		}
		color = (byte)( alpha * 255.0f );
	}

	if ( color > 255 ) {
		color = 255;
	}

	VectorCopy( particle->deltaNormalized, forward );
	VectorMA( start, -len, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );


	VectorMA( finish,  1, right, verts[0].xyz );
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = color;

	VectorMA( finish, -1, right, verts[1].xyz );
	verts[1].st[0] = 0;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = color;

	VectorMA( start, -1, right, verts[2].xyz );
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = color;

	VectorMA( start, 1, right, verts[3].xyz );
	verts[3].st[0] = 1;
	verts[3].st[1] = 1;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = color;

	//      trap_R_AddPolyToScene( trap_R_RegisterShader("gfx/misc/particle_08.tga"), 4, verts );
	trap_R_AddPolyToScene( *particle->effectshader, 4, verts );
}
/*
**      Set up gust parameters.
*/

static void CG_EffectGust( void ) {
	// Generate random values for the next gust

	int diff;

	cg_atmFx.baseEndTime        = cg.time                   + cg_atmFx.baseMinTime          + ( rand() % ( cg_atmFx.baseMaxTime - cg_atmFx.baseMinTime ) );
	diff                        = cg_atmFx.changeMaxTime    - cg_atmFx.changeMinTime;
	cg_atmFx.gustStartTime      = cg_atmFx.baseEndTime          + cg_atmFx.changeMinTime    + ( diff ? ( rand() % diff ) : 0 );
	diff                        = cg_atmFx.gustMaxTime          - cg_atmFx.gustMinTime;
	cg_atmFx.gustEndTime        = cg_atmFx.gustStartTime    + cg_atmFx.gustMinTime          + ( diff ? ( rand() % diff ) : 0 );
	diff                        = cg_atmFx.changeMaxTime    - cg_atmFx.changeMinTime;
	cg_atmFx.baseStartTime          = cg_atmFx.gustEndTime          + cg_atmFx.changeMinTime    + ( diff ? ( rand() % diff ) : 0 );
}

static qboolean CG_EffectGustCurrent( vec3_t curr, float *weight, int *num ) {
	// Calculate direction for new drops.

	vec3_t temp;
	float frac;

	if ( cg.time < cg_atmFx.baseEndTime ) {
		VectorCopy( cg_atmFx.baseVec, curr );
		*weight = cg_atmFx.baseWeight;
		*num = cg_atmFx.baseDrops;
	} else {
		VectorSubtract( cg_atmFx.gustVec, cg_atmFx.baseVec, temp );
		if ( cg.time < cg_atmFx.gustStartTime ) {
			frac = ( (float)( cg.time - cg_atmFx.baseEndTime ) ) / ( (float)( cg_atmFx.gustStartTime - cg_atmFx.baseEndTime ) );
			VectorMA( cg_atmFx.baseVec, frac, temp, curr );
			*weight = cg_atmFx.baseWeight + ( cg_atmFx.gustWeight - cg_atmFx.baseWeight ) * frac;
			*num = cg_atmFx.baseDrops + ( (float)( cg_atmFx.gustDrops - cg_atmFx.baseDrops ) ) * frac;
		} else if ( cg.time < cg_atmFx.gustEndTime ) {
			VectorCopy( cg_atmFx.gustVec, curr );
			*weight = cg_atmFx.gustWeight;
			*num = cg_atmFx.gustDrops;
		} else
		{
			frac = 1.0 - ( (float)( cg.time - cg_atmFx.gustEndTime ) ) / ( (float)( cg_atmFx.baseStartTime - cg_atmFx.gustEndTime ) );
			VectorMA( cg_atmFx.baseVec, frac, temp, curr );
			*weight = cg_atmFx.baseWeight + ( cg_atmFx.gustWeight - cg_atmFx.baseWeight ) * frac;
			*num = cg_atmFx.baseDrops + ( (float)( cg_atmFx.gustDrops - cg_atmFx.baseDrops ) ) * frac;
			if ( cg.time >= cg_atmFx.baseStartTime ) {
				return( qtrue );
			}
		}
	}
	return( qfalse );
}

/*
** Main render loop
*/

void CG_AddAtmosphericEffects( void ) {
	// Add atmospheric effects (e.g. rain, snow etc.) to view

	int curr, max, currnum;
	cg_atmosphericParticle_t *particle;
	float currweight;

	if ( cg_atmFx.numDrops <= 0 || cg_atmFx.numEffectShaders == 0 ) {
		return;
	}

	if ( cg_atmosphericEffects.value <= 0 ) {
		return;
	}

	max = cg_lowEffects.integer ?  cg_atmFx.numDrops / 2 : cg_atmFx.numDrops;

	if ( CG_EffectGustCurrent( currvec, &currweight, &currnum ) ) {
		CG_EffectGust();            // Recalculate gust parameters

	}
	for ( curr = 0; curr < max; curr++ )
	{
		particle = &cg_atmFx.particles[curr];
		if ( !cg_atmFx.ParticleCheckVisible( particle ) ) {
			// Effect has terminated / fallen from screen view
			if ( !particle->nextDropTime ) {
				// Stop rain being synchronized
				particle->nextDropTime = rand() % ATMOSPHERIC_DROPDELAY;
			} else if ( currnum < curr || particle->nextDropTime > cg.time ) {
				continue;
			}
			if ( !cg_atmFx.ParticleGenerate( particle, currweight ) ) {
				// Ensure it doesn't attempt to generate every frame, to prevent
				// 'clumping' when there's only a small sky area available.
				particle->nextDropTime = cg.time + ATMOSPHERIC_DROPDELAY;
				continue;
			}
		}

		cg_atmFx.ParticleRender( particle );
	}

	cg_atmFx.lastRainTime = cg.time;
}


/*
**      G_AtmosphericKludge
*/

static qboolean kludgeChecked, kludgeResult;
qboolean CG_AtmosphericKludge() {
	// Activate effects for specified kludge maps that don't
	// have it specified for them.

	if ( kludgeChecked ) {
		return( kludgeResult );
	}
	kludgeChecked = qtrue;
	kludgeResult = qfalse;

	/*if( !Q_stricmp( cgs.mapname, "maps/2night3.bsp" ) )
	{
	CG_EffectParse( "T=RAIN" );
	return( kludgeResult = qtrue );
	}*/

	return( kludgeResult = qfalse );
}

/*
============
dX: enviroment parse

parses the enviroment variables from the .bmf file

============
*/

void CG_EnviromentParse( char       *text_p ) {
	char        *token;
	char *type;
	float bmin, bmax, cmin, cmax, gmin, gmax, bdrop, gdrop, wsplash, lsplash, height;
	int count;

	if ( !text_p ) {
		CG_Error( "Tried to parse Enviroment Variables. text_p = NULL" );
		return;
	}

	if ( CG_AtmosphericKludge() ) {
		return;
	}

	// Set up some default values
	cg_atmFx.baseVec[0] = cg_atmFx.baseVec[1] = 0;
	cg_atmFx.gustVec[0] = cg_atmFx.gustVec[1] = 100;
	bmin = 5;
	bmax = 10;
	cmin = 1;
	cmax = 1;
	gmin = 0;
	gmax = 2;
	bdrop = gdrop = 300;
	cg_atmFx.baseWeight = 0.7f;
	cg_atmFx.gustWeight = 1.5f;
	wsplash = 1;
	lsplash = 1;
	height  = 100;
	type = NULL;

	while ( 1 )
	{

		token = COM_Parse( &text_p );

		if ( !token ) {
			CG_Error( "Invalid End-of-File. Missing '}'" );
			break;
		}

		if ( !Q_stricmp( token, "}" ) ) {
			break;
		}

		if ( !Q_stricmp( token, "type" ) ) { // found type token
			token = COM_Parse( &text_p );

			if ( !Q_stricmp( token, "rain" ) ) {
				type = "rain";
				cg_atmFx.ParticleCheckVisible = &CG_RainParticleCheckVisible;
				cg_atmFx.ParticleGenerate = &CG_RainParticleGenerate;
				cg_atmFx.ParticleRender = &CG_RainParticleRender;

				cg_atmFx.baseVec[2] = cg_atmFx.gustVec[2] = -ATMOSPHERIC_RAIN_SPEED;
			} else if ( !Q_stricmp( token, "snow" ) ) {
				type = "snow";
				cg_atmFx.ParticleCheckVisible = &CG_RainParticleCheckVisible;
				cg_atmFx.ParticleGenerate = &CG_SnowParticleGenerate;
				cg_atmFx.ParticleRender = &CG_SnowParticleRender;
			} else // !FIXME: Add more types here
			{
				cg_atmFx.numDrops = 0;
				CG_Printf( "Wrong effect type.\n" );
				return;
			}
			continue;
		} else if ( !Q_stricmp( token, "basetimes" ) ) {
			token = COM_Parse( &text_p );
			bmin = atof( token );

			token = COM_Parse( &text_p );
			bmax = atof( token );
			continue;
		} else if ( !Q_stricmp( token, "timetomove" ) ) {
			token = COM_Parse( &text_p );
			cmin = atof( token );

			token = COM_Parse( &text_p );
			cmax = atof( token );
			continue;
		} else if ( !Q_stricmp( token, "gusttimes" ) ) {
			token = COM_Parse( &text_p );
			gmin = atof( token );

			token = COM_Parse( &text_p );
			gmax = atof( token );
			continue;
		} else if ( !Q_stricmp( token, "basevector" ) ) {
			token = COM_Parse( &text_p );
			cg_atmFx.baseVec[0] = atof( token );

			token = COM_Parse( &text_p );
			cg_atmFx.baseVec[1] = atof( token );
			continue;
		} else if ( !Q_stricmp( token, "gustvector" ) ) {
			token = COM_Parse( &text_p );
			cg_atmFx.gustVec[0] = atof( token );

			token = COM_Parse( &text_p );
			cg_atmFx.gustVec[1] = atof( token );
			continue;
		} else if ( !Q_stricmp( token, "weight" ) ) {

			token = COM_Parse( &text_p );
			cg_atmFx.baseWeight = atof( token );

			token = COM_Parse( &text_p );
			cg_atmFx.gustWeight = atof( token );
			continue;
		} else if ( !Q_stricmp( token, "impacteffects" ) ) {

			token = COM_Parse( &text_p );
			if ( !Q_stricmp( token, "true" ) ) {
				wsplash = 1;
			} else {
				wsplash = 0;
			}

			token = COM_Parse( &text_p );
			if ( !Q_stricmp( token, "true" ) ) {
				lsplash = 1;
			} else {
				lsplash = 0;
			}

			continue;
		} else if ( !Q_stricmp( token, "num_drops" ) ) {

			token = COM_Parse( &text_p );
			bdrop = atof( token );

			token = COM_Parse( &text_p );
			gdrop = atof( token );
			continue;
		} else if ( !Q_stricmp( token, "height" ) ) {
			height = atof( COM_Parse( &text_p ) );
			continue;
		} else
		{
			CG_Error( "Unknown token (%s).\n", token );
			break;
		}
	}

	if ( !type ) {
		// No effects
		cg_atmFx.numDrops = -1;
		return;
	}
	/*
	cg_atmFx.baseVec[0] = cg_atmFx.baseVec[1] = 0;
	cg_atmFx.gustVec[0] = cg_atmFx.gustVec[1] = 100;
	bmin = 5;
	bmax = 10;
	cmin = 1;
	cmax = 1;
	gmin = 0;
	gmax = 2;
	bdrop = gdrop = 300;
	cg_atmFx.baseWeight = 0.7f;
	cg_atmFx.gustWeight = 1.5f;
	wsplash = 1;
	lsplash = 1;*/

	cg_atmFx.baseMinTime    =   1000 * bmin;
	cg_atmFx.baseMaxTime    =   1000 * bmax;
	cg_atmFx.changeMinTime  =   1000 * cmin;
	cg_atmFx.changeMaxTime  =   1000 * cmax;
	cg_atmFx.gustMinTime    =   1000 * gmin;
	cg_atmFx.gustMaxTime    =   1000 * gmax;
	cg_atmFx.baseDrops      =   bdrop;
	cg_atmFx.gustDrops      =   gdrop;
	cg_atmFx.waterSplash    =   wsplash;
	cg_atmFx.landSplash     =   lsplash;
	cg_atmFx.height         =   height;

	cg_atmFx.numDrops = ( cg_atmFx.baseDrops > cg_atmFx.gustDrops ) ? cg_atmFx.baseDrops : cg_atmFx.gustDrops;

	if ( cg_atmFx.numDrops > MAX_ATMOSPHERIC_PARTICLES ) {
		cg_atmFx.numDrops = MAX_ATMOSPHERIC_PARTICLES;
	}

	//
	// Load graphics
	//

	// Rain
	if ( !Q_stricmp( type, "rain" ) ) {
		cg_atmFx.numEffectShaders = 1;

		cg_atmFx.effectshaders[0] = trap_R_RegisterShader( "gfx/atmosphere/raindrop" );

		if ( !( cg_atmFx.effectshaders[0] ) ) {
			cg_atmFx.effectshaders[0] = -1;
		}

		if ( cg_atmFx.waterSplash ) {
			cg_atmFx.effectwatershader = trap_R_RegisterShader( "gfx/atmosphere/raindropwater" );
		}
		if ( cg_atmFx.landSplash ) {
			cg_atmFx.effectlandshader = trap_R_RegisterShader( "gfx/atmosphere/raindropsolid" );
		}
	} // Snow
	else if ( !Q_stricmp( type, "snow" ) ) {
		for ( cg_atmFx.numEffectShaders = 0; cg_atmFx.numEffectShaders < 6; cg_atmFx.numEffectShaders++ )
		{
			cg_atmFx.effectshaders[cg_atmFx.numEffectShaders] = trap_R_RegisterShader( va( "gfx/atmosphere/snowflake0%i", cg_atmFx.numEffectShaders ) );

			if ( !( cg_atmFx.effectshaders[cg_atmFx.numEffectShaders] ) ) {
				cg_atmFx.effectshaders[cg_atmFx.numEffectShaders] = -1;     // we had some kind of a problem
			}
		}
		cg_atmFx.waterSplash = 0;
		cg_atmFx.landSplash = 1;

		cg_atmFx.effectlandshader = trap_R_RegisterShader( "gfx/atmosphere/raindropsolid" );
	} // This really should never happen
	else {
		cg_atmFx.numEffectShaders = 0;
	}

	// Initialise atmospheric effect to prevent all particles falling at the start
	for ( count = 0; count < cg_atmFx.numDrops; count++ )
		cg_atmFx.particles[count].nextDropTime = ATMOSPHERIC_DROPDELAY + ( rand() % ATMOSPHERIC_DROPDELAY );

	CG_EffectGust();



}
