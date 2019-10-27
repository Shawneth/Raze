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
#include "ns.h"	// Must come before everything else!

#include "cheats.h"
#include "cmdline.h"
#include "demo.h"  // g_firstDemoFile[]
#include "duke3d.h"
#include "menus.h"
#include "osdcmds.h"
#include "osdfuncs.h"
#include "savegame.h"
#include "sbar.h"

BEGIN_RR_NS

struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;

static inline int osdcmd_quit(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    OSD_ShowDisplay(0);
    G_GameQuit();
    return OSDCMD_OK;
}

static int osdcmd_changelevel(osdcmdptr_t parm)
{
    int32_t volume=0,level;
    char *p;

    if (!VOLUMEONE)
    {
        if (parm->numparms != 2) return OSDCMD_SHOWHELP;

        volume = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
        level = strtol(parm->parms[1], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }
    else
    {
        if (parm->numparms != 1) return OSDCMD_SHOWHELP;

        level = strtol(parm->parms[0], &p, 10) - 1;
        if (p[0]) return OSDCMD_SHOWHELP;
    }

    if (volume < 0) return OSDCMD_SHOWHELP;
    if (level < 0) return OSDCMD_SHOWHELP;

    if (!VOLUMEONE)
    {
        if (volume > g_volumeCnt)
        {
            OSD_Printf("changelevel: invalid volume number (range 1-%d)\n",g_volumeCnt);
            return OSDCMD_OK;
        }
    }

    if (level > MAXLEVELS || g_mapInfo[volume *MAXLEVELS+level].filename == NULL)
    {
        OSD_Printf("changelevel: invalid level number\n");
        return OSDCMD_SHOWHELP;
    }

    if (numplayers > 1)
    {
        /*
        if (g_netServer)
            Net_NewGame(volume,level);
        else if (voting == -1)
        {
            ud.m_volume_number = volume;
            ud.m_level_number = level;

            if (g_player[myconnectindex].ps->i)
            {
                int32_t i;

                for (i=0; i<MAXPLAYERS; i++)
                {
                    g_player[i].vote = 0;
                    g_player[i].gotvote = 0;
                }

                g_player[myconnectindex].vote = g_player[myconnectindex].gotvote = 1;

                voting = myconnectindex;

                tempbuf[0] = PACKET_MAP_VOTE_INITIATE;
                tempbuf[1] = myconnectindex;
                tempbuf[2] = ud.m_volume_number;
                tempbuf[3] = ud.m_level_number;

                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 4, ENET_PACKET_FLAG_RELIABLE));
            }
            if ((g_gametypeFlags[ud.m_coop] & GAMETYPE_PLAYERSFRIENDLY) && !(g_gametypeFlags[ud.m_coop] & GAMETYPE_TDM))
                ud.m_noexits = 0;

            M_OpenMenu(myconnectindex);
            Menu_Change(MENU_NETWAITVOTES);
        }
        */
        return OSDCMD_OK;
    }
    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        // in-game behave like a cheat
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_SCOTTY;
        osdcmd_cheatsinfo_stat.volume   = volume;
        osdcmd_cheatsinfo_stat.level    = level;
    }
    else
    {
        // out-of-game behave like a menu command
        osdcmd_cheatsinfo_stat.cheatnum = -1;

        ud.m_volume_number     = volume;
        ud.m_level_number      = level;

        ud.m_monsters_off      = 0;
        ud.monsters_off        = 0;

        ud.m_respawn_items     = 0;
        ud.m_respawn_inventory = 0;

        ud.multimode           = 1;

        G_NewGame_EnterLevel();
    }

    return OSDCMD_OK;
}

