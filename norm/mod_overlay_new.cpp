#include "stdafx.h"

#include "mod_overlay_new.h"
#include "norm.h"

overlay_new::overlay_new(norm_dll::norm* c_state, std::shared_ptr<graphics> g)
    : mod(c_state)
    , g(g)
#include "hook_renderer.h"
#include "hook_session.h"
#include "norm.h"

#define D3DCOLOR_ARGB(a, r, g, b) \
    ((D3DCOLOR)((((a)&0xff) << 24) | (((r)&0xff) << 16) | (((g)&0xff) << 8) | ((b)&0xff)))

overlay_new::overlay_new(norm_dll::norm* c_state, json* config)
    : mod(c_state)
{
    if (config) {
        /* renderer is not yet available.*/
        this->fps_conf = static_cast<int>(config->at("fps_default_on").get<BOOL>());
        this->display_ping = static_cast<int>(config->at("ping_default_on").get<BOOL>());
    }
}

overlay_new::~overlay_new()
{
}

HRESULT overlay_new::end_scene(IDirect3DDevice7** d3ddevice)
{
	//c_state->dbg_sock->do_send("overlay_new: end_scene");
	if (display_ping) {
		char ping_str[32];
        ULONG ping = p_session.get_average_ping_time();
		if (ping == 0)
			sprintf_s(ping_str, "Ping: wait...");
		else
			sprintf_s(ping_str, "Ping: %ld ms", ping);
        g->print_screen(ping_str, this->x, this->y);
	}

	if (display_fps) {
		char fps_str[32];
		sprintf_s(fps_str, "FPS: %d", renderer_get_fps());
		g->print_screen(fps_str, this->x, this->y + 20);
    }

	return S_OK;
}

