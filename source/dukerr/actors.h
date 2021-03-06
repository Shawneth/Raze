//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef __actors_h_
#define __actors_h_

BEGIN_DUKERR_NS

#define MAXSLEEPDIST        16384
#define SLEEPTIME           1536
#define ACTOR_FLOOR_OFFSET  (1<<8)
#define ZOFFSET             (1<<8)
#define ZOFFSET2            (16<<8)
#define ZOFFSET3            (8<<8)
#define ZOFFSET4            (12<<8)
#define ZOFFSET5            (32<<8)
#define ZOFFSET6            (4<<8)

#define ACTOR_MAXFALLINGZVEL 6144
#define ACTOR_ONWATER_ADDZ (24<<8)

// KEEPINSYNC lunatic/con_lang.lua
#define STAT_DEFAULT        0
#define STAT_ACTOR          1
#define STAT_ZOMBIEACTOR    2
#define STAT_EFFECTOR       3
#define STAT_PROJECTILE     4
#define STAT_MISC           5
#define STAT_STANDABLE      6
#define STAT_LOCATOR        7
#define STAT_ACTIVATOR      8
#define STAT_TRANSPORT      9
#define STAT_PLAYER         10
#define STAT_FX             11
#define STAT_FALLER         12
#define STAT_DUMMYPLAYER    13
#define STAT_LIGHT          14
#define STAT_RAROR          15
#define STAT_NETALLOC       (MAXSTATUS-1)


// Defines the motion characteristics of an actor
enum amoveflags_t
{
    face_player       = 1,
    geth              = 2,
    getv              = 4,
    random_angle      = 8,
    face_player_slow  = 16,
    spin              = 32,
    face_player_smart = 64,
    fleeenemy         = 128,
    jumptoplayer_only = 256,
    jumptoplayer_bits = 257,  // NOTE: two bits set!
    seekplayer        = 512,
    furthestdir       = 1024,
    dodgebullet       = 4096,
    justjump2         = 8192,
    windang           = 16384,
    antifaceplayerslow = 32768
};

// Defines for 'useractor' keyword
enum uactortypes_t
{
    notenemy,
    enemy,
    enemystayput
};

// These macros are there to give names to the t_data[]/T*/vm.g_t[] indices
// when used with actors. Greppability of source code is certainly a virtue.
#define AC_COUNT(t) ((t)[0])  /* the actor's count */
/* The ID of the actor's current move. In C-CON, the bytecode offset to the
 * move composite: */
#define AC_MOVE_ID(t) ((t)[1])
#define AC_ACTION_COUNT(t) ((t)[2])  /* the actor's action count */
#define AC_CURFRAME(t) ((t)[3])  /* the actor's current frame offset */
/* The ID of the actor's current action. In C-CON, the bytecode offset to the
 * action composite: */
#define AC_ACTION_ID(t) ((t)[4])
#define AC_AI_ID(t) ((t)[5])  /* the ID of the actor's current ai */

enum actionparams
{
    ACTION_STARTFRAME = 0,
    ACTION_NUMFRAMES,
    ACTION_VIEWTYPE,
    ACTION_INCVAL,
    ACTION_DELAY,
    ACTION_FLAGS,
    ACTION_PARAM_COUNT,
};

enum actionflags
{
    AF_VIEWPOINT = 1u<<0u,
};

// Select an actor's actiontics and movflags locations depending on
// whether we compile the Lunatic build.
// <spr>: sprite pointer
// <a>: actor_t pointer
# define AC_ACTIONTICS(spr, a) ((spr)->lotag)
# define AC_MOVFLAGS(spr, a) ((spr)->hitag)

// (+ 40 16 16 4 8 6 8 6 4 20)
#pragma pack(push, 1)
struct actor_t
{
    int32_t t_data[10];  // 40b sometimes used to hold offsets to con code

    int32_t flags;                             // 4b
    vec3_t  bpos;                              // 12b
    int32_t floorz, ceilingz;                  // 8b
    vec2_t lastv;                              // 8b
    int16_t picnum, ang, extra, owner;         // 8b
    int16_t movflag, tempang, timetosleep;     // 6b
    int16_t stayput;                           // 2b

    uint8_t cgg, lasttransport;                // 2b
    // NOTE: 'dispicnum' is updated every frame, not in sync with game tics!
    int16_t dispicnum;                         // 2b

#ifdef POLYMER
    int16_t lightId, lightmaxrange;  // 4b
    _prlight *lightptr;              // 4b/8b  aligned on 96 bytes
    uint8_t lightcount, filler[3];
#endif
};

#pragma pack(pop)

struct projectile_t;