static int osdcmd_map(osdcmdptr_t parm)
{
    char filename[BMAX_PATH];

    const int32_t wildcardp = parm->numparms==1 &&
        (Bstrchr(parm->parms[0], '*') != NULL);

    if (parm->numparms != 1 || wildcardp)
    {
        CACHE1D_FIND_REC *r;
        fnlist_t fnlist = FNLIST_INITIALIZER;
        int32_t maxwidth = 0;

        if (wildcardp)
            maybe_append_ext(filename, sizeof(filename), parm->parms[0], ".map");
        else
            Bstrcpy(filename, "*.MAP");

        fnlist_getnames(&fnlist, "/", filename, -1, 0);

        for (r=fnlist.findfiles; r; r=r->next)
            maxwidth = max<int>(maxwidth, Bstrlen(r->name));

        if (maxwidth > 0)
        {
            int32_t x = 0;
            maxwidth += 3;
            OSD_Printf(OSDTEXT_RED "Map listing:\n");
            for (r=fnlist.findfiles; r; r=r->next)
            {
                OSD_Printf("%-*s",maxwidth,r->name);
                x += maxwidth;
                if (x > OSD_GetCols() - maxwidth)
                {
                    x = 0;
                    OSD_Printf("\n");
                }
            }
            if (x) OSD_Printf("\n");
            OSD_Printf(OSDTEXT_RED "Found %d maps\n", fnlist.numfiles);
        }

        fnlist_clearnames(&fnlist);

        return OSDCMD_SHOWHELP;
    }

    maybe_append_ext(filename, sizeof(filename), parm->parms[0], ".map");

    if (!testkopen(filename,0))
    {
        OSD_Printf(OSD_ERROR "map: file \"%s\" not found.\n", filename);
        return OSDCMD_OK;
    }

    boardfilename[0] = '/';
    boardfilename[1] = 0;
    strcat(boardfilename, filename);

    if (numplayers > 1)
    {
        /*
        if (g_netServer)
        {
            Net_SendUserMapName();
            ud.m_volume_number = 0;
            ud.m_level_number = 7;
            Net_NewGame(ud.m_volume_number, ud.m_level_number);
        }
        else if (voting == -1)
        {
            Net_SendUserMapName();

            ud.m_volume_number = 0;
            ud.m_level_number = 7;

            if (g_player[myconnectindex].ps->i)
            {
                int32_t i;

                for (i=0; i<MAXPLAYERS; i++)
                {
                    g_player[i].vote = 0;
                    g_player[i].gotvote = 0;
                }

                g_player[myconnectindex].vote = g_player[myconnectindex].gotvote = 1;
                voting = myconnectindex;

                tempbuf[0] = PACKET_MAP_VOTE_INITIATE;
                tempbuf[1] = myconnectindex;
                tempbuf[2] = ud.m_volume_number;
                tempbuf[3] = ud.m_level_number;

                enet_peer_send(g_netClientPeer, CHAN_GAMESTATE, enet_packet_create(tempbuf, 4, ENET_PACKET_FLAG_RELIABLE));
            }
            if ((g_gametypeFlags[ud.m_coop] & GAMETYPE_PLAYERSFRIENDLY) && !(g_gametypeFlags[ud.m_coop] & GAMETYPE_TDM))
                ud.m_noexits = 0;

            M_OpenMenu(myconnectindex);
            Menu_Change(MENU_NETWAITVOTES);
        }
        */
        return OSDCMD_OK;
    }

    osdcmd_cheatsinfo_stat.cheatnum = -1;
    ud.m_volume_number = 0;
    ud.m_level_number = 7;

    ud.m_monsters_off = ud.monsters_off = 0;

    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;

    ud.multimode = 1;

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        G_NewGame(ud.m_volume_number, ud.m_level_number, ud.m_player_skill);
        g_player[myconnectindex].ps->gm = MODE_RESTART;
    }
    else G_NewGame_EnterLevel();

    return OSDCMD_OK;
}

