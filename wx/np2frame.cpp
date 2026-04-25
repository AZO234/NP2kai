/* === Main application frame for wx port === */

#include <compiler.h>
#include "np2frame.h"
#include "np2mt.h"
#include "np2panel.h"
#include "np2.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "ini.h"
#include <pccore.h>
#include <keystat.h>
#include <sysmng.h>
#include <wab/wab.h>
#if defined(SUPPORT_WAB)
#include <wab/wabbmpsave.h>
#include <common/bmpdata.h>
#endif
#include <dosio.h>
#include <codecnv/codecnv.h>
#include <diskimage/fddfile.h>
#include <fdd/diskdrv.h>
#include <fdd/sxsi.h>
#include <vram/scrndraw.h>
#include <statsave.h>
#include <font/font.h>
#include <io/iocore.h>   /* for fdc.equip */

#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/aboutdlg.h>
#include <wx/bmpbndl.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include "dialog/prefframe.h"
#include "subwnd/keydisp.h"

/* ---- module-level frame pointer for C hooks ---- */
static Np2Frame *s_frame = nullptr;

void np2frame_requestRedraw(void)
{
	if (s_frame) {
		/* thread-safe: post custom event */
		wxCommandEvent *ev = new wxCommandEvent(wxEVT_MENU, wxID_REFRESH);
		wxQueueEvent(s_frame, ev);
	}
}

void np2frame_updateCaption(UINT8 flag)
{
	if (s_frame) {
		/* pass flag as extra long so OnRefreshEvent can decide what to update */
		wxCommandEvent *ev = new wxCommandEvent(wxEVT_MENU, wxID_REFRESH + 1);
		ev->SetInt((int)flag);
		wxQueueEvent(s_frame, ev);
	}
}

void np2frame_setTitle(const char *title)
{
	if (s_frame) {
		/* SetTitle must run on the UI thread; post as a refresh+1 event.
		 * The title string is short-lived, so store it in a local static
		 * protected by the event serialization. */
		s_frame->CallAfter([t = wxString::FromUTF8(title)]() mutable {
			if (s_frame) s_frame->SetTitle(t);
		});
	}
}

/* ---- event table ---- */
wxBEGIN_EVENT_TABLE(Np2Frame, wxFrame)
	/* Emulate */
	EVT_MENU(ID_EMU_STARTPAUSE,   Np2Frame::OnEmuStartPause)
	EVT_MENU(ID_EMU_RESET,        Np2Frame::OnEmuReset)
	EVT_MENU_RANGE(ID_EMU_STATESLOT_0, ID_EMU_STATESLOT_MAX, Np2Frame::OnEmuStateSlot)
	EVT_MENU(ID_EMU_LOADSTATE,    Np2Frame::OnEmuLoadState)
	EVT_MENU(ID_EMU_SAVESTATE,    Np2Frame::OnEmuSaveState)
	EVT_MENU(ID_EMU_NEW_FD_IMAGE, Np2Frame::OnEmuNewFdImage)
	EVT_MENU(ID_EMU_NEW_HD_IMAGE, Np2Frame::OnEmuNewHdImage)
	EVT_MENU(ID_EMU_PREFERENCE,   Np2Frame::OnEmuPreference)
	EVT_MENU(ID_EMU_EXIT,         Np2Frame::OnEmuExit)
	EVT_MENU_RANGE(ID_EMU_SPEED_05, ID_EMU_SPEED_16, Np2Frame::OnSysSpeed)
	/* FD */
	EVT_MENU(ID_FD1_OPEN,  Np2Frame::OnFdOpen)
	EVT_MENU(ID_FD1_EJECT, Np2Frame::OnFdEject)
	EVT_MENU(ID_FD2_OPEN,  Np2Frame::OnFdOpen)
	EVT_MENU(ID_FD2_EJECT, Np2Frame::OnFdEject)
	EVT_MENU(ID_FD3_OPEN,  Np2Frame::OnFdOpen)
	EVT_MENU(ID_FD3_EJECT, Np2Frame::OnFdEject)
	EVT_MENU(ID_FD4_OPEN,  Np2Frame::OnFdOpen)
	EVT_MENU(ID_FD4_EJECT, Np2Frame::OnFdEject)
	/* HD */
	EVT_MENU(ID_HD1_OPEN,  Np2Frame::OnHdOpen)
	EVT_MENU(ID_HD1_EJECT, Np2Frame::OnHdEject)
	EVT_MENU(ID_HD2_OPEN,  Np2Frame::OnHdOpen)
	EVT_MENU(ID_HD2_EJECT, Np2Frame::OnHdEject)
	EVT_MENU(ID_HD3_OPEN,  Np2Frame::OnHdOpen)
	EVT_MENU(ID_HD3_EJECT, Np2Frame::OnHdEject)
	EVT_MENU(ID_HD4_OPEN,  Np2Frame::OnHdOpen)
	EVT_MENU(ID_HD4_EJECT, Np2Frame::OnHdEject)
	/* CD */
	EVT_MENU(ID_CD_OPEN,   Np2Frame::OnCdOpen)
	EVT_MENU(ID_CD_EJECT,  Np2Frame::OnCdEject)
	/* Display */
	EVT_MENU(ID_DISP_FULLSCREEN,   Np2Frame::OnDispFullScreen)
	EVT_MENU_RANGE(ID_DISP_ROTATE_0, ID_DISP_ROTATE_270, Np2Frame::OnDispRotate)
	/* Input */
	EVT_MENU(ID_INPUT_CTRL_ALT_DEL, Np2Frame::OnInputCtrlAltDel)
	EVT_MENU(ID_INPUT_PASTE,        Np2Frame::OnInputPaste)
	/* View */
	EVT_MENU(ID_VIEW_KEYDISP, Np2Frame::OnViewKeyDisp)
	/* Screen */
	EVT_MENU(ID_SCREEN_X1,   Np2Frame::OnScreenX1)
	EVT_MENU(ID_SCREEN_X2,   Np2Frame::OnScreenX2)
	EVT_MENU(ID_SCREEN_X3,   Np2Frame::OnScreenX3)
	EVT_MENU(ID_SCREEN_FULL, Np2Frame::OnScreenFull)
	/* Other */
	EVT_MENU(ID_OTHER_COPY_SCREEN, Np2Frame::OnOtherCopyScreen)
	EVT_MENU(ID_OTHER_PNG_SAVE,    Np2Frame::OnOtherPngSave)
	EVT_MENU(ID_OTHER_CYCLE_SCREENSHOT, Np2Frame::OnOtherCycleScreenshot)
	EVT_MENU(ID_OTHER_TEXT_HOOK,   Np2Frame::OnOtherTextHook)
	EVT_MENU(ID_OTHER_ABOUT,       Np2Frame::OnOtherAbout)
	/* Redraw / caption hooks (posted from C side via np2frame_requestRedraw etc.) */
	EVT_MENU(wxID_REFRESH,     Np2Frame::OnRefreshEvent)
	EVT_MENU(wxID_REFRESH + 1, Np2Frame::OnRefreshEvent)
	/* Timer */
	EVT_TIMER(wxID_ANY, Np2Frame::OnEmuTimer)
	/* Window */
	EVT_CLOSE(Np2Frame::OnClose)
	EVT_SIZE(Np2Frame::OnSize)
