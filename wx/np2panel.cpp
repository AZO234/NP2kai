/* === PC-98 screen panel for wx port === */

#include <compiler.h>
#include "np2panel.h"
#include "np2frame.h"
#include "scrnmng.h"
#include "mousemng.h"
#include "kbtrans.h"
#include "inputmng.h"
#include "np2.h"

#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/image.h>

wxBEGIN_EVENT_TABLE(Np2Panel, wxPanel)
	EVT_PAINT(Np2Panel::OnPaint)
	EVT_SIZE(Np2Panel::OnSize)
	EVT_KEY_DOWN(Np2Panel::OnKeyDown)
	EVT_KEY_UP(Np2Panel::OnKeyUp)
	EVT_MOTION(Np2Panel::OnMouseMove)
	EVT_LEFT_DOWN(Np2Panel::OnMouseLeft)
	EVT_LEFT_UP(Np2Panel::OnMouseLeft)
	EVT_MIDDLE_DOWN(Np2Panel::OnMouseMiddle)
	EVT_RIGHT_DOWN(Np2Panel::OnMouseRight)
	EVT_RIGHT_UP(Np2Panel::OnMouseRight)
	EVT_MOUSE_CAPTURE_LOST(Np2Panel::OnMouseCaptureLost)
	EVT_SET_FOCUS(Np2Panel::OnSetFocus)
	EVT_KILL_FOCUS(Np2Panel::OnKillFocus)
wxEND_EVENT_TABLE()

Np2Panel::Np2Panel(wxWindow *parent)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
	          wxWANTS_CHARS | wxTAB_TRAVERSAL)
	, m_mouseCaptured(false)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(*wxBLACK);
}

Np2Panel::~Np2Panel()
{
	if (m_mouseCaptured) {
		wxPanel::ReleaseMouse();
		m_mouseCaptured = false;
	}
}

void Np2Panel::UpdateScreen(void)
{
	int w, h, bpp;
	scrnmng_lockbuf();
	const UINT8 *src = scrnmng_getpixbuf(&w, &h, &bpp);

	if (!src || w <= 0 || h <= 0) {
		scrnmng_unlockbuf();
		Refresh(false);
		return;
	}

	/* Build wxImage (always 24-bit RGB, platform-independent) */
	if (!m_image.IsOk() || m_image.GetWidth() != w || m_image.GetHeight() != h) {
		m_image.Create(w, h, false);
	}

	unsigned char *dst = m_image.GetData();

	if (bpp == 16) {
		/* RGB565 → RGB24 */
		const UINT16 *s16 = (const UINT16 *)src;
		unsigned char *d = dst;
		for (int y = 0; y < h; y++) {
			const UINT16 *row = s16 + (size_t)y * w;
			for (int x = 0; x < w; x++) {
				UINT16 pix = row[x];
				*d++ = (unsigned char)((pix >> 11) << 3);
				*d++ = (unsigned char)(((pix >> 5) & 0x3f) << 2);
				*d++ = (unsigned char)((pix & 0x1f) << 3);
			}
		}
	} else if (bpp == 32) {
		/* BGRA → RGB24 */
		const UINT8 *s32 = src;
		unsigned char *d = dst;
		for (int y = 0; y < h; y++) {
			const UINT8 *row = s32 + (size_t)y * w * 4;
			for (int x = 0; x < w; x++) {
				*d++ = row[x * 4 + 2];  /* R */
				*d++ = row[x * 4 + 1];  /* G */
				*d++ = row[x * 4 + 0];  /* B */
			}
		}
	}

	scrnmng_unlockbuf();

	/* Apply rotation based on scrnmode bits:
	 *   0x00 = normal
	 *   0x10 = SCRNMODE_ROTATELEFT  (90° CCW)
	 *   0x20 = 180°
	 *   0x30 = SCRNMODE_ROTATERIGHT (90° CW) */
	UINT8 rot = scrnmode & SCRNMODE_ROTATEMASK;
	if (rot == SCRNMODE_ROTATELEFT) {
		m_bitmap = wxBitmap(m_image.Rotate90(false));   /* 90° CCW */
	} else if (rot == SCRNMODE_ROTATERIGHT) {
		m_bitmap = wxBitmap(m_image.Rotate90(true));    /* 90° CW */
	} else if (rot == 0x20) {
		m_bitmap = wxBitmap(m_image.Rotate90(true).Rotate90(true)); /* 180° */
	} else {
		m_bitmap = wxBitmap(m_image);
	}

	Refresh(false);
}

wxBitmap Np2Panel::GetBitmap(void)
{
	return m_bitmap;
}

/* ---- paint ---- */