// demo <demonum or demofn> [<prof>]
//
// To profile a demo ("timedemo mode"), <prof> can be given in the range 0-8,
// which will start to replay it as fast as possible, rendering <prof> frames
// for each gametic.
//
// Notes:
//  * The demos should be recorded with demorec_diffs set to 0, so that the
//    game state updates are actually computed.
//  * Currently, the profiling can only be aborted on SDL 1.2 builds by
//    pressing any key.
//  * With <prof> greater than 1, interpolation should be calculated properly,
//    though this has not been verified by looking at the frames.
//  * When testing whether a change in the source has an effect on performance,
//    the variance of the run times MUST be taken into account (that is, the
//    replaying must be performed multiple times for the old and new versions,
//    etc.)
static int osdcmd_demo(osdcmdptr_t parm)
{
    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    if (g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        OSD_Printf("demo: Must not be in a game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1 && parm->numparms != 2)
        return OSDCMD_SHOWHELP;

    {
        int32_t prof = parm->numparms==2 ? Batoi(parm->parms[1]) : -1;

        Demo_SetFirst(parm->parms[0]);
        Demo_PlayFirst(clamp(prof, -1, 8)+1, 0);
    }

    return OSDCMD_OK;
}

static int osdcmd_activatecheat(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
        osdcmd_cheatsinfo_stat.cheatnum = Batoi(parm->parms[0]);
    else
        OSD_Printf("activatecheat: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_god(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_CORNHOLIO;
    else
        OSD_Printf("god: Not in a single-player game.\n");

    return OSDCMD_OK;
}

static int osdcmd_noclip(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (numplayers == 1 && g_player[myconnectindex].ps->gm & MODE_GAME)
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_CLIP;
    }
    else
    {
        OSD_Printf("noclip: Not in a single-player game.\n");
    }

    return OSDCMD_OK;
}

static int osdcmd_restartsound(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    S_SoundShutdown();
    S_MusicShutdown();

    S_MusicStartup();
    S_SoundStartup();

    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (mus_enabled)
        S_RestartMusic();

    return OSDCMD_OK;
}

static int osdcmd_music(osdcmdptr_t parm)
{
    if (parm->numparms == 1)
    {
        int32_t sel = G_GetMusicIdx(parm->parms[0]);

        if (sel == -1)
            return OSDCMD_SHOWHELP;

        if (sel == -2)
        {
            OSD_Printf("%s is not a valid episode/level number pair\n", parm->parms[0]);
            return OSDCMD_OK;
        }

        if (!S_TryPlayLevelMusic(sel))
        {
            G_PrintCurrentMusic();
        }
        else
        {
            OSD_Printf("No music defined for %s\n", parm->parms[0]);
        }

        return OSDCMD_OK;
    }

    return OSDCMD_SHOWHELP;
}

int osdcmd_restartvid(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    videoResetMode();
    if (videoSetGameMode(ScreenMode,ScreenWidth,ScreenHeight,ScreenBPP,ud.detail))
        G_GameExit("restartvid: Reset failed...\n");
    onvideomodechange(ScreenBPP>8);
    G_UpdateScreenArea();

    return OSDCMD_OK;
}

int osdcmd_restartmap(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    if (g_player[myconnectindex].ps->gm & MODE_GAME && ud.multimode == 1)
        g_player[myconnectindex].ps->gm = MODE_RESTART;

    return OSDCMD_OK;
}

static int osdcmd_vidmode(osdcmdptr_t parm)
{
    int32_t newbpp = ScreenBPP, newwidth = ScreenWidth,
            newheight = ScreenHeight, newfs = ScreenMode;
    int32_t tmp;

    if (parm->numparms < 1 || parm->numparms > 4) return OSDCMD_SHOWHELP;

    switch (parm->numparms)
    {
    case 1: // bpp switch
        tmp = Batol(parm->parms[0]);
        if (!(tmp==8 || tmp==16 || tmp==32))
            return OSDCMD_SHOWHELP;
        newbpp = tmp;
        break;
    case 2: // res switch
        newwidth = Batol(parm->parms[0]);
        newheight = Batol(parm->parms[1]);
        break;
    case 3: // res & bpp switch
    case 4:
        newwidth = Batol(parm->parms[0]);
        newheight = Batol(parm->parms[1]);
        tmp = Batol(parm->parms[2]);
        if (!(tmp==8 || tmp==16 || tmp==32))
            return OSDCMD_SHOWHELP;
        newbpp = tmp;
        if (parm->numparms == 4)
            newfs = (Batol(parm->parms[3]) != 0);
        break;
    }

    if (videoSetGameMode(newfs,newwidth,newheight,newbpp,upscalefactor))
    {
        initprintf("vidmode: Mode change failed!\n");
        if (videoSetGameMode(ScreenMode, ScreenWidth, ScreenHeight, ScreenBPP, upscalefactor))
            G_GameExit("vidmode: Reset failed!\n");
    }
    ScreenBPP = newbpp;
    ScreenWidth = newwidth;
    ScreenHeight = newheight;
    ScreenMode = newfs;
    onvideomodechange(ScreenBPP>8);
    G_UpdateScreenArea();
    return OSDCMD_OK;
}

static int osdcmd_spawn(osdcmdptr_t parm)
{
    int32_t picnum = 0;
    uint16_t cstat=0;
    char pal=0;
    int16_t ang=0;
    int16_t set=0, idx;
    vec3_t vect;

    if (numplayers > 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME))
    {
        OSD_Printf("spawn: Can't spawn sprites in multiplayer games or demos\n");
        return OSDCMD_OK;
    }

    switch (parm->numparms)
    {
    case 7: // x,y,z
        vect.x = Batol(parm->parms[4]);
        vect.y = Batol(parm->parms[5]);
        vect.z = Batol(parm->parms[6]);
        set |= 8;
        fallthrough__;
    case 4: // ang
        ang = Batol(parm->parms[3]) & 2047;
        set |= 4;
        fallthrough__;
    case 3: // cstat
        cstat = (uint16_t)Batol(parm->parms[2]);
        set |= 2;
        fallthrough__;
    case 2: // pal
        pal = (uint8_t)Batol(parm->parms[1]);
        set |= 1;
        fallthrough__;
    case 1: // tile number
        if (isdigit(parm->parms[0][0]))
        {
            picnum = Batol(parm->parms[0]);
        }
        else
        {
            int32_t i;
            int32_t j;

            for (j=0; j<2; j++)
            {
                for (i=0; i<g_labelCnt; i++)
                {
                    if ((j == 0 && !Bstrcmp(label+(i<<6),     parm->parms[0])) ||
                        (j == 1 && !Bstrcasecmp(label+(i<<6), parm->parms[0])))
                    {
                        picnum = labelcode[i];
                        break;
                    }
                }

                if (i < g_labelCnt)
                    break;
            }
            if (i==g_labelCnt)
            {
                OSD_Printf("spawn: Invalid tile label given\n");
                return OSDCMD_OK;
            }
        }

        if ((uint32_t)picnum >= MAXUSERTILES)
        {
            OSD_Printf("spawn: Invalid tile number\n");
            return OSDCMD_OK;
        }
        break;

    default:
        return OSDCMD_SHOWHELP;
    }

    idx = A_Spawn(g_player[myconnectindex].ps->i, picnum);
    if (set & 1) sprite[idx].pal = (uint8_t)pal;
    if (set & 2) sprite[idx].cstat = (int16_t)cstat;
    if (set & 4) sprite[idx].ang = ang;
    if (set & 8)
    {
        if (setsprite(idx, &vect) < 0)
        {
            OSD_Printf("spawn: Sprite can't be spawned into null space\n");
            A_DeleteSprite(idx);
        }
    }

    return OSDCMD_OK;
}

static int osdcmd_initgroupfile(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    initgroupfile(parm->parms[0]);

    return OSDCMD_OK;
}

static int osdcmd_cmenu(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (numplayers > 1)
    {
        OSD_Printf("Command not allowed in multiplayer\n");
        return OSDCMD_OK;
    }

    if ((g_player[myconnectindex].ps->gm & MODE_MENU) != MODE_MENU)
        Menu_Open(myconnectindex);

    Menu_Change(Batol(parm->parms[0]));

    return OSDCMD_OK;
}




static int osdcmd_crosshaircolor(osdcmdptr_t parm)
{
    if (parm->numparms != 3)
    {
        OSD_Printf("crosshaircolor: r:%d g:%d b:%d\n",CrosshairColors.r,CrosshairColors.g,CrosshairColors.b);
        return OSDCMD_SHOWHELP;
    }

    uint8_t const r = Batol(parm->parms[0]);
    uint8_t const g = Batol(parm->parms[1]);
    uint8_t const b = Batol(parm->parms[2]);

    G_SetCrosshairColor(r,g,b);

    if (!OSD_ParsingScript())
        OSD_Printf("%s\n", parm->raw);

    return OSDCMD_OK;
}

static int osdcmd_give(osdcmdptr_t parm)
{
    int32_t i;

    if (numplayers != 1 || (g_player[myconnectindex].ps->gm & MODE_GAME) == 0 ||
            g_player[myconnectindex].ps->dead_flag != 0)
    {
        OSD_Printf("give: Cannot give while dead or not in a single-player game.\n");
        return OSDCMD_OK;
    }

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if (!Bstrcasecmp(parm->parms[0], "all"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_STUFF;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "health"))
    {
        sprite[g_player[myconnectindex].ps->i].extra = g_player[myconnectindex].ps->max_player_health<<1;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "weapons"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_WEAPONS;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "ammo"))
    {
        for (i=MAX_WEAPONS-(VOLUMEONE?6:1)-1; i>=PISTOL_WEAPON; i--)
            P_AddAmmo(g_player[myconnectindex].ps,i,g_player[myconnectindex].ps->max_ammo_amount[i]);
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "armor"))
    {
        g_player[myconnectindex].ps->inv_amount[GET_SHIELD] = 100;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "keys"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_KEYS;
        return OSDCMD_OK;
    }
    else if (!Bstrcasecmp(parm->parms[0], "inventory"))
    {
        osdcmd_cheatsinfo_stat.cheatnum = CHEAT_INVENTORY;
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

void onvideomodechange(int32_t newmode)
{
    uint8_t palid;

    // XXX?
    if (!newmode || g_player[screenpeek].ps->palette < BASEPALCOUNT)
        palid = g_player[screenpeek].ps->palette;
    else
        palid = BASEPAL;

#ifdef POLYMER
    if (videoGetRenderMode() == REND_POLYMER)
    {
        int32_t i = 0;

        while (i < MAXSPRITES)
        {
            if (actor[i].lightptr)
            {
                polymer_deletelight(actor[i].lightId);
                actor[i].lightptr = NULL;
                actor[i].lightId = -1;
            }
            i++;
        }
    }
#endif

    videoSetPalette(0, palid, 0);
    g_restorePalette = -1;
    g_crosshairSum = -1;
}


const char *const ConsoleButtons[] =
{
    "mouse1", "mouse2", "mouse3", "mouse4", "mwheelup",
    "mwheeldn", "mouse5", "mouse6", "mouse7", "mouse8"
};

static int osdcmd_bind(osdcmdptr_t parm)
{
    if (parm->numparms==1 && !Bstrcasecmp(parm->parms[0],"showkeys"))
    {
        for (int i=0; sctokeylut[i].key; i++)
            OSD_Printf("%s\n",sctokeylut[i].key);
        for (auto ConsoleButton : ConsoleButtons)
            OSD_Printf("%s\n",ConsoleButton);
        return OSDCMD_OK;
    }

    if (parm->numparms==0)
    {
        int j=0;

        OSD_Printf("Current key bindings:\n");

        for (int i=0; i<MAXBOUNDKEYS+MAXMOUSEBUTTONS; i++)
            if (CONTROL_KeyIsBound(i))
            {
                j++;
                OSD_Printf("%-9s %s\"%s\"\n", CONTROL_KeyBinds[i].key, CONTROL_KeyBinds[i].repeat?"":"norepeat ",
                           CONTROL_KeyBinds[i].cmdstr);
            }

        if (j == 0)
            OSD_Printf("No binds found.\n");

        return OSDCMD_OK;
    }

    int i, j, repeat;

    for (i=0; i < ARRAY_SSIZE(sctokeylut); i++)
    {
        if (!Bstrcasecmp(parm->parms[0], sctokeylut[i].key))
            break;
    }

    // didn't find the key
    if (i == ARRAY_SSIZE(sctokeylut))
    {
        for (i=0; i<MAXMOUSEBUTTONS; i++)
            if (!Bstrcasecmp(parm->parms[0],ConsoleButtons[i]))
                break;

        if (i >= MAXMOUSEBUTTONS)
            return OSDCMD_SHOWHELP;

        if (parm->numparms < 2)
        {
            if (CONTROL_KeyBinds[MAXBOUNDKEYS + i].cmdstr && CONTROL_KeyBinds[MAXBOUNDKEYS + i ].key)
                OSD_Printf("%-9s %s\"%s\"\n", ConsoleButtons[i], CONTROL_KeyBinds[MAXBOUNDKEYS + i].repeat?"":"norepeat ",
                CONTROL_KeyBinds[MAXBOUNDKEYS + i].cmdstr);
            else OSD_Printf("%s is unbound\n", ConsoleButtons[i]);
            return OSDCMD_OK;
        }

        j = 1;

        repeat = 1;
        if (!Bstrcasecmp(parm->parms[j],"norepeat"))
        {
            repeat = 0;
            j++;
        }

        Bstrcpy(tempbuf,parm->parms[j++]);
        for (; j<parm->numparms; j++)
        {
            Bstrcat(tempbuf," ");
            Bstrcat(tempbuf,parm->parms[j++]);
        }

        CONTROL_BindMouse(i, tempbuf, repeat, ConsoleButtons[i]);

        if (!OSD_ParsingScript())
            OSD_Printf("%s\n",parm->raw);
        return OSDCMD_OK;
    }

    if (parm->numparms < 2)
    {
        if (CONTROL_KeyIsBound(sctokeylut[i].sc))
            OSD_Printf("%-9s %s\"%s\"\n", sctokeylut[i].key, CONTROL_KeyBinds[sctokeylut[i].sc].repeat?"":"norepeat ",
                       CONTROL_KeyBinds[sctokeylut[i].sc].cmdstr);
        else OSD_Printf("%s is unbound\n", sctokeylut[i].key);

        return OSDCMD_OK;
    }

    j = 1;

    repeat = 1;
    if (!Bstrcasecmp(parm->parms[j],"norepeat"))
    {
        repeat = 0;
        j++;
    }

    Bstrcpy(tempbuf,parm->parms[j++]);
    for (; j<parm->numparms; j++)
    {
        Bstrcat(tempbuf," ");
        Bstrcat(tempbuf,parm->parms[j++]);
    }

    CONTROL_BindKey(sctokeylut[i].sc, tempbuf, repeat, sctokeylut[i].key);

    char *cp = tempbuf;

    // Populate the keyboard config menu based on the bind.
    // Take care of processing one-to-many bindings properly, too.
    static char const s_gamefunc_[] = "gamefunc_";
    int constexpr strlen_gamefunc_  = ARRAY_SIZE(s_gamefunc_) - 1;

    while ((cp = Bstrstr(cp, s_gamefunc_)))
    {
        cp += strlen_gamefunc_;

        char *semi = Bstrchr(cp, ';');

        if (semi)
            *semi = 0;

        j = CONFIG_FunctionNameToNum(cp);

        if (semi)
            cp = semi+1;

        if (j != -1)
        {
            KeyboardKeys[j][1] = KeyboardKeys[j][0];
            KeyboardKeys[j][0] = sctokeylut[i].sc;
//            CONTROL_MapKey(j, sctokeylut[i].sc, KeyboardKeys[j][0]);

            if (j == gamefunc_Show_Console)
                OSD_CaptureKey(sctokeylut[i].sc);
        }
    }

    if (!OSD_ParsingScript())
        OSD_Printf("%s\n",parm->raw);

    return OSDCMD_OK;
}

static int osdcmd_unbindall(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    for (int i = 0; i < MAXBOUNDKEYS; ++i)
        CONTROL_FreeKeyBind(i);

    for (int i = 0; i < MAXMOUSEBUTTONS; ++i)
        CONTROL_FreeMouseBind(i);

    for (auto &KeyboardKey : KeyboardKeys)
        KeyboardKey[0] = KeyboardKey[1] = 0xff;

    if (!OSD_ParsingScript())
        OSD_Printf("unbound all controls\n");

    return OSDCMD_OK;
}

static int osdcmd_unbind(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    for (auto ConsoleKey : sctokeylut)
    {
        if (ConsoleKey.key && !Bstrcasecmp(parm->parms[0], ConsoleKey.key))
        {
            CONTROL_FreeKeyBind(ConsoleKey.sc);
            OSD_Printf("unbound key %s\n", ConsoleKey.key);
            return OSDCMD_OK;
        }
    }

    for (int i = 0; i < MAXMOUSEBUTTONS; i++)
    {
        if (!Bstrcasecmp(parm->parms[0], ConsoleButtons[i]))
        {
            CONTROL_FreeMouseBind(i);
            OSD_Printf("unbound %s\n", ConsoleButtons[i]);
            return OSDCMD_OK;
        }
    }

    return OSDCMD_SHOWHELP;
}

static int osdcmd_quicksave(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_Printf("quicksave: not in a game.\n");
    else g_doQuickSave = 1;
    return OSDCMD_OK;
}

static int osdcmd_quickload(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    if (!(g_player[myconnectindex].ps->gm & MODE_GAME))
        OSD_Printf("quickload: not in a game.\n");
    else g_doQuickSave = 2;
    return OSDCMD_OK;
}

static int osdcmd_screenshot(osdcmdptr_t parm)
{
//    KB_ClearKeysDown();
    static const char *fn = "duke0000.png";

    if (parm->numparms == 1 && !Bstrcasecmp(parm->parms[0], "tga"))
        videoCaptureScreenTGA(fn, 0);
    else videoCaptureScreen(fn, 0);

    return OSDCMD_OK;
}

#if 0
static int osdcmd_savestate(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_PARAMETER(parm);
    G_SaveMapState();
    return OSDCMD_OK;
}

static int osdcmd_restorestate(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_PARAMETER(parm);
    G_RestoreMapState();
    return OSDCMD_OK;
}
#endif

#ifdef DEBUGGINGAIDS
static int osdcmd_inittimer(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("%dHz timer\n",g_timerTicsPerSecond);
        return OSDCMD_SHOWHELP;
    }

    G_InitTimer(Batol(parm->parms[0]));

    OSD_Printf("%s\n",parm->raw);
    return OSDCMD_OK;
}
#endif

