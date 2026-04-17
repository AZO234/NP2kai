/* === PC-98 screen panel for wx port === */

#ifndef NP2_WX_NP2PANEL_H
#define NP2_WX_NP2PANEL_H

#include <wx/wx.h>
#include <wx/timer.h>

/* The Np2Panel handles:
 *  - Drawing the PC-98 framebuffer via wxBitmap
 *  - Keyboard/mouse input forwarding to the emulator
 *  - Mouse capture
 */
class Np2Panel : public wxPanel
{
public:
	explicit Np2Panel(wxWindow *parent);
	~Np2Panel();

	void UpdateScreen(void);
	wxBitmap GetBitmap(void);
	void CaptureMouse(void);
	void ReleaseMouse(void);
	bool IsMouseCaptured(void) const { return m_mouseCaptured; }

private:
	wxImage   m_image;
	bool      m_mouseCaptured;
	wxPoint   m_lastMousePos;

	void OnPaint(wxPaintEvent &evt);
	void OnSize(wxSizeEvent &evt);
	void OnKeyDown(wxKeyEvent &evt);
	void OnKeyUp(wxKeyEvent &evt);
	void OnMouseMove(wxMouseEvent &evt);
	void OnMouseLeft(wxMouseEvent &evt);
	void OnMouseMiddle(wxMouseEvent &evt);
	void OnMouseRight(wxMouseEvent &evt);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent &evt);
	void OnSetFocus(wxFocusEvent &evt);
	void OnKillFocus(wxFocusEvent &evt);

	wxDECLARE_EVENT_TABLE();
};

#endif /* NP2_WX_NP2PANEL_H */
