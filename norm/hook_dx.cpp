#include "stdafx.h"

#include "norm.h"
#include "hook_dx.h"
#include "detours.h"
#include "client_ver.h"
#include "CProxyIDirectDraw7.h"

static std::shared_ptr<norm_dll::norm> c_state;
CProxyIDirectDraw7 *lpcDD;


HRESULT(WINAPI *pDirectDrawCreateEx)(GUID *lpGuid, LPVOID *lplpDD, const IID &iid, IUnknown *pUnkOuter) = DirectDrawCreateEx;

HRESULT WINAPI DirectDrawCreateEx_hook(GUID *lpGuid, LPVOID *lplpDD, const IID &iid, IUnknown *pUnkOuter)
{
	c_state->dbg_sock->do_send("DirectDrawCreateEx called!");

	HRESULT Result = pDirectDrawCreateEx(lpGuid, lplpDD, iid, pUnkOuter);
	if (FAILED(Result))
		return Result;

	*lplpDD = lpcDD = new CProxyIDirectDraw7((IDirectDraw7*)*lplpDD, c_state);
	lpcDD->setThis(lpcDD);
	c_state->dbg_sock->do_send("DirectDrawCreateEx called! Fin");
    
	return Result;
}

int dx_detour(std::shared_ptr<norm_dll::norm> state_) {
	int err = 0;
	int hook_count = 0;
	char info_buf[256];
	c_state = state_;

	err = DetourAttach(&(LPVOID&)pDirectDrawCreateEx, DirectDrawCreateEx_hook);
	CHECK(info_buf, err);
    if (err == NO_ERROR) {
        hook_count++;
     } else
        c_state->dbg_sock->do_send(info_buf);

	sprintf_s(info_buf, "DX hooks available: %d", hook_count);
	c_state->dbg_sock->do_send(info_buf);

	return hook_count;
}