wxEND_EVENT_TABLE()

/* ---- construction ---- */

Np2Frame::Np2Frame(const wxString &title, const wxPoint &pos, const wxSize &size)
	: wxFrame(nullptr, wxID_ANY, title, pos, size)
	, m_panel(nullptr)
	, m_emuTimer(this)
	, m_cycleScreenshotTimer(this)
	, m_running(true)
	, m_textHookEnabled(false)
	, m_cycleScreenshotEnabled(np2oscfg.cycle_shot_enabled != 0)
	, m_stateSlot(0)
{
	s_frame = this;

	Bind(wxEVT_TIMER, &Np2Frame::OnCycleScreenshotTimer, this, m_cycleScreenshotTimer.GetId());

	SetMinSize(wxSize(320, 200));

	/* Create screen panel */
	m_panel = new Np2Panel(this);

	if (m_cycleScreenshotEnabled && cycle_shot_interval >= 1000) {
		m_cycleScreenshotTimer.Start(cycle_shot_interval);
	} else {
		m_cycleScreenshotEnabled = false;
	}

	/* Menu */
	BuildMenuBar();

	/* Status bar */
	BuildStatusBar();

	/* Layout: panel fills entire client area */
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_panel, 1, wxEXPAND);
	SetSizer(sizer);

	/* Multithread system must always be initialized (nevent mutex etc.) */
	np2_multithread_initialize();

	if (np2wabcfg.multithread) {
		/* Emulator thread mode: timer is UI-refresh only */
		np2_multithread_start();
		m_emuTimer.Start(50);
	} else {
		/* Single-thread mode: timer drives emulation */
		m_emuTimer.Start(1);
	}

	m_panel->SetFocus();
}

Np2Frame::~Np2Frame()
{
	m_emuTimer.Stop();
	np2_multithread_shutdown();
	s_frame = nullptr;
}

void Np2Frame::StartEmulation(void)
{
	if (!m_running) return;
	np2_multithread_resume();
	soundmng_play();
}

void Np2Frame::StopEmulation(void)
{
	soundmng_stop();
	np2_multithread_suspend();
}

/* ---- menu construction ---- */

/* Helper: append a menu item with an ArtProvider bitmap */
static wxMenuItem *AppendArt(wxMenu *menu, int id,
                              const wxString &text, const wxString &help,
                              const wxArtID &artId, wxItemKind kind = wxITEM_NORMAL)
{
	wxMenuItem *item = new wxMenuItem(menu, id, text, help, kind);
	if (kind == wxITEM_NORMAL) {
		wxBitmap bmp = wxArtProvider::GetBitmap(artId, wxART_MENU);
		if (bmp.IsOk())
			item->SetBitmap(bmp);
	}
	menu->Append(item);
	return item;
}

/* Helper: append a sub-menu item with an ArtProvider bitmap */
static wxMenuItem *AppendSubArt(wxMenu *menu, wxMenu *sub,
                                const wxString &text, const wxArtID &artId,
                                int id = wxID_ANY)
{
	wxMenuItem *item = new wxMenuItem(menu, id, text, "", wxITEM_NORMAL, sub);
	wxBitmap bmp = wxArtProvider::GetBitmap(artId, wxART_MENU);
	if (bmp.IsOk())
		item->SetBitmap(bmp);
	menu->Append(item);
	return item;
}

