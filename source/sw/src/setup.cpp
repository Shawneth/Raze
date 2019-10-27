//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "build.h"

#include "keys.h"
#include "game.h"

#include "mytypes.h"
#include "fx_man.h"
#include "music.h"
#include "scriplib.h"
#include "gamedefs.h"
#include "keyboard.h"

#include "control.h"
#include "config.h"
#include "sounds.h"
#include "gamecontrol.h"

#include "rts.h"

BEGIN_SW_NS

void CenterCenter(void)
{
    printf("\nCenter the joystick and press a button\n");
}

void UpperLeft(void)
{
    printf("Move joystick to upper-left corner and press a button\n");
}

void LowerRight(void)
{
    printf("Move joystick to lower-right corner and press a button\n");
}

void CenterThrottle(void)
{
    printf("Center the throttle control and press a button\n");
}

void CenterRudder(void)
{
    printf("Center the rudder control and press a button\n");
}

/*
===================
=
= GetTime
=
===================
*/

static int32_t timert;

int32_t GetTime(void)
{
    return (int32_t) totalclock;
    //return timert++;
}

void InitSetup(void)
{
    int i;
    //RegisterShutdownFunction( ShutDown );

    //StartWindows();
    //initkeys();
    //CONFIG_GetSetupFilename();
    //InitializeKeyDefList();
    //CONFIG_ReadSetup();
    CONFIG_SetupMouse();
    CONFIG_SetupJoystick();

    CONTROL_JoystickEnabled = (UseJoystick && CONTROL_JoyPresent);
    CONTROL_MouseEnabled = (UseMouse && CONTROL_MousePresent);

    /*{
    int i;
    CONTROL_PrintKeyMap();
    for(i=0;i<NUMGAMEFUNCTIONS;i++) CONTROL_PrintControlFlag(i);
    CONTROL_PrintAxes();
    }*/

}

#if 0
void TermSetup(void)
{
    //FreeKeyDefList();
}
#endif

// BELOW IS FROM A TEST SETUP BY MARK DOC
//******************************************************************************
//******************************************************************************
//******************************************************************************
//******************************************************************************

#if 0
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "sndcards.h"
#include "fx_man.h"
#include "music.h"
#include "scriplib.h"
#include "gamedefs.h"
#include "keyboard.h"

#include "control.h"
#include "config.h"
#include "sounds.h"
#include "gamecontrol.h"
#include "rts.h"
#include "timer.h"

int32_t timerhandle=0;
volatile int32_t timer;
/*
===================
=
= Shutdown
=
===================
*/

void ShutDown(void)
{
    KB_Shutdown();
    TIME_RemoveTimer(timerhandle);
    SoundShutdown();
    MusicShutdown();
    CONFIG_WriteSetup();
}

/*
===================
=
= GetTime
=
===================
*/

int32_t GetTime(void)
{
    return timer;
}

/*
===================
=
= CenterCenter
=
===================
*/

void CenterCenter(void)
{
    printf("Center the joystick and press a button\n");
}

/*
===================
=
= UpperLeft
=
===================
*/

void UpperLeft(void)
{
    printf("Move joystick to upper-left corner and press a button\n");
}

/*
===================
=
= LowerRight
=
===================
*/

void LowerRight(void)
{
    printf("Move joystick to lower-right corner and press a button\n");
}

/*
===================
=
= CenterThrottle
=
===================
*/

void CenterThrottle(void)
{
    printf("Center the throttle control and press a button\n");
}

/*
===================
=
= CenterRudder
=
===================
*/

void CenterRudder(void)
{
    printf("Center the rudder control and press a button\n");
}

void main()
{
    char *song;
    char *voc;
    volatile int32_t lasttime;

    RegisterShutdownFunction(ShutDown);
    KB_Startup();
    timerhandle = TIME_AddTimer(40, &timer);
    //CONFIG_GetSetupFilename();
    CONFIG_ReadSetup();

    SoundStartup();
    MusicStartup();

    // load in some test data

    LoadFile("test.mid",&song);
    LoadFile("test.voc",&voc);

    // start playing a song

    MUSIC_PlaySong(song, MUSIC_LoopSong);


    lasttime = timer;
    while (1)
    {
        int32_t i;
        ControlInfo info;

        while (lasttime==timer)
        {
            ServiceEvents();
        }
        lasttime = timer;
//      printf("timer=%ld\n",timer);
        CONTROL_GetInput(&info);

        if (
            info.dx!=0 ||
            info.dy!=0 ||
            info.dz!=0 ||
            info.dpitch!=0 ||
            info.dyaw!=0 ||
            info.droll!=0
            )
            printf("x=%6ld y=%6ld z=%6ld yaw=%6ld pitch=%6ld roll=%6ld\n",
                   info.dx,info.dy,info.dz,info.dyaw,info.dpitch,info.droll);
        // Get Keyboard input and set appropiate game function states
        for (i=0; i<10; i++)
        {
            if (KB_KeyPressed(sc_F1+i))
            {
                uint8_t *ptr;
                KB_ClearKeyDown(sc_F1+i);
                ptr = (uint8_t *)RTS_GetSound(i);
                FX_PlayVOC(ptr, 0, 255, 255, 255, 255, 0);
            }
        }
        // Check to see if fire is being pressed so we can play a sound
        if (BUTTON(gamefunc_Fire) && !BUTTONHELD(gamefunc_Fire))
        {
            FX_PlayVOC(voc, 0, 255, 255, 255, 255, 0);
        }

        // Check to see if we want to exit
        if (KB_KeyPressed(sc_Escape))
        {
            break;
        }

    }

    ShutDown();
}
#endif
END_SW_NS
