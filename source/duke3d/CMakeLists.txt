
set( NOT_COMPILED_SOURCE_FILES
	src/gamestructures.cpp
)

set( PCH_SOURCES
	src/actors.cpp
	src/anim.cpp
	src/cheats.cpp
	src/cmdline.cpp
	src/common.cpp
	src/config.cpp
	src/demo.cpp
	src/game.cpp
	src/gamedef.cpp
	src/gameexec.cpp
	src/gamevars.cpp
	src/global.cpp
	src/network.cpp
	src/osdcmds.cpp
	src/player.cpp
	src/premap.cpp
	src/savegame.cpp
	src/sbar.cpp
	src/screens.cpp
	src/text.cpp
	src/sector.cpp
	src/sounds.cpp
	src/d_menu.cpp
)

add_game_library( duke3d )