#if !defined NETCODE_DISABLE
static int osdcmd_disconnect(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    // NUKE-TODO:
    if (g_player[myconnectindex].ps->gm&MODE_MENU)
        g_netDisconnect = 1;
    return OSDCMD_OK;
}

static int osdcmd_connect(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    Net_Connect(parm->parms[0]);
    G_BackToMenu();
    return OSDCMD_OK;
}

static int osdcmd_password(osdcmdptr_t parm)
{
    if (parm->numparms < 1)
    {
        Bmemset(g_netPassword, 0, sizeof(g_netPassword));
        return OSDCMD_OK;
    }
    Bstrncpy(g_netPassword, (parm->raw) + 9, sizeof(g_netPassword)-1);

    return OSDCMD_OK;
}

static int osdcmd_listplayers(osdcmdptr_t parm)
{
    ENetPeer *currentPeer;
    char ipaddr[32];

    if (parm && parm->numparms != 0)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    initprintf("Connected clients:\n");

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        enet_address_get_host_ip(&currentPeer->address, ipaddr, sizeof(ipaddr));
        initprintf("%x %s %s\n", currentPeer->address.host, ipaddr,
                   g_player[(intptr_t)currentPeer->data].user_name);
    }

    return OSDCMD_OK;
}

#if 0
static int osdcmd_kick(osdcmdptr_t parm)
{
    ENetPeer *currentPeer;
    uint32_t hexaddr;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        sscanf(parm->parms[0],"%" SCNx32 "", &hexaddr);

        if (currentPeer->address.host == hexaddr)
        {
            initprintf("Kicking %x (%s)\n", currentPeer->address.host,
                       g_player[(intptr_t)currentPeer->data].user_name);
            enet_peer_disconnect(currentPeer, DISC_KICKED);
            return OSDCMD_OK;
        }
    }

    initprintf("Player %s not found!\n", parm->parms[0]);
    osdcmd_listplayers(NULL);

    return OSDCMD_OK;
}

