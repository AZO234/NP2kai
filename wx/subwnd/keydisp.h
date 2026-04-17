/* === Key display sub-window for wx port (non-modal) === */

#ifndef NP2_WX_SUBWND_KEYDISP_H
#define NP2_WX_SUBWND_KEYDISP_H

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/timer.h>

/* KeyDispFrame: shows currently-pressed PC-98 keys.
 * Floats over the main window, non-modal, non-blocking.
 */
class KeyDispFrame : public wxFrame
{
public:
	explicit KeyDispFrame(wxWindow *parent);
	~KeyDispFrame();

private:
	wxTimer m_refreshTimer;
	wxPanel *m_panel;

	void OnPaint(wxPaintEvent &evt);
	void OnTimer(wxTimerEvent &evt);
	void OnClose(wxCloseEvent &evt);

	/* draw key state */
	void DrawKeys(wxDC &dc);

	wxDECLARE_EVENT_TABLE();
};

#endif /* NP2_WX_SUBWND_KEYDISP_H */