int overlay_new::get_talk_type(char* src, int* retval)
{
	if (strcmp(src, "/ping") == 0) {
		this->display_ping ^= 1;
		char buf[64];
		if (this->display_ping)
			sprintf_s(buf, "Ping is now displayed.");
		else
			sprintf_s(buf, "Ping is no longer displayed.");
		this->print_to_chat(buf);
		*retval = -1;
		return 1;
	}

	if (strcmp(src, "/fps") == 0) {
		this->display_fps ^= 1;
		char buf[64];
		if (this->display_fps)
			sprintf_s(buf, "FPS is now displayed.");
		else
			sprintf_s(buf, "FPS is no longer displayed.");
		this->print_to_chat(buf);
		*retval = -1;
		return 1;
	}
	return 0;
  
static HRESULT CALLBACK TextureSearchCallback(DDPIXELFORMAT* pddpf,
    VOID* param)
{
    if (pddpf->dwFlags & (DDPF_LUMINANCE | DDPF_BUMPLUMINANCE | DDPF_BUMPDUDV))
        return DDENUMRET_OK;
    if (pddpf->dwFourCC != 0)
        return DDENUMRET_OK;
    if (pddpf->dwRGBBitCount != 16)
        return DDENUMRET_OK;
    if (!(pddpf->dwFlags & DDPF_ALPHAPIXELS))
        return DDENUMRET_OK;
    // get 16 bit with alphapixel format
    memcpy((DDPIXELFORMAT*)param, pddpf, sizeof(DDPIXELFORMAT));
    return DDENUMRET_CANCEL;
}

void overlay_new::init(IDirect3DDevice7* d3ddevice)
{
    if (initialized != 0)
        return;
    D3DDEVICEDESC7 ddDesc;
    d3ddevice->GetCaps(&ddDesc);

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.dwWidth = 512;
    ddsd.dwHeight = 512;

    if (ddDesc.deviceGUID == IID_IDirect3DHALDevice) {
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    } else if (ddDesc.deviceGUID == IID_IDirect3DTnLHalDevice) {
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    } else {
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    }

    d3ddevice->EnumTextureFormats(TextureSearchCallback, &ddsd.ddpfPixelFormat);
    if (ddsd.ddpfPixelFormat.dwRGBBitCount) {
        LPDIRECTDRAWSURFACE7 pddsRender = NULL;
        LPDIRECTDRAW7 pDD = NULL;

        d3ddevice->GetRenderTarget(&pddsRender);
        if (pddsRender) {
            pddsRender->GetDDInterface((VOID**)&pDD);
            pddsRender->Release();
        }
        if (pDD) {
            if (SUCCEEDED(pDD->CreateSurface(&ddsd, &m_pddsFontTexture, NULL))) {
                c_state->dbg_sock->do_send("created font texture");
            } else {
                c_state->dbg_sock->do_send("failed create a font texture");
            }
            pDD->Release();
        }
        if (m_pddsFontTexture) {
            LOGFONT logfont;
            logfont.lfHeight = -12;
            logfont.lfWidth = 0;
            logfont.lfEscapement = 0;
            logfont.lfOrientation = 0;
            logfont.lfWeight = FW_REGULAR;
            //
            logfont.lfItalic = FALSE;
            logfont.lfUnderline = FALSE;
            logfont.lfStrikeOut = FALSE;
            logfont.lfCharSet = DEFAULT_CHARSET;
            logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
            logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            logfont.lfQuality = NONANTIALIASED_QUALITY;
            logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
            wcscpy_s(logfont.lfFaceName, L"‚l‚r ‚oƒSƒVƒbƒN");

            m_pSFastFont = new CSFastFont;
            m_pSFastFont->CreateFastFont(&logfont, d3ddevice, m_pddsFontTexture, 0);
            initialized++;
        }
    }
}

HRESULT overlay_new::begin_scene(IDirect3DDevice7** instance)
{
    //c_state->dbg_sock->do_send("overlay_new: begin_scene");
    this->init(*instance);
    return S_OK;
}

HRESULT overlay_new::end_scene(IDirect3DDevice7** d3ddevice)
{
    //c_state->dbg_sock->do_send("overlay_new: end_scene");
    if (!m_pSFastFont || !m_pddsFontTexture)
        return E_ABORT;

    if (display_ping) {
        char ping_str[32];
        DWORD ping = session_get_averagePingTime();
        if (ping == 0)
            sprintf_s(ping_str, "Ping: wait...");
        else
            sprintf_s(ping_str, "Ping: %ld ms", ping);

        m_pSFastFont->DrawText(ping_str, this->x, this->y, D3DCOLOR_ARGB(255, 255, 255, 0), 0, NULL);
    }

    if (display_fps) {
        char fps_str[32];
        sprintf_s(fps_str, "FPS: %d", renderer_get_fps());
        m_pSFastFont->DrawText(fps_str, this->x, this->y + 20, D3DCOLOR_ARGB(255, 255, 255, 0), 0, NULL);
    }

    if (display_ping || display_fps)
        m_pSFastFont->Flush();
    return S_OK;
}

void overlay_new::get_current_setting(json& setting)
{
    setting = {
		{ "ping_default_on", static_cast<bool>(this->display_ping)}, 
		{ "fps_default_on", static_cast<bool>(this->display_fps)}
	};
}

#if ((CLIENT_VER <= 20180919 && CLIENT_VER >= 20180620) || CLIENT_VER_RE == 20180621)
int overlay_new::get_talk_type(void** this_obj, void** src, int* a1, int* a2, int* retval)
#elif CLIENT_VER == 20150000
int overlay_new::get_talk_type(void** this_obj, char** src, int* a1, char** a2, int* retval)
#endif
{
    if (strcmp((char*)*src, "/ping") == 0) {
        this->display_ping ^= 1;
        char buf[64];
        if (this->display_ping)
            sprintf_s(buf, "Ping is now displayed.");
        else
            sprintf_s(buf, "Ping is no longer displayed.");
        this->print_to_chat(buf);
        *retval = -1;
        return 1;
    }

    if (strcmp((char*)*src, "/fps") == 0) {
        this->display_fps ^= 1;
        char buf[64];
        if (this->display_fps)
            sprintf_s(buf, "FPS is now displayed.");
        else
            sprintf_s(buf, "FPS is no longer displayed.");
        this->print_to_chat(buf);
        *retval = -1;
        return 1;
    }
    return 0;
}

// we get the screen dimension when we are ingame.
void overlay_new::draw_scene(void* this_obj)
{
	if (this->x == -1 && this->y == -1) {
		c_state->dbg_sock->do_send("Trying to get screen size!");
        //ProxyRenderer::instance->get_height();
		int screen_width = (int)renderer_get_width();
		int screen_height = (int)renderer_get_height();
		this->x = (int)(screen_width - 145);
		this->y = (int)(screen_height - (screen_height - 160));
		char buf[32];
		sprintf_s(buf, "Width: %d | Height: %d", screen_width, screen_height);
		c_state->dbg_sock->do_send(buf);
	}
}
    if (this->x == -1 && this->y == -1) {
        c_state->dbg_sock->do_send("Trying to get screen size!");
        int screen_width = (int)renderer_get_width();
        int screen_height = (int)renderer_get_height();
        this->x = (int)(screen_width - 145);
        this->y = (int)(screen_height - (screen_height - 160));
        char buf[32];
        sprintf_s(buf, "Width: %d | Height: %d", screen_width, screen_height);
        c_state->dbg_sock->do_send(buf);
        /* renderer should now be available */
        this->display_fps = this->fps_conf;
    }
}

void overlay_new::ddraw_release()
{
    if (m_pSFastFont)
        delete m_pSFastFont;
    if (m_pddsFontTexture) {
        m_pddsFontTexture->Release();
        m_pddsFontTexture = NULL;
    }
}