static int osdcmd_kickban(osdcmdptr_t parm)
{
    ENetPeer *currentPeer;
    uint32_t hexaddr;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (!g_netServer)
    {
        initprintf("You are not the server.\n");
        return OSDCMD_OK;
    }

    for (currentPeer = g_netServer -> peers;
            currentPeer < & g_netServer -> peers [g_netServer -> peerCount];
            ++ currentPeer)
    {
        if (currentPeer -> state != ENET_PEER_STATE_CONNECTED)
            continue;

        sscanf(parm->parms[0],"%" SCNx32 "", &hexaddr);

        // TODO: implement banning logic

        if (currentPeer->address.host == hexaddr)
        {
            char ipaddr[32];

            enet_address_get_host_ip(&currentPeer->address, ipaddr, sizeof(ipaddr));
            initprintf("Host %s is now banned.\n", ipaddr);
            initprintf("Kicking %x (%s)\n", currentPeer->address.host,
                       g_player[(intptr_t)currentPeer->data].user_name);
            enet_peer_disconnect(currentPeer, DISC_BANNED);
            return OSDCMD_OK;
        }
    }

    initprintf("Player %s not found!\n", parm->parms[0]);
    osdcmd_listplayers(NULL);

    return OSDCMD_OK;
}
#endif
#endif