void Np2Frame::BuildMenuBar(void)
{
	wxMenuBar *bar = new wxMenuBar;

	/* --- Emulate --- */
	wxMenu *menuEmu = new wxMenu;

	AppendArt(menuEmu, ID_EMU_STARTPAUSE, "&Start/Pause\tF5",
	          "Start or pause emulation", wxART_GO_FORWARD);
	AppendArt(menuEmu, ID_EMU_RESET, "&Reset",
	          "Hardware reset",           wxART_UNDO);

	/* Emulation Speed sub-menu */
	{
		wxMenu *menuSpeed = new wxMenu;
		menuSpeed->AppendRadioItem(ID_EMU_SPEED_05, "x0.5");
		menuSpeed->AppendRadioItem(ID_EMU_SPEED_1,  "x1");
		menuSpeed->AppendRadioItem(ID_EMU_SPEED_2,  "x2");
		menuSpeed->AppendRadioItem(ID_EMU_SPEED_4,  "x4");
		menuSpeed->AppendRadioItem(ID_EMU_SPEED_8,  "x8");
		menuSpeed->AppendRadioItem(ID_EMU_SPEED_16, "x16");
		menuSpeed->Check(ID_EMU_SPEED_1, true);
		AppendSubArt(menuEmu, menuSpeed, "&Emulation Speed", wxART_LIST_VIEW);
	}
	menuEmu->AppendSeparator();

	/* State Slot sub-menu */
	wxMenu *slotMenu = new wxMenu;
	for (int i = 0; i <= 9; i++) {
		slotMenu->AppendRadioItem(ID_EMU_STATESLOT_0 + i,
		    wxString::Format("Slot &%d", i));
	}
	slotMenu->Check(ID_EMU_STATESLOT_0, true);
	AppendSubArt(menuEmu, slotMenu, "State &Slot", wxART_COPY);

	AppendArt(menuEmu, ID_EMU_LOADSTATE, "&Load State\tF7",
	          "Load emulator state", wxART_FILE_OPEN);
	AppendArt(menuEmu, ID_EMU_SAVESTATE, "&Save State\tF8",
	          "Save emulator state", wxART_FILE_SAVE);
	menuEmu->AppendSeparator();
	AppendArt(menuEmu, ID_EMU_NEW_FD_IMAGE, "New &FD Image...",
	          "Create new floppy image",   wxART_NEW);
	AppendArt(menuEmu, ID_EMU_NEW_HD_IMAGE, "New &HD Image...",
	          "Create new hard disk image", wxART_NEW);
	menuEmu->AppendSeparator();
	AppendArt(menuEmu, ID_EMU_PREFERENCE, "&Preference...\tCtrl+P",
	          "Emulator settings", wxART_HELP_SETTINGS);
	menuEmu->AppendSeparator();
	AppendArt(menuEmu, ID_EMU_EXIT, "E&xit\tAlt+F4",
	          "Exit the emulator", wxART_QUIT);

	bar->Append(menuEmu, "&Emulate");

	/* --- FD drives --- */
	wxMenu *menuFD = new wxMenu;
	for (int drv = 0; drv < 4; drv++) {
		AppendSubArt(menuFD, BuildFdMenu(drv),
		    wxString::Format("FD &%d", drv + 1), wxART_FLOPPY, ID_FDD1_SUB + drv);
	}
	bar->Append(menuFD, "&FDD");

	/* --- HD --- */
	bar->Append(BuildHdMenu(), "&HDD");

	/* --- CD --- */
	wxMenu *menuCD = new wxMenu;
	AppendSubArt(menuCD, BuildCdMenu(), "&CD-ROM", wxART_CDROM, ID_CD_SUB);
	bar->Append(menuCD, "&CD");

	/* --- Display --- */
	wxMenu *menuDisp = new wxMenu;
	AppendArt(menuDisp, ID_DISP_FULLSCREEN, "&Full Screen\tAlt+Enter", "Toggle full screen mode", wxART_FULL_SCREEN, wxITEM_CHECK);
	menuDisp->AppendSeparator();
	wxMenu *rotateMenu = new wxMenu;
	rotateMenu->AppendRadioItem(ID_DISP_ROTATE_0,   "&Normal");
	rotateMenu->AppendRadioItem(ID_DISP_ROTATE_90,  "&Left rotated");
	rotateMenu->AppendRadioItem(ID_DISP_ROTATE_180, "&180 rotated");
	rotateMenu->AppendRadioItem(ID_DISP_ROTATE_270, "&Right rotated");
	AppendSubArt(menuDisp, rotateMenu, "&Rotate", wxART_UNDO);
	menuDisp->AppendSeparator();
	{
		wxMenuItem *ki = new wxMenuItem(menuDisp, ID_VIEW_KEYDISP,
		    "&Key Display", "Show/hide key display window", wxITEM_CHECK);
		menuDisp->Append(ki);
	}
	{
		wxMenu *scrnMenu = new wxMenu;
		scrnMenu->AppendRadioItem(ID_SCREEN_X1,   "&x1 (640x400)");
		scrnMenu->AppendRadioItem(ID_SCREEN_X2,   "&x2 (1280x800)");
		scrnMenu->AppendRadioItem(ID_SCREEN_X3,   "&x3 (1920x1200)");
		scrnMenu->AppendRadioItem(ID_SCREEN_FULL, "&Full Screen");
		scrnMenu->Check(ID_SCREEN_X1, true);
		AppendSubArt(menuDisp, scrnMenu, "&Screen Size", wxART_FULL_SCREEN);
	}
	bar->Append(menuDisp, "&Display");

	/* --- Input --- */
	wxMenu *menuInput = new wxMenu;
	AppendArt(menuInput, ID_INPUT_CTRL_ALT_DEL, "Send Ctrl+Alt+Del\tCtrl+Alt+Del", "Send hardware reset sequence", wxART_GO_HOME);
	AppendArt(menuInput, ID_INPUT_PASTE,        "Paste Clipboard Text\tCtrl+V", "Paste text from clipboard", wxART_PASTE);
	bar->Append(menuInput, "&Input");

	/* --- Other --- */
	wxMenu *menuOther = new wxMenu;
	AppendArt(menuOther, ID_OTHER_COPY_SCREEN, "&Copy Screen\tCtrl+C", "Copy screen to clipboard", wxART_COPY);
	AppendArt(menuOther, ID_OTHER_PNG_SAVE,    "&PNG Save...", "Save screen as PNG image", wxART_FILE_SAVE_AS);
	AppendArt(menuOther, ID_OTHER_CYCLE_SCREENSHOT, "Cycle Screenshot", "Periodically save screenshots to a file", wxART_REDO, wxITEM_CHECK);
	AppendArt(menuOther, ID_OTHER_TEXT_HOOK,   "&Text ROM Hook...", "Start/stop text ROM hook", wxART_LIST_VIEW, wxITEM_CHECK);
	menuOther->AppendSeparator();
	AppendArt(menuOther, ID_OTHER_ABOUT,       "&About...", "Show about dialog", wxART_HELP);
	bar->Append(menuOther, "&Other");

	SetMenuBar(bar);
	UpdateMenuStatus();
}

