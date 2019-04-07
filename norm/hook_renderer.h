#pragma once
#include "stdafx.h"
#include "client_ver.h"
#include "debug_socket.h"
#include "norm.h"
#include <Windows.h>
#include "mod.h"

/* contains all hooks related to drawing and rendering */

//int register_DrawScene_hook(std::shared_ptr<norm_dll::mod> mod_ptr);

/* hooks all available functions. */
int renderer_detour(std::shared_ptr<norm_dll::norm> c_state);
DWORD renderer_get_addr();
ULONG renderer_get_width();
ULONG renderer_get_height();
int renderer_get_fps();


#if (CLIENT_VER == 20180620 || CLIENT_VER == 20180621 || CLIENT_VER == 20150000 || CLIENT_VER_RE == 20180621)
typedef int(__thiscall *CRenderer__TextOutScreen)(void*, int, int, LPCSTR, COLORREF);
typedef bool(__fastcall *CRenderer__DrawScene)(void*);
#endif

//DWORD get_DrawScene_Addr();
//DWORD get_TextOutScreen_Addr();