static int osdcmd_purgesaves(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);
    G_DeleteOldSaves();
    return OSDCMD_OK;
}

static int osdcmd_printtimes(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    char buf[32];
    int32_t maxlen = 0;
    int32_t haveac=0;

    for (int i=0; i<MAXTILES; i++)
        if (g_actorCalls[i])
        {
            if (!haveac)
            {
                haveac = 1;
                OSD_Printf("\nactor times: tile, total calls, total time [ms], {min,mean,max} time/call [us]\n");
            }

            buf[0] = 0;

            for (int ii=0; ii<g_labelCnt; ii++)
            {
                if (labelcode[ii] == i && labeltype[ii] & LABEL_ACTOR)
                {
                    Bstrcpy(buf, label+(ii<<6));
                    break;
                }
            }

            if (!buf[0]) Bsprintf(buf, "%d", i);

            OSD_Printf("%17s, %8d, %9.3f, %9.3f, %9.3f, %9.3f,\n",
                buf, g_actorCalls[i], g_actorTotalMs[i],
                1000*g_actorMinMs[i],
                1000*g_actorTotalMs[i]/g_actorCalls[i],
                1000*g_actorMaxMs[i]);
        }

    return OSDCMD_OK;
}


int32_t registerosdcommands(void)
{

    if (VOLUMEONE)
        OSD_RegisterFunction("changelevel","changelevel <level>: warps to the given level", osdcmd_changelevel);
    else
    {
        OSD_RegisterFunction("changelevel","changelevel <volume> <level>: warps to the given level", osdcmd_changelevel);
        OSD_RegisterFunction("map","map <mapfile>: loads the given user map", osdcmd_map);
        OSD_RegisterFunction("demo","demo <demofile or demonum>: starts the given demo", osdcmd_demo);
    }

    OSD_RegisterFunction("bind",R"(bind <key> <string>: associates a keypress with a string of console input. Type "bind showkeys" for a list of keys and "listsymbols" for a list of valid console commands.)", osdcmd_bind);
    OSD_RegisterFunction("cmenu","cmenu <#>: jumps to menu", osdcmd_cmenu);
    OSD_RegisterFunction("crosshaircolor","crosshaircolor: changes the crosshair color", osdcmd_crosshaircolor);

#if !defined NETCODE_DISABLE
    OSD_RegisterFunction("connect","connect: connects to a multiplayer game", osdcmd_connect);
    OSD_RegisterFunction("disconnect","disconnect: disconnects from the local multiplayer game", osdcmd_disconnect);
#endif

    OSD_RegisterFunction("give","give <all|health|weapons|ammo|armor|keys|inventory>: gives requested item", osdcmd_give);
    OSD_RegisterFunction("god","god: toggles god mode", osdcmd_god);
    OSD_RegisterFunction("activatecheat","activatecheat <id>: activates a cheat code", osdcmd_activatecheat);

    OSD_RegisterFunction("initgroupfile","initgroupfile <path>: adds a grp file into the game filesystem", osdcmd_initgroupfile);
#ifdef DEBUGGINGAIDS
    OSD_RegisterFunction("inittimer","debug", osdcmd_inittimer);
#endif
#if !defined NETCODE_DISABLE
    //OSD_RegisterFunction("kick","kick <id>: kicks a multiplayer client.  See listplayers.", osdcmd_kick);
    //OSD_RegisterFunction("kickban","kickban <id>: kicks a multiplayer client and prevents them from reconnecting.  See listplayers.", osdcmd_kickban);

    OSD_RegisterFunction("listplayers","listplayers: lists currently connected multiplayer clients", osdcmd_listplayers);
#endif
    OSD_RegisterFunction("music","music E<ep>L<lev>: change music", osdcmd_music);

    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

#if !defined NETCODE_DISABLE
    OSD_RegisterFunction("password","password: sets multiplayer game password", osdcmd_password);
#endif

    OSD_RegisterFunction("printtimes", "printtimes: prints VM timing statistics", osdcmd_printtimes);

    OSD_RegisterFunction("purgesaves", "purgesaves: deletes obsolete and unreadable save files", osdcmd_purgesaves);

    OSD_RegisterFunction("quicksave","quicksave: performs a quick save", osdcmd_quicksave);
    OSD_RegisterFunction("quickload","quickload: performs a quick load", osdcmd_quickload);
    OSD_RegisterFunction("quit","quit: exits the game immediately", osdcmd_quit);
    OSD_RegisterFunction("exit","exit: exits the game immediately", osdcmd_quit);

    OSD_RegisterFunction("restartmap", "restartmap: restarts the current map", osdcmd_restartmap);
    OSD_RegisterFunction("restartsound","restartsound: reinitializes the sound system",osdcmd_restartsound);
    OSD_RegisterFunction("restartvid","restartvid: reinitializes the video mode",osdcmd_restartvid);

    OSD_RegisterFunction("screenshot","screenshot [format]: takes a screenshot.", osdcmd_screenshot);

    OSD_RegisterFunction("spawn","spawn <picnum> [palnum] [cstat] [ang] [x y z]: spawns a sprite with the given properties",osdcmd_spawn);

    OSD_RegisterFunction("unbind","unbind <key>: unbinds a key", osdcmd_unbind);
    OSD_RegisterFunction("unbindall","unbindall: unbinds all keys", osdcmd_unbindall);

    OSD_RegisterFunction("vidmode","vidmode <xdim> <ydim> <bpp> <fullscreen>: change the video mode",osdcmd_vidmode);
#ifdef USE_OPENGL
    baselayer_osdcmd_vidmode_func = osdcmd_vidmode;
#endif
    return 0;
}

END_RR_NS