wxMenu *Np2Frame::BuildFdMenu(int drive)
{
	wxMenu *m = new wxMenu;
	int base = ID_FD1_OPEN + drive * 2;
	AppendArt(m, base,     "&Open/Change Image...", "", wxART_FILE_OPEN);
	AppendArt(m, base + 1, "&Eject",                "", wxART_DELETE);
	(void)drive;
	return m;
}

wxMenu *Np2Frame::BuildHdMenu(void)
{
	wxMenu *m = new wxMenu;
	for (int drv = 0; drv < 4; drv++) {
		wxMenu *sub = new wxMenu;
		int base = ID_HD1_OPEN + drv * 2;
		AppendArt(sub, base,     "&Open/Change Image...", "", wxART_FILE_OPEN);
		AppendArt(sub, base + 1, "&Eject",                "", wxART_DELETE);
		AppendSubArt(m, sub, wxString::Format("HD &%d", drv + 1), wxART_HARDDISK, ID_HDD1_SUB + drv);
	}
	return m;
}

wxMenu *Np2Frame::BuildCdMenu(void)
{
	wxMenu *m = new wxMenu;
	AppendArt(m, ID_CD_OPEN,  "&Open/Mount Image...", "", wxART_CDROM);
	AppendArt(m, ID_CD_EJECT, "&Eject",               "", wxART_DELETE);
	return m;
}

void Np2Frame::BuildStatusBar(void)
{
	CreateStatusBar(2);
	SetStatusText("NP2kai wx", 0);
}

/* ---- emulation timer / redraw ---- */

void Np2Frame::OnEmuTimer(wxTimerEvent &evt)
{
	(void)evt;
	if (np2wabcfg.multithread) {
		/* Emulator thread is running — timer is UI-refresh only */
		UpdateMenuStatus();
	} else {
		/* Single-thread mode: drive emulation from timer */
		if (m_running) {
			extern void np2_exec(void);
			np2_exec();
		}
	}
}

/* Returns the current screen as wxBitmap.
 * When the Window Accelerator Board relay is active, captures the WAB
 * framebuffer; otherwise reads from the normal VRAM panel. */
static wxBitmap CaptureScreen(Np2Panel *panel)
{
#if defined(SUPPORT_WAB)
	if (np2wab.relay) {
		/* Save WAB screen to a temp BMP and load it back as wxImage */
		wxString tmpPath = wxFileName::GetTempDir() + wxFILE_SEP_PATH + "np2wab_tmp.bmp";
		if (np2wab_writebmp(tmpPath.ToUTF8().data()) == SUCCESS) {
			wxImage img;
			if (img.LoadFile(tmpPath, wxBITMAP_TYPE_BMP)) {
				wxRemoveFile(tmpPath);
				return wxBitmap(img);
			}
			wxRemoveFile(tmpPath);
		}
	}
#endif
	return panel ? panel->GetBitmap() : wxNullBitmap;
}

void Np2Frame::OnCycleScreenshotTimer(wxTimerEvent & /*evt*/)
{
	if (!m_cycleScreenshotEnabled) return;

	bool success = false;
	wxBitmap bmp = CaptureScreen(m_panel);
	if (bmp.IsOk()) {
		wxString path = wxString::FromUTF8(cycle_shot_path);
		if (!path.empty()) {
			if (bmp.ConvertToImage().SaveFile(path, wxBITMAP_TYPE_PNG)) {
				success = true;
			}
		}
	}

	if (!success) {
		m_cycleScreenshotEnabled = false;
		m_cycleScreenshotTimer.Stop();
		np2oscfg.cycle_shot_enabled = 0;
		sysmng_update(SYS_UPDATECFG);
		UpdateMenuStatus();
	}
}

