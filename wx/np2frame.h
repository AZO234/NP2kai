/* === Main application frame for wx port === */

#ifndef NP2_WX_NP2FRAME_H
#define NP2_WX_NP2FRAME_H

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/timer.h>
#include <wx/menu.h>

class Np2Panel;

/* IDs for menus */
enum {
	/* Emulate */
	ID_EMU_STARTPAUSE = wxID_HIGHEST + 1,
	ID_EMU_RESET,
	ID_EMU_STATESLOT_0,
	ID_EMU_STATESLOT_MAX = ID_EMU_STATESLOT_0 + 9,
	ID_EMU_LOADSTATE,
	ID_EMU_SAVESTATE,
	ID_EMU_NEW_FD_IMAGE,
	ID_EMU_NEW_HD_IMAGE,
	ID_EMU_PREFERENCE,
	ID_EMU_EXIT,

	/* Emulation Speed */
	ID_EMU_SPEED_05,
	ID_EMU_SPEED_1,
	ID_EMU_SPEED_2,
	ID_EMU_SPEED_4,
	ID_EMU_SPEED_8,
	ID_EMU_SPEED_16,

	/* FD drives */
	ID_FD1_OPEN,
	ID_FD1_EJECT,
	ID_FD2_OPEN,
	ID_FD2_EJECT,
	ID_FD3_OPEN,
	ID_FD3_EJECT,
	ID_FD4_OPEN,
	ID_FD4_EJECT,

	/* Drive submenu IDs */
	ID_FDD1_SUB = ID_FD1_OPEN + 100,
	ID_FDD2_SUB,
	ID_FDD3_SUB,
	ID_FDD4_SUB,
	ID_HDD1_SUB,
	ID_HDD2_SUB,
	ID_HDD3_SUB,
	ID_HDD4_SUB,
	ID_CD_SUB,

	/* HD drives */
	ID_HD1_OPEN,
	ID_HD1_EJECT,
	ID_HD2_OPEN,
	ID_HD2_EJECT,
	ID_HD3_OPEN,
	ID_HD3_EJECT,
	ID_HD4_OPEN,
	ID_HD4_EJECT,

	/* CD drive */
	ID_CD_OPEN,
	ID_CD_EJECT,

	/* Display */
	ID_DISP_FULLSCREEN,
	ID_DISP_ROTATE_0,
	ID_DISP_ROTATE_90,
	ID_DISP_ROTATE_180,
	ID_DISP_ROTATE_270,

	/* Input */
	ID_INPUT_CTRL_ALT_DEL,
	ID_INPUT_PASTE,

	/* View sub-windows */
	ID_VIEW_KEYDISP,

	/* Screen size */
	ID_SCREEN_X1,
	ID_SCREEN_X2,
	ID_SCREEN_X3,
	ID_SCREEN_FULL,

	/* Other */
	ID_OTHER_COPY_SCREEN,
	ID_OTHER_PNG_SAVE,
	ID_OTHER_TEXT_HOOK,
	ID_OTHER_ABOUT,
};

class Np2Frame : public wxFrame
{
public:
	Np2Frame(const wxString &title, const wxPoint &pos, const wxSize &size);
	~Np2Frame();

	/* Called from C code in sysmng.cpp */
	void RequestRedraw(void);
	void UpdateCaption(UINT8 flag);
	void UpdateMenuStatus(void);

	Np2Panel *GetPanel(void) { return m_panel; }
	bool IsRunning(void)     const { return m_running; }
	void ToggleFullScreen(void);
	void StartEmulation(void);
	void StopEmulation(void);

private:
	Np2Panel *m_panel;
	wxTimer   m_emuTimer;
	bool      m_running;
	bool      m_textHookEnabled;
	int       m_stateSlot;

	void BuildMenuBar(void);
	void BuildStatusBar(void);
	wxMenu *BuildFdMenu(int drive);
	wxMenu *BuildHdMenu(void);
	wxMenu *BuildCdMenu(void);

	/* Menu handlers */
	void OnEmuStartPause(wxCommandEvent &evt);
	void OnEmuReset(wxCommandEvent &evt);
	void OnEmuStateSlot(wxCommandEvent &evt);
	void OnEmuLoadState(wxCommandEvent &evt);
	void OnEmuSaveState(wxCommandEvent &evt);
	void OnEmuNewFdImage(wxCommandEvent &evt);
	void OnEmuNewHdImage(wxCommandEvent &evt);
	void OnEmuPreference(wxCommandEvent &evt);
	void OnEmuExit(wxCommandEvent &evt);

	void OnSysSpeed(wxCommandEvent &evt);

	void OnFdOpen(wxCommandEvent &evt);
	void OnFdEject(wxCommandEvent &evt);
	void OnHdOpen(wxCommandEvent &evt);
	void OnHdEject(wxCommandEvent &evt);
	void OnCdOpen(wxCommandEvent &evt);
	void OnCdEject(wxCommandEvent &evt);

	void OnDispFullScreen(wxCommandEvent &evt);
	void OnDispRotate(wxCommandEvent &evt);

	void OnInputCtrlAltDel(wxCommandEvent &evt);
	void OnInputPaste(wxCommandEvent &evt);

	void OnViewKeyDisp(wxCommandEvent &evt);

	void OnScreenX1(wxCommandEvent &evt);
	void OnScreenX2(wxCommandEvent &evt);
	void OnScreenX3(wxCommandEvent &evt);
	void OnScreenFull(wxCommandEvent &evt);

	void OnOtherCopyScreen(wxCommandEvent &evt);
	void OnOtherPngSave(wxCommandEvent &evt);
	void OnOtherTextHook(wxCommandEvent &evt);
	void OnOtherAbout(wxCommandEvent &evt);

	/* Timer: emulation step */
	void OnEmuTimer(wxTimerEvent &evt);
	/* Refresh events posted from C threads */
	void OnRefreshEvent(wxCommandEvent &evt);

	/* Window */
	void OnClose(wxCloseEvent &evt);
	void OnSize(wxSizeEvent &evt);

	wxDECLARE_EVENT_TABLE();
};

/* C-callable hooks (called from sysmng.cpp etc.) */
#ifdef __cplusplus
extern "C" {
#endif
void np2frame_requestRedraw(void);
void np2frame_updateCaption(UINT8 flag);
void np2frame_setTitle(const char *title);
#ifdef __cplusplus
}
#endif

#endif /* NP2_WX_NP2FRAME_H */