void Np2Panel::OnPaint(wxPaintEvent & /*evt*/)
{
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	if (!m_bitmap.IsOk()) {
		return;
	}
	int pw = GetClientSize().x;
	int ph = GetClientSize().y;
	if (pw > 0 && ph > 0) {
		int bw = m_bitmap.GetWidth();
		int bh = m_bitmap.GetHeight();
		if (pw == bw && ph == bh) {
			dc.DrawBitmap(m_bitmap, 0, 0);
		} else {
			wxMemoryDC memDC;
			memDC.SelectObject(m_bitmap);
			dc.StretchBlit(0, 0, pw, ph, &memDC, 0, 0, bw, bh);
		}
	}
}

void Np2Panel::OnSize(wxSizeEvent &evt)
{
	Refresh();
	evt.Skip();
}

/* ---- keyboard ---- */

void Np2Panel::OnKeyDown(wxKeyEvent &evt)
{
	int key = evt.GetKeyCode();

	/* Alt+Enter → toggle fullscreen */
	if (evt.AltDown() && (key == WXK_RETURN || key == WXK_NUMPAD_ENTER)) {
		Np2Frame *frame = wxDynamicCast(GetParent(), Np2Frame);
		if (frame) frame->ToggleFullScreen();
		return;
	}

	/* F12: capture mouse toggle (when F12KEY==0) or pass to emulator */
	if (key == WXK_F12) {
		if (np2oscfg.F12KEY == 0) {
			/* Toggle mouse capture */
			if (m_mouseCaptured) ReleaseMouse();
			else                  CaptureMouse();
		} else {
			wxkbd_keydown(key, (int)evt.GetRawKeyCode());
		}
		return;
	}

	wxkbd_keydown(key, (int)evt.GetRawKeyCode());
}

void Np2Panel::OnKeyUp(wxKeyEvent &evt)
{
	wxkbd_keyup(evt.GetKeyCode(), (int)evt.GetRawKeyCode());
}

/* ---- mouse ---- */

void Np2Panel::OnMouseMove(wxMouseEvent &evt)
{
	if (!m_mouseCaptured) { evt.Skip(); return; }

	wxPoint pos = evt.GetPosition();
	int cx = GetClientSize().x / 2;
	int cy = GetClientSize().y / 2;
	int dx = pos.x - m_lastMousePos.x;
	int dy = pos.y - m_lastMousePos.y;

	if (dx || dy) {
		mousemng_onmove(dx, dy);
		/* warp cursor back to center to allow unlimited movement */
		WarpPointer(cx, cy);
		m_lastMousePos = wxPoint(cx, cy);
	}
}

void Np2Panel::OnMouseLeft(wxMouseEvent &evt)
{
	if (evt.ButtonDown()) {
		if (!m_mouseCaptured) {
			CaptureMouse();
		}
		mousemng_onleft(TRUE);
	} else {
		mousemng_onleft(FALSE);
	}
}

void Np2Panel::OnMouseMiddle(wxMouseEvent &evt)
{
	/* Middle click toggles mouse capture */
	if (m_mouseCaptured) ReleaseMouse();
	else                  CaptureMouse();
}

void Np2Panel::OnMouseRight(wxMouseEvent &evt)
{
	if (evt.ButtonDown()) {
		mousemng_onright(TRUE);
	} else {
		/* right-click while captured releases mouse */
		if (m_mouseCaptured) {
			ReleaseMouse();
		}
		mousemng_onright(FALSE);
	}
}

void Np2Panel::OnMouseCaptureLost(wxMouseCaptureLostEvent & /*evt*/)
{
	m_mouseCaptured = false;
	mousemng_showcursor();
}

void Np2Panel::CaptureMouse(void)
{
	if (!m_mouseCaptured) {
		m_mouseCaptured = true;
		int cx = GetClientSize().x / 2;
		int cy = GetClientSize().y / 2;
		m_lastMousePos = wxPoint(cx, cy);
		WarpPointer(cx, cy);
		wxPanel::CaptureMouse();
		SetCursor(wxCursor(wxCURSOR_BLANK));
		mousemng_hidecursor();
	}
}

void Np2Panel::ReleaseMouse(void)
{
	if (m_mouseCaptured) {
		m_mouseCaptured = false;
		wxPanel::ReleaseMouse();
		SetCursor(wxNullCursor);
		mousemng_showcursor();
	}
}

void Np2Panel::OnSetFocus(wxFocusEvent &evt)
{
	evt.Skip();
}

void Np2Panel::OnKillFocus(wxFocusEvent &evt)
{
	/* release all held keys */
	wxkbd_reset();
	evt.Skip();
}