void Np2Frame::UpdateMenuStatus(void)
{
	wxMenuBar *bar = GetMenuBar();
	if (!bar) return;

	/* FDD: np2cfg.fddfile は diskdrv_setfddex で即座に更新される */
	for (int i = 0; i < 4; i++) {
		const OEMCHAR *fname = np2cfg.fddfile[i];
		wxString label = wxString::Format("FD &%d", i + 1);
		if (fname && fname[0]) {
			label += ": ";
			label += wxString::FromUTF8(file_getname((char *)fname));
		}
		wxMenuItem *item = bar->FindItem(ID_FDD1_SUB + i);
		if (item) item->SetItemLabel(label);
	}

	/* HDD */
	for (int i = 0; i < 4; i++) {
		const OEMCHAR *fname = diskdrv_getsxsi((REG8)i);
		wxString label = wxString::Format("HD &%d", i + 1);
		if (fname && fname[0]) {
			label += ": ";
			label += wxString::FromUTF8(file_getname((char *)fname));
		}
		wxMenuItem *item = bar->FindItem(ID_HDD1_SUB + i);
		if (item) item->SetItemLabel(label);
	}

	/* CD-ROM: diskdrv_getsxsi は sasihdd を返すので sxsi_getfilename を使う */
	{
		int cd_drv = findCdromDrive();
		wxString label = "&CD-ROM";
		if (cd_drv >= 0) {
			const OEMCHAR *fname = sxsi_getfilename((REG8)cd_drv);
			if (fname && fname[0]) {
				label += ": ";
				label += wxString::FromUTF8(file_getname((char *)fname));
			}
		}
		wxMenuItem *item = bar->FindItem(ID_CD_SUB);
		if (item) item->SetItemLabel(label);
	}

	/* Display */
	bar->Check(ID_DISP_FULLSCREEN,  IsFullScreen());
	/* Map SCRNMODE rotate bits → menu item index (0-3) */
	{
		int rotidx = 0;
		UINT8 rot = scrnmode & SCRNMODE_ROTATEMASK;
		if      (rot == SCRNMODE_ROTATELEFT)  rotidx = 1;
		else if (rot == SCRNMODE_ROTATEDIR)   rotidx = 2;
		else if (rot == SCRNMODE_ROTATERIGHT) rotidx = 3;
		bar->Check(ID_DISP_ROTATE_0 + rotidx, true);
	}

	/* Other */
	bar->Check(ID_OTHER_TEXT_HOOK, m_textHookEnabled);
	bar->Check(ID_OTHER_CYCLE_SCREENSHOT, m_cycleScreenshotEnabled);
}

void Np2Frame::OnRefreshEvent(wxCommandEvent &evt)
{
	if (evt.GetId() == wxID_REFRESH) {
		if (m_panel) m_panel->UpdateScreen();
	} else {
		UpdateCaption(SYS_UPDATECAPTION_ALL);
		UpdateMenuStatus();
	}
}

void Np2Frame::RequestRedraw(void)
{
	if (m_panel) m_panel->UpdateScreen();
}

void Np2Frame::UpdateCaption(UINT8 flag)
{
	sysmng_updatecaption(flag);
}

/* ---- Emulate menu handlers ---- */

void Np2Frame::OnEmuStartPause(wxCommandEvent & /*evt*/)
{
	m_running = !m_running;
	if (m_running) {
		np2_multithread_resume();
		soundmng_play();
		GetMenuBar()->SetLabel(ID_EMU_STARTPAUSE, "&Pause");
	} else {
		soundmng_stop();
		np2_multithread_suspend();
		GetMenuBar()->SetLabel(ID_EMU_STARTPAUSE, "&Start");
	}
}

void Np2Frame::OnEmuReset(wxCommandEvent & /*evt*/)
{
	pccore_reset();
	soundmng_reset();
}

void Np2Frame::OnEmuStateSlot(wxCommandEvent &evt)
{
	m_stateSlot = evt.GetId() - ID_EMU_STATESLOT_0;
	np2_stateslotnow = m_stateSlot;
	UpdateCaption(SYS_UPDATECAPTION_ALL);
}

void Np2Frame::OnEmuLoadState(wxCommandEvent & /*evt*/)
{
	char ext[8];
	snprintf(ext, sizeof(ext), "s%d", m_stateSlot);
	flagload(ext, NP2_WX_APPNAME, FALSE);
}

void Np2Frame::OnEmuSaveState(wxCommandEvent & /*evt*/)
{
	char ext[8];
	snprintf(ext, sizeof(ext), "s%d", m_stateSlot);
	flagsave(ext);
}

void Np2Frame::OnEmuNewFdImage(wxCommandEvent & /*evt*/)
{
	wxFileDialog dlg(this, "Create New Floppy Image", fddfolder, "",
	    "D88 Images (*.d88)|*.d88|All files (*.*)|*.*",
	    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dlg.ShowModal() == wxID_OK) {
		milstr_ncpy(fddfolder, dlg.GetDirectory().ToUTF8().data(), MAX_PATH);
		sysmng_update(SYS_UPDATECFG);
		/* disk_newdisk would create the image - stub */
		wxMessageBox("New FD image: not yet implemented.", "Info", wxOK);
	}
}