struct tiledata_t
{
    intptr_t *execPtr;  // pointer to CON script for this tile, formerly actorscrptr
    intptr_t *loadPtr;  // pointer to load time CON script, formerly actorLoadEventScrPtr or something
    projectile_t *proj;
    projectile_t *defproj;
    uint32_t      flags;       // formerly SpriteFlags, ActorType
    int32_t       cacherange;  // formerly SpriteCache
};


// KEEPINSYNC lunatic/con_lang.lua
enum sflags_t
{
    SFLAG_SHADOW        = 0x00000001,
    SFLAG_NVG           = 0x00000002,
    SFLAG_NOSHADE       = 0x00000004,
    SFLAG_PROJECTILE    = 0x00000008,
    SFLAG_DECAL         = 0x00000010,
    SFLAG_BADGUY        = 0x00000020,
    SFLAG_NOPAL         = 0x00000040,
    SFLAG_NOEVENTCODE   = 0x00000080,
    SFLAG_NOLIGHT       = 0x00000100,
    SFLAG_USEACTIVATOR  = 0x00000200,
    SFLAG_NULL          = 0x00000400,  // null sprite in multiplayer
    SFLAG_NOCLIP        = 0x00000800,  // clipmove it with cliptype 0
    SFLAG_NOFLOORSHADOW = 0x00001000,  // for temp. internal use, per-tile flag not checked
    SFLAG_SMOOTHMOVE    = 0x00002000,
    SFLAG_NOTELEPORT    = 0x00004000,
    SFLAG_BADGUYSTAYPUT = 0x00008000,
    SFLAG_CACHE         = 0x00010000,
    // rotation-fixed wrt a pivot point to prevent position diverging due to
    // roundoff error accumulation:
    SFLAG_ROTFIXED         = 0x00020000,
    SFLAG_HARDCODED_BADGUY = 0x00040000,
    SFLAG_DIDNOSE7WATER    = 0x00080000,  // used temporarily
    SFLAG_NODAMAGEPUSH     = 0x00100000,
    SFLAG_NOWATERDIP       = 0x00200000,
    SFLAG_HURTSPAWNBLOOD   = 0x00400000,
    SFLAG_GREENSLIMEFOOD   = 0x00800000,
    SFLAG_REALCLIPDIST     = 0x01000000,
    SFLAG_WAKEUPBADGUYS    = 0x02000000,
    SFLAG_DAMAGEEVENT      = 0x04000000,
    SFLAG_NOWATERSECTOR    = 0x08000000,
    SFLAG_QUEUEDFORDELETE  = 0x10000000,
    SFLAG_BADGUY_TILE      = 0x20000000,	// these are from RedNukem and had to be renumbered.
    SFLAG_KILLCOUNT        = 0x40000000,
    SFLAG_NOCANSEECHECK    = 0x80000000,
};

// Custom projectiles "workslike" flags.
// XXX: Currently not predefined from CON.
enum pflags_t
{
    PROJECTILE_HITSCAN           = 0x00000001,
    PROJECTILE_RPG               = 0x00000002,
    PROJECTILE_BOUNCESOFFWALLS   = 0x00000004,
    PROJECTILE_BOUNCESOFFMIRRORS = 0x00000008,
    PROJECTILE_KNEE              = 0x00000010,
    PROJECTILE_WATERBUBBLES      = 0x00000020,
    PROJECTILE_TIMED             = 0x00000040,
    PROJECTILE_BOUNCESOFFSPRITES = 0x00000080,
    PROJECTILE_SPIT              = 0x00000100,
    PROJECTILE_COOLEXPLOSION1    = 0x00000200,
    PROJECTILE_BLOOD             = 0x00000400,
    PROJECTILE_LOSESVELOCITY     = 0x00000800,
    PROJECTILE_NOAIM             = 0x00001000,
    PROJECTILE_RANDDECALSIZE     = 0x00002000,
    PROJECTILE_EXPLODEONTIMER    = 0x00004000,
    PROJECTILE_RPG_IMPACT        = 0x00008000,
    PROJECTILE_RADIUS_PICNUM     = 0x00010000,
    PROJECTILE_ACCURATE_AUTOAIM  = 0x00020000,
    PROJECTILE_FORCEIMPACT       = 0x00040000,
    PROJECTILE_REALCLIPDIST      = 0x00080000,
    PROJECTILE_ACCURATE          = 0x00100000,
    PROJECTILE_NOSETOWNERSHADE   = 0x00200000,
    PROJECTILE_RPG_IMPACT_DAMAGE = 0x00400000,
    PROJECTILE_MOVED             = 0x80000000,  // internal flag, do not document
    PROJECTILE_TYPE_MASK         = PROJECTILE_HITSCAN | PROJECTILE_RPG | PROJECTILE_KNEE | PROJECTILE_BLOOD,
};


END_DUKE_NS

#endif