void Np2Frame::OnEmuNewHdImage(wxCommandEvent & /*evt*/)
{
	wxFileDialog dlg(this, "Create New Hard Disk Image", hddfolder, "",
	    "NHD Images (*.nhd)|*.nhd|All files (*.*)|*.*",
	    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dlg.ShowModal() == wxID_OK) {
		milstr_ncpy(hddfolder, dlg.GetDirectory().ToUTF8().data(), MAX_PATH);
		sysmng_update(SYS_UPDATECFG);
		wxMessageBox("New HD image: not yet implemented.", "Info", wxOK);
	}
}

void Np2Frame::OnEmuPreference(wxCommandEvent & /*evt*/)
{
	/* Pause emulator thread while dialog is open */
	StopEmulation();

	PrefFrame pref(this);
	pref.ShowModal();   /* blocks until OK / Cancel / close */

	/* Resume emulator thread */
	StartEmulation();
}

void Np2Frame::OnEmuExit(wxCommandEvent & /*evt*/)
{
	Close(true);
}

void Np2Frame::OnSysSpeed(wxCommandEvent &evt)
{
	const UINT32 speeds[] = {50, 100, 200, 400, 800, 1600};
	int idx = evt.GetId() - ID_EMU_SPEED_05;
	if (idx >= 0 && idx < 6) {
		np2cfg.emuspeed = speeds[idx];
		sysmng_update(SYS_UPDATECFG);
	}
}

/* ---- FD handlers ---- */

static int fdDriveFromId(int id)
{
	/* IDs: FD1_OPEN=base, FD1_EJECT=base+1, FD2_OPEN=base+2 ... */
	return (id - ID_FD1_OPEN) / 2;
}


void Np2Frame::OnFdOpen(wxCommandEvent &evt)
{
	int drv = fdDriveFromId(evt.GetId());
	wxFileDialog dlg(this,
	    wxString::Format("Open FD%d Image", drv + 1),
	    fddfolder, "",
	    "FD Images (*.d88;*.d98;*.fdi;*.hdm)|*.d88;*.d98;*.fdi;*.hdm|All files (*.*)|*.*",
	    wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() != wxID_OK) return;

	wxString path = dlg.GetPath();
	StopEmulation();

	np2cfg.fddequip |= (UINT8)(1 << drv);
	fdc.equip       |= (UINT8)(1 << drv);
	/* diskdrv_readyfdd: immediate mount (same as startup), not delayed */
	diskdrv_readyfdd((REG8)drv, path.ToUTF8().data(), 0);
	milstr_ncpy(np2cfg.fddfile[drv], path.ToUTF8().data(), MAX_PATH);
	milstr_ncpy(fddfolder, dlg.GetDirectory().ToUTF8().data(), MAX_PATH);
	sysmng_update(SYS_UPDATEFDD | SYS_UPDATECFG);
	UpdateMenuStatus();

	StartEmulation();
}

void Np2Frame::OnFdEject(wxCommandEvent &evt)
{
	int drv = fdDriveFromId(evt.GetId());
	StopEmulation();
	np2cfg.fddfile[drv][0] = '\0';
	fdd_eject((REG8)drv);
	sysmng_update(SYS_UPDATEFDD | SYS_UPDATECFG);
	UpdateMenuStatus();
	StartEmulation();
}

/* ---- HD handlers ---- */

static int hdDriveFromId(int id)
{
	return (id - ID_HD1_OPEN) / 2;
}

void Np2Frame::OnHdOpen(wxCommandEvent &evt)
{
	int drv = hdDriveFromId(evt.GetId());
	wxFileDialog dlg(this,
	    wxString::Format("Open HD%d Image", drv + 1),
	    hddfolder, "",
	    "HD Images (*.nhd;*.hdi;*.thd;*.vhd)|*.nhd;*.hdi;*.thd;*.vhd|All files (*.*)|*.*",
	    wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() != wxID_OK) return;

	wxString path = dlg.GetPath();
	StopEmulation();

	milstr_ncpy(hddfolder, dlg.GetDirectory().ToUTF8().data(), MAX_PATH);
	milstr_ncpy(np2cfg.sasihdd[drv], path.ToUTF8().data(), MAX_PATH);
#if defined(SUPPORT_IDEIO)
	np2cfg.idetype[drv] = SXSIDEV_HDD;
#endif
	sysmng_update(SYS_UPDATEHDD | SYS_UPDATECFG);
	UpdateMenuStatus();

	StartEmulation();
}

void Np2Frame::OnHdEject(wxCommandEvent &evt)
{
	int drv = hdDriveFromId(evt.GetId());
	StopEmulation();
	np2cfg.sasihdd[drv][0] = '\0';
#if defined(SUPPORT_IDEIO)
	np2cfg.idetype[drv] = SXSIDEV_NC;
#endif
	sysmng_update(SYS_UPDATEHDD | SYS_UPDATECFG);
	UpdateMenuStatus();
	StartEmulation();
}

/* ---- CD handlers ---- */

void Np2Frame::OnCdOpen(wxCommandEvent & /*evt*/)
{
	int cd_drv = findCdromDrive();
	if (cd_drv < 0) {
		wxMessageBox("No CD-ROM drive configured.", "Info", wxOK | wxICON_INFORMATION);
		return;
	}
	wxFileDialog dlg(this, "Mount CD Image", cdfolder, "",
	    "CD Images (*.iso;*.cue;*.mds)|*.iso;*.cue;*.mds|All files (*.*)|*.*",
	    wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() != wxID_OK) return;

	wxString path = dlg.GetPath();
	StopEmulation();

	milstr_ncpy(cdfolder, dlg.GetDirectory().ToUTF8().data(), MAX_PATH);
	sxsi_devopen((REG8)cd_drv, path.ToUTF8().data());
#if defined(SUPPORT_IDEIO)
	milstr_ncpy(np2cfg.idecd[cd_drv & 0x03], path.ToUTF8().data(), MAX_PATH);
#endif
	sysmng_update(SYS_UPDATECFG);
	UpdateMenuStatus();

	StartEmulation();
}

void Np2Frame::OnCdEject(wxCommandEvent & /*evt*/)
{
	int cd_drv = findCdromDrive();
	if (cd_drv < 0) return;

	StopEmulation();
	sxsi_devopen((REG8)cd_drv, NULL);
#if defined(SUPPORT_IDEIO)
	np2cfg.idecd[cd_drv & 0x03][0] = '\0';
#endif
	sysmng_update(SYS_UPDATECFG);
	UpdateMenuStatus();
	StartEmulation();
}

void Np2Frame::OnDispFullScreen(wxCommandEvent &evt)
{
	ShowFullScreen(evt.IsChecked());
	/* Sync the menu checkmark to actual state (in case ShowFullScreen was denied) */
	wxMenuBar *bar = GetMenuBar();
	if (bar) bar->Check(ID_DISP_FULLSCREEN, IsFullScreen());
}

void Np2Frame::ToggleFullScreen(void)
{
	bool goFull = !IsFullScreen();
	ShowFullScreen(goFull);
	wxMenuBar *bar = GetMenuBar();
	if (bar) bar->Check(ID_DISP_FULLSCREEN, IsFullScreen());
}

void Np2Frame::OnDispRotate(wxCommandEvent &evt)
{
	/* Map menu item index to SCRNMODE rotate bits:
	 *  0 = normal (0x00)
	 *  1 = left/CCW  → SCRNMODE_ROTATELEFT  (0x10)
	 *  2 = 180°      → SCRNMODE_ROTATEDIR   (0x20)
	 *  3 = right/CW  → SCRNMODE_ROTATERIGHT (0x30) */
	static const UINT8 modetab[] = {
		0x00,
		SCRNMODE_ROTATELEFT,
		SCRNMODE_ROTATEDIR,
		SCRNMODE_ROTATERIGHT,
	};
	int id = evt.GetId() - ID_DISP_ROTATE_0;
	if (id >= 0 && id < 4) {
		UINT8 newmode = (scrnmode & ~SCRNMODE_ROTATEMASK) | modetab[id];
		changescreen(newmode);
		sysmng_update(SYS_UPDATECFG);

		/* Resize window to match orientation (90°/270° → portrait) */
		if (!IsFullScreen()) {
			int w, h;
			scrnmng_getsize(&w, &h);
			bool portrait = (newmode & SCRNMODE_ROTATE) != 0;
			SetClientSize(portrait ? h : w, portrait ? w : h);
		}
	}
}

/* ---- Input handlers ---- */

void Np2Frame::OnInputCtrlAltDel(wxCommandEvent & /*evt*/)
{
	keystat_keydown(0x74); /* Ctrl */
	keystat_keydown(0x73); /* GRPH (Alt) */
	keystat_keydown(0x39); /* Del */
	keystat_keyup(0x39);
	keystat_keyup(0x73);
	keystat_keyup(0x74);
}

void Np2Frame::OnInputPaste(wxCommandEvent & /*evt*/)
{
	if (!wxTheClipboard->Open()) return;
	if (!wxTheClipboard->IsSupported(wxDF_TEXT)) {
		wxTheClipboard->Close();
		return;
	}
	wxTextDataObject data;
	wxTheClipboard->GetData(data);
	wxString text = data.GetText();
	wxTheClipboard->Close();

	/* Convert UTF-8 (wxString internal) → Shift_JIS for PC-98 */
	wxScopedCharBuffer utf8 = text.ToUTF8();
	const char *utf8str = utf8.data();
	UINT sjislen = codecnv_utf8tosjis(NULL, 0, utf8str, (UINT)-1);
	if (sjislen == 0) return;
	char *sjisbuf = new char[sjislen + 1];
	codecnv_utf8tosjis(sjisbuf, sjislen + 1, utf8str, (UINT)-1);
	sjisbuf[sjislen] = '\0';

	autokey_start(sjisbuf);
	delete[] sjisbuf;
}

/* ---- View handlers ---- */

static KeyDispFrame *s_keydisp = nullptr;

void Np2Frame::OnViewKeyDisp(wxCommandEvent &evt)
{
	if (evt.IsChecked()) {
		if (!s_keydisp) {
			s_keydisp = new KeyDispFrame(this);
		}
		s_keydisp->Show(true);
	} else {
		if (s_keydisp) {
			s_keydisp->Hide();
		}
	}
}

/* ---- Screen size handlers ---- */

/* Return screen dimensions swapped when rotated 90°/270° */
static void GetDisplaySize(int scale, int *outW, int *outH)
{
	int w, h;
	scrnmng_getsize(&w, &h);
	bool portrait = (scrnmode & SCRNMODE_ROTATE) != 0;
	*outW = (portrait ? h : w) * scale;
	*outH = (portrait ? w : h) * scale;
}

void Np2Frame::OnScreenX1(wxCommandEvent & /*evt*/)
{
	int w, h;
	GetDisplaySize(1, &w, &h);
	SetClientSize(w, h);
}

void Np2Frame::OnScreenX2(wxCommandEvent & /*evt*/)
{
	int w, h;
	GetDisplaySize(2, &w, &h);
	SetClientSize(w, h);
}

void Np2Frame::OnScreenX3(wxCommandEvent & /*evt*/)
{
	int w, h;
	GetDisplaySize(3, &w, &h);
	SetClientSize(w, h);
}

void Np2Frame::OnScreenFull(wxCommandEvent & /*evt*/)
{
	ShowFullScreen(!IsFullScreen());
}

/* ---- Window events ---- */

void Np2Frame::OnClose(wxCloseEvent &evt)
{
	m_emuTimer.Stop();
	np2_multithread_stop();
	np2_terminate();
	evt.Skip();
}

void Np2Frame::OnSize(wxSizeEvent &evt)
{
	/* save window geometry */
	if (!IsMaximized() && !IsFullScreen()) {
		wxPoint pos = GetPosition();
		wxSize  sz  = GetClientSize();
		np2oscfg.winx      = pos.x;
		np2oscfg.winy      = pos.y;
		np2oscfg.winwidth  = sz.x;
		np2oscfg.winheight = sz.y;
	}
	evt.Skip();
}

/* ---- Other handlers ---- */

void Np2Frame::OnOtherCopyScreen(wxCommandEvent & /*evt*/)
{
	wxBitmap bmp = CaptureScreen(m_panel);
	if (bmp.IsOk()) {
		if (wxTheClipboard->Open()) {
			wxTheClipboard->SetData(new wxBitmapDataObject(bmp));
			wxTheClipboard->Close();
		}
	}
}

void Np2Frame::OnOtherPngSave(wxCommandEvent & /*evt*/)
{
	wxFileDialog dlg(this, "Save Screen as PNG", bmpfilefolder, "screen.png",
	    "PNG files (*.png)|*.png|All files (*.*)|*.*",
	    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dlg.ShowModal() == wxID_OK) {
		wxBitmap bmp = CaptureScreen(m_panel);
		if (bmp.IsOk()) {
			bmp.ConvertToImage().SaveFile(dlg.GetPath(), wxBITMAP_TYPE_PNG);
			milstr_ncpy(bmpfilefolder, dlg.GetDirectory().ToUTF8().data(), MAX_PATH);
			sysmng_update(SYS_UPDATECFG);
		}
	}
}

void Np2Frame::OnOtherCycleScreenshot(wxCommandEvent &evt)
{
	m_cycleScreenshotEnabled = evt.IsChecked();
	np2oscfg.cycle_shot_enabled = (UINT8)(m_cycleScreenshotEnabled ? 1 : 0);
	sysmng_update(SYS_UPDATECFG);

	if (m_cycleScreenshotEnabled && cycle_shot_interval >= 1000) {
		m_cycleScreenshotTimer.Start(cycle_shot_interval);
	} else {
		m_cycleScreenshotEnabled = false;
		m_cycleScreenshotTimer.Stop();
		UpdateMenuStatus();
	}
}

void Np2Frame::OnOtherTextHook(wxCommandEvent &evt)
{
	m_textHookEnabled = evt.IsChecked();
	if (m_textHookEnabled) {
		hook_fontrom_defenable();
	} else {
		hook_fontrom_defdisable();
	}
}

void Np2Frame::OnOtherAbout(wxCommandEvent & /*evt*/)
{
	extern const OEMCHAR np2version[];
	wxAboutDialogInfo info;
	info.SetName("NP2kai wx");
	info.SetVersion(wxString::FromUTF8(np2version));
	info.SetDescription("NP2kai port for wxWidgets + SDL3");
	info.SetCopyright("(c) 2026 AZO");
	
	/* Search for np2.svg in several locations:
	 *  1. ../misc/np2.svg  relative to the executable (dev build in build_wx/)
	 *  2. misc/np2.svg     relative to CWD (when run from project root) */
	wxIcon icon;
	wxString exeDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
	wxString candidates[] = {
		exeDir + wxFILE_SEP_PATH + ".." + wxFILE_SEP_PATH + "misc" + wxFILE_SEP_PATH + "np2.svg",
		wxString("misc") + wxFILE_SEP_PATH + "np2.svg",
	};
	for (const auto &iconPath : candidates) {
		if (wxFileExists(iconPath)) {
			wxBitmapBundle bundle = wxBitmapBundle::FromSVGFile(iconPath, wxSize(64, 64));
			if (bundle.IsOk()) {
				wxBitmap bmp = bundle.GetBitmap(wxSize(64, 64));
				icon.CopyFromBitmap(bmp);
			}
			break;
		}
	}
	if (icon.IsOk()) info.SetIcon(icon);

	wxAboutBox(info, this);
}
