/* === Preference dialog for wx port === */

#include <compiler.h>
#include "prefframe.h"
#include "np2.h"
#include "joymng.h"
#include "kbdmng.h"
#include "scrnmng.h"
#include "soundmng.h"
#include "sysmng.h"
#include "ini.h"
#include <pccore.h>
#include <wab/wab.h>
#if defined(SUPPORT_CL_GD5430)
#include <wab/cirrus_vga_extern.h>
#endif
#include <fdd/sxsi.h>
#include <common/bmpdata.h>
#include <fdd/diskdrv.h>
#include <generic/dipswbmp.h>
#include "timemng.h"
#include <calendar.h>
#include <sound/sound.h>
#include <sound/beep.h>
#include <sound/tms3631.h>
#include <sound/opngen.h>
#include <sound/psggen.h>
#include <sound/rhythm.h>
#include <sound/adpcm.h>
#include <sound/pcm86.h>
#include <sound/oplgen.h>
#if defined(SUPPORT_FMGEN)
#include <sound/opna.h>
#endif
#if defined(SUPPORT_VIDEOFILTER)
#include <vram/videofilter.h>
#endif

#include <wx/artprov.h>
#include <wx/gbsizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/colordlg.h>
#include <wx/filedlg.h>
#include <wx/statbmp.h>
#include <wx/dc.h>
#include <wx/mstream.h>
#include <wx/filepicker.h>
#include <wx/statline.h>

/* ---- IDs ---- */
enum {
	ID_PREF_OK      = wxID_OK,
	ID_PREF_CANCEL  = wxID_CANCEL,
	ID_PREF_DEFAULT = wxID_HIGHEST + 1,

	ID_DIPSW_BASE = 2000,  /* 2000..2023 = dipsw[0][0..7], [1][0..7], [2][0..7] */
};

/* Tab indices matching AddPage() order in the ctor. */
enum {
	TAB_SYSTEM = 0, TAB_DISPLAY, TAB_SOUND, TAB_INPUT, TAB_FDD, TAB_HDD,
	TAB_MIDI, TAB_SERIAL, TAB_NETWORK, TAB_HOSTDRV, TAB_DIPSW, TAB_CALENDAR, TAB_MISC
};
#define IF_TAB(x) if (tabId < 0 || tabId == (x))

/* ---- sound board names ---- */
static const wxString s_sndboard_names[] = {
	"None",
	"PC-9801-14",
	"PC-9801-26K",
	"PC-9801-86",
	"PC-9801-86 + 26K",
	"PC-9801-118",
	"PC-9801-86 with ADPCM",
	"Speak Board",
	"Mate-X PCM",
	"Wave Star",
};
static const UINT8 s_sndboard_vals[] = {
	0x00, 0x01, 0x02, 0x04, 0x06, 0x08, 0x14, 0x20, 0x60, 0x70
};

/* ---- CPU model names ---- */
static const wxString s_cpumodel_names[] = {
	"V30 (8MHz)", "V30 (10MHz)", "80286 (8MHz)", "80286 (10MHz)",
	"80286 (12MHz)", "i386SX (16MHz)", "i386SX (20MHz)", "i386SX (25MHz)",
	"i486SX (33MHz)"
};

#if defined(SUPPORT_CL_GD5430)
/* ---- CL-GD54xx type names and values ---- */
static const wxString s_clgd_names[] = {
	"PC-9821Bp,Bs,Be,Bf built-in",
	"PC-9821Xe built-in",
	"PC-9821Cb built-in",
	"PC-9821Cf built-in",
	"PC-9821Xe10,Xa7e,Xb10 built-in",
	"PC-9821Cb2 built-in",
	"PC-9821Cx2 built-in",
	"PC-9821 PCI CL-GD5446 built-in",
	"MELCO WAB-S",
	"MELCO WSN-A2F",
	"MELCO WSN-A4F",
	"I-O DATA GA-98NBI/C",
	"I-O DATA GA-98NBII",
	"I-O DATA GA-98NBIV",
	"PC-9801-96(PC-9801B3-E02)",
	"Auto Select(Xe10, GA-98NBI/C, PCI)",
	"Auto Select(Xe10, GA-98NBII, PCI)",
	"Auto Select(Xe10, GA-98NBIV, PCI)",
	"Auto Select(Xe10, WAB-S, PCI)",
	"Auto Select(Xe10, WSN-A2F, PCI)",
	"Auto Select(Xe10, WSN-A4F, PCI)",
	"Auto Select(Xe10, WAB-S)",
	"Auto Select(Xe10, WSN-A2F)",
	"Auto Select(Xe10, WSN-A4F)",
};
static const UINT16 s_clgd_vals[] = {
	CIRRUS_98ID_Be,
	CIRRUS_98ID_Xe,
	CIRRUS_98ID_Cb,
	CIRRUS_98ID_Cf,
	CIRRUS_98ID_Xe10,
	CIRRUS_98ID_Cb2,
	CIRRUS_98ID_Cx2,
	CIRRUS_98ID_PCI,
	CIRRUS_98ID_WAB,
	CIRRUS_98ID_WSN_A2F,
	CIRRUS_98ID_WSN,
	CIRRUS_98ID_GA98NBIC,
	CIRRUS_98ID_GA98NBII,
	CIRRUS_98ID_GA98NBIV,
	CIRRUS_98ID_96,
	CIRRUS_98ID_AUTO_XE_G1_PCI,
	CIRRUS_98ID_AUTO_XE_G2_PCI,
	CIRRUS_98ID_AUTO_XE_G4_PCI,
	CIRRUS_98ID_AUTO_XE_WA_PCI,
	CIRRUS_98ID_AUTO_XE_WS_PCI,
	CIRRUS_98ID_AUTO_XE_W4_PCI,
	CIRRUS_98ID_AUTO_XE10_WABS,
	CIRRUS_98ID_AUTO_XE10_WSN2,
	CIRRUS_98ID_AUTO_XE10_WSN4,
};
#endif /* SUPPORT_CL_GD5430 */

/* Visual DIP switch picture using bitmaps from generic/dipswbmp.c */

class BmpDipSwPanel : public wxPanel
{
public:
	BmpDipSwPanel(wxWindow *parent)
		: wxPanel(parent, wxID_ANY)
	{
		Bind(wxEVT_PAINT, &BmpDipSwPanel::OnPaint, this);
	}

	void SetBmpData(const void *data)
	{
		if (!data) return;

		/* The data is a full BMP file in memory. */
		const BMPFILE *lpBmpFile = (const BMPFILE *)data;
		size_t size = LOADINTELDWORD(lpBmpFile->bfSize);

		wxMemoryInputStream stream(data, size);
		wxImage img(stream, wxBITMAP_TYPE_BMP);
		if (img.IsOk()) {
			m_bmp = wxBitmap(img);
			SetMinSize(m_bmp.GetSize());
			SetSize(m_bmp.GetSize());
			Refresh();
		}
	}

private:
	wxBitmap m_bmp;

	void OnPaint(wxPaintEvent &)
	{
		wxPaintDC dc(this);
		if (m_bmp.IsOk()) {
			dc.DrawBitmap(m_bmp, 0, 0, false);
		}
	}
};

/* ---- DIP switch page with visual picture ---- */

class DipswPicPanel : public wxPanel
{
public:
	DipswPicPanel(wxWindow *parent, wxCheckBox *sw[][8])
		: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(360, 120))
		, m_sw(sw)
	{
		SetMinSize(wxSize(360, 120));
		Bind(wxEVT_PAINT, &DipswPicPanel::OnPaint, this);
	}

private:
	wxCheckBox *(*m_sw)[8];

	void OnPaint(wxPaintEvent &)
	{
		wxPaintDC dc(this);
		dc.SetBackground(wxBrush(wxColour(60, 60, 160)));
		dc.Clear();

		dc.SetPen(*wxBLACK_PEN);
		for (int bank = 0; bank < 3; bank++) {
			int x = 20 + bank * 110;
			int y = 30;
			/* Switch body */
			dc.SetBrush(wxBrush(wxColour(40, 40, 40)));
			dc.DrawRectangle(x, y, 90, 70);
			
			dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
			dc.SetTextForeground(*wxWHITE);
			dc.DrawText(wxString::Format("SW%d", bank + 1), x + 5, y - 22);

			for (int bit = 0; bit < 8; bit++) {
				bool on = (m_sw[bank][bit] && m_sw[bank][bit]->GetValue());
				int swX = x + 6 + bit * 10;
				int swY = y + 10;
				
				/* Knob slot */
				dc.SetBrush(*wxBLACK_BRUSH);
				dc.DrawRectangle(swX, swY, 8, 50);
				
				/* Knob */
				dc.SetBrush(on ? wxBrush(wxColour(200, 40, 40)) : *wxWHITE_BRUSH);
				if (on) {
					dc.DrawRectangle(swX + 1, swY + 2, 6, 18);
				} else {
					dc.DrawRectangle(swX + 1, swY + 30, 6, 18);
				}
			}
		}
	}
};

/* ============================================================ */

wxBEGIN_EVENT_TABLE(PrefFrame, wxDialog)
	EVT_BUTTON(ID_PREF_OK,      PrefFrame::OnOK)
	EVT_BUTTON(ID_PREF_DEFAULT, PrefFrame::OnDefault)
	EVT_BUTTON(ID_PREF_CANCEL,  PrefFrame::OnCancel)
	EVT_CLOSE(PrefFrame::OnClose)
	EVT_COMMAND_RANGE(ID_DIPSW_BASE, ID_DIPSW_BASE + 23, wxEVT_CHECKBOX, PrefFrame::OnDipswChange)
wxEND_EVENT_TABLE()

PrefFrame::PrefFrame(wxWindow *parent)
	: wxDialog(parent, wxID_ANY, "Preference",
	           wxDefaultPosition, wxSize(640, 560),
	           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, m_notebook(nullptr)
	, m_dipswPanel(nullptr)
	, m_sndboard(nullptr)
	, m_sndDipsw(nullptr)
	, m_snd26Dipsw(nullptr)
	, m_snd86Dipsw(nullptr)
	, m_snd118Dipsw(nullptr)
	, m_sndSpbDipsw(nullptr)
	, m_cpumodel(nullptr)
	, m_cpuMHz(nullptr)
{
	memset(m_dipsw, 0, sizeof(m_dipsw));
	memset(m_beepvol, 0, sizeof(m_beepvol));
	memset(m_arch, 0, sizeof(m_arch));

	wxPanel *root = new wxPanel(this);
	wxBoxSizer *rootSizer = new wxBoxSizer(wxVERTICAL);

	/* Notebook with tab icons */
	m_notebook = new wxNotebook(root, wxID_ANY);

	{
		const wxSize  sz(16, 16);
		const wxArtClient ctx = wxART_OTHER;
		wxImageList *imgs = new wxImageList(sz.x, sz.y, true);
		// 0 System
		imgs->Add(wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, ctx, sz));
		// 1 Display
		imgs->Add(wxArtProvider::GetBitmap(wxART_FULL_SCREEN,     ctx, sz));
		// 2 Sound
		imgs->Add(wxArtProvider::GetBitmap(wxART_INFORMATION,     ctx, sz));
		// 3 Input
		imgs->Add(wxArtProvider::GetBitmap(wxART_FIND,            ctx, sz));
		// 4 FDD
		imgs->Add(wxArtProvider::GetBitmap(wxART_FLOPPY,          ctx, sz));
		// 5 HDD
		imgs->Add(wxArtProvider::GetBitmap(wxART_HARDDISK,        ctx, sz));
		// 6 Serial/Parallel
		imgs->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE,     ctx, sz));
		// 7 Network
		imgs->Add(wxArtProvider::GetBitmap(wxART_GO_FORWARD,      ctx, sz));
		// 8 Hostdrv
		imgs->Add(wxArtProvider::GetBitmap(wxART_FOLDER_OPEN,     ctx, sz));
		// 9 DIP SW
		imgs->Add(wxArtProvider::GetBitmap(wxART_TICK_MARK,       ctx, sz));
		// 10 Calendar
		imgs->Add(wxArtProvider::GetBitmap(wxART_REPORT_VIEW,     ctx, sz));
		// 11 Misc
		imgs->Add(wxArtProvider::GetBitmap(wxART_HELP,            ctx, sz));
		m_notebook->AssignImageList(imgs);
	}

	m_notebook->AddPage(BuildSystemPage(m_notebook),   "System",   false, 0);
	m_notebook->AddPage(BuildDisplayPage(m_notebook),  "Display",  false, 1);
	m_notebook->AddPage(BuildSoundPage(m_notebook),    "Sound",    false, 2);
	m_notebook->AddPage(BuildInputPage(m_notebook),    "Input",    false, 3);
	m_notebook->AddPage(BuildFddPage(m_notebook),      "FDD",      false, 4);
	m_notebook->AddPage(BuildHddPage(m_notebook),      "HDD",      false, 5);
	m_notebook->AddPage(BuildMidiPage(m_notebook),     "MIDI",     false, 6);
	m_notebook->AddPage(BuildSerialPage(m_notebook),   "Serial",   false, 7);
	m_notebook->AddPage(BuildNetworkPage(m_notebook),  "Network",  false, 8);
	m_notebook->AddPage(BuildHostdrvPage(m_notebook),  "Hostdrv",  false, 9);
	m_notebook->AddPage(BuildDipswPage(m_notebook),    "DIP SW",   false, 10);
	m_notebook->AddPage(BuildCalendarPage(m_notebook), "Calendar", false, 11);
	m_notebook->AddPage(BuildMiscPage(m_notebook),     "Misc",     false, 12);

	rootSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 4);

	/* Buttons (OK / Cancel) */
	wxStdDialogButtonSizer *btnSizer = new wxStdDialogButtonSizer();
	btnSizer->AddButton(new wxButton(root, ID_PREF_OK,     wxGetStockLabel(wxID_OK)));
	btnSizer->AddButton(new wxButton(root, ID_PREF_CANCEL, wxGetStockLabel(wxID_CANCEL)));
	btnSizer->Realize();
	rootSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 4);

	root->SetSizer(rootSizer);

	Freeze();
	LoadFromConfig();
	Thaw();
	Centre();
}

PrefFrame::~PrefFrame() {}

#if defined(CPUCORE_IA32)
static const char *s_cpu_types[] = {
	"(custom)", "Intel 80386", "Intel i486SX", "Intel i486DX", "Intel Pentium", "Intel MMX Pentium",
	"Intel Pentium Pro", "Intel Pentium II", "Intel Pentium III", "Intel Pentium M", "Intel Pentium 4",
	"Intel Core 2 Duo", "Intel Core 2 Duo W", "Intel Core i", "AMD K6-2", "AMD K6-III", "AMD K7 Athlon",
	"AMD K7 Athlon XP", "Neko Processor II"
};
static const int s_cpu_indices[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 255
};
#else
static const char *s_cpu_types[] = {
	"Intel 80286 / V30"
};
#endif

/* ------------------------------------------------------------ */
/*  Page builders                                               */
/* ------------------------------------------------------------ */

wxPanel *PrefFrame::BuildSystemPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);

	int row = 0;

	/* Architecture */
	gs->Add(new wxStaticText(page, wxID_ANY, "Architecture:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *hsArch = new wxBoxSizer(wxHORIZONTAL);
	m_arch[0] = new wxRadioButton(page, wxID_ANY, "PC-98001VM", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	m_arch[1] = new wxRadioButton(page, wxID_ANY, "PC-9801VX");
	m_arch[2] = new wxRadioButton(page, wxID_ANY, "PC-286");
	for (int i = 0; i < 3; i++) hsArch->Add(m_arch[i], 0, wxRIGHT, 4);
	gs->Add(hsArch, wxGBPosition(row, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL);
	row++;

	/* CPU Type */
	gs->Add(new wxStaticText(page, wxID_ANY, "CPU Type:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *cpuType = new wxChoice(page, wxID_ANY);
	for (auto &s : s_cpu_types) cpuType->Append(s);
	cpuType->SetName("CPUType");
	cpuType->SetSelection(0);
	gs->Add(cpuType, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* Base Clock */
	gs->Add(new wxStaticText(page, wxID_ANY, "Base Clock:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *clkBase = new wxChoice(page, wxID_ANY);
	clkBase->Append("1.9968 MHz");
	clkBase->Append("2.4576 MHz");
	clkBase->SetName("clk_base");
	gs->Add(clkBase, wxGBPosition(row, 1), wxDefaultSpan);
	clkBase->Bind(wxEVT_CHOICE, [this](wxCommandEvent &) { UpdateMHz(); });

	/* Emulation Speed */
	gs->Add(new wxStaticText(page, wxID_ANY, "Speed:"),
	        wxGBPosition(row, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *speed = new wxChoice(page, wxID_ANY);
	for (auto &s : {"x0.5", "x1", "x2", "x4", "x8", "x16"}) speed->Append(s);
	speed->SetName("EmuSpeed");
	speed->SetSelection(1);
	gs->Add(speed, wxGBPosition(row, 3), wxDefaultSpan);
	row++;

	/* Multiplier */
	gs->Add(new wxStaticText(page, wxID_ANY, "Multiplier:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *slMult = new wxSlider(page, wxID_ANY, 20, 1, 140,
	                            wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL | wxSL_LABELS);
	slMult->SetName("clk_mult");
	gs->Add(slMult, wxGBPosition(row, 1), wxGBSpan(1, 2), wxEXPAND);
	slMult->Bind(wxEVT_SLIDER, [this](wxCommandEvent &) { UpdateMHz(); });

	m_cpuMHz = new wxStaticText(page, wxID_ANY, "0.0000 MHz");
	gs->Add(m_cpuMHz, wxGBPosition(row, 3), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	row++;

	/* Memory */
	gs->Add(new wxStaticText(page, wxID_ANY, "Memory (MB):"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *slMem = new wxSlider(page, wxID_ANY, 16, 1, 1024,
	                           wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL | wxSL_LABELS);
	slMem->SetName("extmem");
	gs->Add(slMem, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* FPU */
	gs->Add(new wxStaticText(page, wxID_ANY, "FPU Type:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *fpu = new wxChoice(page, wxID_ANY);
	fpu->Append("None");
	fpu->Append("80bit Extended");
	fpu->Append("64bit Double");
	fpu->Append("64bit Double + INT64");
	fpu->SetName("FPU_TYPE");
	fpu->SetSelection(1);
	gs->Add(fpu, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* Booleans */
	auto addCheck = [&](const char *label, const char *name) {
		auto *cb = new wxCheckBox(page, wxID_ANY, label);
		cb->SetName(name);
		gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	};
	addCheck("16bit I/O port addressing (PC-9821)", "SYSIOMSK");
	addCheck("Disable MMX",         "DisableMMX");
	addCheck("Fast memory check",   "SUPPORT_FAST_MEMORYCHECK");
	addCheck("Multi-threaded",      "MULTITHREAD");
#if defined(SUPPORT_ASYNC_CPU)
	addCheck("Async CPU",           "ASYNCCPU");
#endif

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	page->GetSizer()->AddStretchSpacer(1);
	page->GetSizer()->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	return page;
}

#if defined(SUPPORT_VIDEOFILTER)
static const char *s_vf_types[] = {
	"Thru", "NegaPosi", "DepthDown", "Grey",
	"Gamma", "Rotate Hue", "HSV smooth", "RGB smooth"
};

static wxStaticBoxSizer *BuildVideoFilterBox(wxWindow *parent, int filter_idx)
{
	wxString title = wxString::Format("Filter%d", filter_idx + 1);
	auto *box = new wxStaticBoxSizer(wxVERTICAL, parent, title);
	auto *gs  = new wxGridBagSizer(3, 6);
	int row = 0;

	auto *en = new wxCheckBox(parent, wxID_ANY, "Enable");
	en->SetName(wxString::Format("vf_f%d_en", filter_idx));
	gs->Add(en, wxGBPosition(row, 0), wxGBSpan(1, 4));
	row++;

	gs->Add(new wxStaticText(parent, wxID_ANY, "Type:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *tp = new wxChoice(parent, wxID_ANY);
	tp->SetName(wxString::Format("vf_f%d_type", filter_idx));
	for (auto &s : s_vf_types) tp->Append(s);
	tp->SetSelection(0);
	gs->Add(tp, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	for (int p = 0; p < 8; p++) {
		gs->Add(new wxStaticText(parent, wxID_ANY,
		    wxString::Format("Param%d:", p + 1)),
		    wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *txt = new wxTextCtrl(parent, wxID_ANY, "");
		txt->SetName(wxString::Format("vf_f%d_p%d", filter_idx, p));
		gs->Add(txt, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
		row++;
	}

	gs->AddGrowableCol(1, 1);
	box->Add(gs, 0, wxEXPAND | wxALL, 4);
	return box;
}
#endif /* SUPPORT_VIDEOFILTER */

wxPanel *PrefFrame::BuildDisplayPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addCheck = [&](const char *label, const char *name) {
		auto *cb = new wxCheckBox(page, wxID_ANY, label);
		cb->SetName(name);
		gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	};

	addCheck("Display Sync",     "DispSync");
	addCheck("Real Palette",     "Real_Pal");

	/* GDC */
	gs->Add(new wxStaticText(page, wxID_ANY, "GDC:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	{
		wxBoxSizer *hs = new wxBoxSizer(wxHORIZONTAL);
		auto *r0 = new wxRadioButton(page, wxID_ANY, "uPD7220",
		    wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		r0->SetName("gdc_7220");
		auto *r1 = new wxRadioButton(page, wxID_ANY, "uPD72020");
		r1->SetName("gdc_72020");
		hs->Add(r0, 0, wxRIGHT, 8);
		hs->Add(r1, 0);
		gs->Add(hs, wxGBPosition(row, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL);
	}
	row++;

	/* Graphic Charger */
	gs->Add(new wxStaticText(page, wxID_ANY, "Graphic Charger:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	{
		wxChoice *grcg = new wxChoice(page, wxID_ANY);
		grcg->SetName("GRCG_EGC");
#if defined(CPUCORE_IA32)
		grcg->Append("EGC");
		grcg->SetSelection(0);
		grcg->Enable(false);
#else
		for (auto &s : {"None", "GRCG", "GRCG+"}) grcg->Append(s);
		grcg->SetSelection(2); /* default GRCG+ */
#endif
		gs->Add(grcg, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	}
	row++;

#if defined(SUPPORT_PEGC)
	{
		auto *cb = new wxCheckBox(page, wxID_ANY, "Enable PEGC plane mode");
		cb->SetName("pegcplane");
#if !defined(CPUCORE_IA32)
		cb->Enable(false);
#endif
		gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	}
#endif

	/* LCD */
	addCheck("Liquid Crystal Display", "LCD_MODE_en");
	{
		wxBoxSizer *hs = new wxBoxSizer(wxHORIZONTAL);
		hs->Add(new wxStaticText(page, wxID_ANY, "  "), 0);
		auto *rev = new wxCheckBox(page, wxID_ANY, "LCD Reverse");
		rev->SetName("LCD_MODE_rev");
		hs->Add(rev, 0);
		gs->Add(hs, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	}

	addCheck("16 color board (PC-9801-24)", "color16b");
	addCheck("Skip scanlines",   "skipline");
	addCheck("Draw in 32-bit",   "draw32bit");
	addCheck("CRT relay sound",  "wabasw");

	gs->Add(new wxStaticText(page, wxID_ANY, "Skip light:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	{
		auto *skiplight = new wxSpinCtrl(page, wxID_ANY, "0",
		    wxDefaultPosition, wxDefaultSize,
		    wxSP_ARROW_KEYS, -32768, 32767, 0);
		skiplight->SetName("skplight");
		gs->Add(skiplight, wxGBPosition(row, 1), wxDefaultSpan);
	}
	row++;

#if defined(SUPPORT_WAB)
	/* WAB separator */
	gs->Add(new wxStaticLine(page), wxGBPosition(row++, 0), wxGBSpan(1, 4), wxEXPAND);
	gs->Add(new wxStaticText(page, wxID_ANY, "Window Accelerator Board"),
	        wxGBPosition(row++, 0), wxGBSpan(1, 4));
	addCheck("Multi Window Mode", "MULTIWND");
#endif

#if defined(SUPPORT_CL_GD5430)
	/* CL-GD54xx separator */
	gs->Add(new wxStaticLine(page), wxGBPosition(row++, 0), wxGBSpan(1, 4), wxEXPAND);
	gs->Add(new wxStaticText(page, wxID_ANY, "CL-GD54xx"),
	        wxGBPosition(row++, 0), wxGBSpan(1, 4));
	addCheck("Enable CL-GD54xx", "USE_CLGD");

	gs->Add(new wxStaticText(page, wxID_ANY, "Type:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	{
		wxChoice *clgdtype = new wxChoice(page, wxID_ANY);
		clgdtype->SetName("CLGDTYPE");
		for (auto &n : s_clgd_names) clgdtype->Append(n);
		clgdtype->SetSelection(4); /* default Xe10 */
		gs->Add(clgdtype, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	}
	row++;

	addCheck("Fake Hardware Cursor", "CLGDFCUR");
#endif /* SUPPORT_CL_GD5430 */

	gs->AddGrowableCol(1, 1);
	vs->Add(gs, 0, wxEXPAND | wxALL, 8);

#if defined(SUPPORT_VIDEOFILTER)
	auto *vfBox = new wxStaticBoxSizer(wxVERTICAL, page, "VideoFilter");
	for (int f = 0; f < 3; f++) {
		vfBox->Add(BuildVideoFilterBox(page, f), 0, wxEXPAND | wxALL, 4);
	}
	vs->Add(vfBox, 0, wxEXPAND | wxALL, 8);
#endif

	vs->AddStretchSpacer(1);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildSoundPage(wxNotebook *nb)
{
	wxPanel *page = new wxPanel(nb, wxID_ANY);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);

	wxGridBagSizer *gsTop = new wxGridBagSizer(4, 8);
	int row = 0;

	/* Sound board */
	gsTop->Add(new wxStaticText(page, wxID_ANY, "Sound Board:"),
	           wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	m_sndboard = new wxChoice(page, wxID_ANY);
	for (auto &n : s_sndboard_names) m_sndboard->Append(n);
	m_sndboard->SetSelection(0);
	gsTop->Add(m_sndboard, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	m_sndboard->Bind(wxEVT_CHOICE, [this](wxCommandEvent &) { UpdateDipswBmp(); });
	row++;

	/* Sample rate */
	gsTop->Add(new wxStaticText(page, wxID_ANY, "Sample Rate:"),
	           wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *rate = new wxChoice(page, wxID_ANY);
	rate->SetName("SampleHz");
	for (auto &r : {"11025", "22050", "44100", "48000", "88200", "96000"})
		rate->Append(wxString::Format("%s Hz", r));
	rate->SetSelection(2);
	gsTop->Add(rate, wxGBPosition(row, 1), wxDefaultSpan);

	/* Buffer delay */
	gsTop->Add(new wxStaticText(page, wxID_ANY, "Buffer delay:"),
	           wxGBPosition(row, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *delay = new wxChoice(page, wxID_ANY);
	delay->SetName("Latencys");
	delay->Append("Short (47ms)");
	delay->Append("Normal (93ms)");
	delay->Append("Long (186ms)");
	delay->Append("Max (372ms)");
	delay->SetSelection(1);
	gsTop->Add(delay, wxGBPosition(row, 3), wxDefaultSpan);
	row++;

	vs->Add(gsTop, 0, wxEXPAND | wxALL, 8);

	/* Sub-notebook for board-specific settings */
	wxNotebook *sndNb = new wxNotebook(page, wxID_ANY);
	sndNb->AddPage(BuildSndMixerPage(sndNb), "Mixer");
	sndNb->AddPage(BuildSnd14Page(sndNb),    "PC-9801-14");
	sndNb->AddPage(BuildSnd26Page(sndNb),    "PC-9801-26K");
	sndNb->AddPage(BuildSnd86Page(sndNb),    "PC-9801-86");
	sndNb->AddPage(BuildSnd118Page(sndNb),   "PC-9801-118");
	sndNb->AddPage(BuildSndWSSPage(sndNb),   "WSS");
	sndNb->AddPage(BuildSndSB16Page(sndNb),  "SB16");
	sndNb->AddPage(BuildSndSpbPage(sndNb),   "SpeakBoard");
	sndNb->AddPage(BuildSndJoyPage(sndNb),   "Joypad");
#if defined(SUPPORT_FMGEN)
	sndNb->AddPage(BuildSndFMGenPage(sndNb), "fmgen");
#endif

	vs->Add(sndNb, 1, wxEXPAND | wxALL, 4);

	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildSndMixerPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addVol = [&](const char *label, const char *name, int defval, int minv, int maxv) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *sl = new wxSlider(page, wxID_ANY, defval, minv, maxv,
		                        wxDefaultPosition, wxSize(200, -1), wxSL_HORIZONTAL | wxSL_LABELS);
		sl->SetName(name);
		gs->Add(sl, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
		row++;
	};

	addVol("Master:", "vol_master", 100, 0, 100);
	addVol("FM:",     "vol_fm",     128, 0, 128);
	addVol("PSG:",    "vol_ssg",    128, 0, 128);
	addVol("ADPCM:",  "vol_adpcm",  128, 0, 128);
	addVol("PCM:",    "vol_pcm",    128, 0, 128);
	addVol("Rhythm:", "vol_rhythm", 128, 0, 128);
	addVol("CD-DA:",  "davolume",   128, 0, 255);
	addVol("MIDI:",   "vol_midi",   128, 0, 128);
	addVol("Seek:",   "MOTORVOL",   0,   0, 100);

	/* Beep Volume */
	gs->Add(new wxStaticText(page, wxID_ANY, "Beep Volume:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *hsBeep = new wxBoxSizer(wxHORIZONTAL);
	m_beepvol[0] = new wxRadioButton(page, wxID_ANY, "OFF", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	m_beepvol[1] = new wxRadioButton(page, wxID_ANY, "Low");
	m_beepvol[2] = new wxRadioButton(page, wxID_ANY, "Mid");
	m_beepvol[3] = new wxRadioButton(page, wxID_ANY, "High");
	for (int i = 0; i < 4; i++) hsBeep->Add(m_beepvol[i], 0, wxRIGHT, 4);
	gs->Add(hsBeep, wxGBPosition(row, 1), wxGBSpan(1, 3), wxALIGN_CENTER_VERTICAL);
	row++;

#if defined(SUPPORT_WAB)
	auto *cbWab = new wxCheckBox(page, wxID_ANY, "CRT relay sound");
	cbWab->SetName("wabasw");
	gs->Add(cbWab, wxGBPosition(row++, 0), wxGBSpan(1, 4));
#endif
	auto *cbSeek = new wxCheckBox(page, wxID_ANY, "HDD seek sound");
	cbSeek->SetName("Seek_Snd");
	gs->Add(cbSeek, wxGBPosition(row++, 0), wxGBSpan(1, 4));

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}

wxPanel *PrefFrame::BuildSnd14Page(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addVol = [&](const char *label, const char *name) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *sl = new wxSlider(page, wxID_ANY, 12, 0, 15,
		                        wxDefaultPosition, wxSize(200, -1), wxSL_HORIZONTAL | wxSL_LABELS);
		sl->SetName(name);
		gs->Add(sl, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
		row++;
	};

	addVol("14L:",  "vol14_0");
	addVol("14R:",  "vol14_1");
	addVol("F2:",   "vol14_2");
	addVol("F4:",   "vol14_3");
	addVol("F8:",   "vol14_4");
	addVol("F16:",  "vol14_5");

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}
static const char *s_io26[]  = {"0x088", "0x188"};
static const char *s_int26[] = {"INT0", "INT41", "INT5", "INT6"};
static const char *s_rom26[] = {"C8000", "CC000", "D0000", "D4000", "Non-connect"};

static const char *s_io86[]  = {"0x188", "0x288"};
static const char *s_int86[] = {"INT0", "INT41", "INT5", "INT6"};
static const char *s_id86[]  = {"0x", "1x", "2x", "3x", "4x", "5x", "6x", "7x"};

wxPanel *PrefFrame::BuildSnd26Page(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addChoice = [&](const char *label, const char *name, const char **items, int count) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *ch = new wxChoice(page, wxID_ANY);
		for (int i = 0; i < count; i++) ch->Append(items[i]);
		ch->SetName(name);
		ch->SetSelection(0);
		gs->Add(ch, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxEXPAND);
	};

	addChoice("I/O Port:", "SND26IO",  s_io26,  (int)WXSIZEOF(s_io26));
	addChoice("Interrupt:", "SND26INT", s_int26, (int)WXSIZEOF(s_int26));
	addChoice("ROM Address:","SND26ROM", s_rom26, (int)WXSIZEOF(s_rom26));

	m_snd26Dipsw = new BmpDipSwPanel(page);
	gs->Add(m_snd26Dipsw, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxALIGN_CENTER);

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}

wxPanel *PrefFrame::BuildSnd86Page(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addChoice = [&](const char *label, const char *name, const char **items, int count) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *ch = new wxChoice(page, wxID_ANY);
		for (int i = 0; i < count; i++) ch->Append(items[i]);
		ch->SetName(name);
		ch->SetSelection(0);
		gs->Add(ch, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxEXPAND);
	};

	addChoice("I/O Port:", "SND86IO",  s_io86,  (int)WXSIZEOF(s_io86));
	addChoice("Interrupt:", "SND86INTA", s_int86, (int)WXSIZEOF(s_int86));
	addChoice("ID:",        "SND86ID",  s_id86,  (int)WXSIZEOF(s_id86));

	auto *cbRom = new wxCheckBox(page, wxID_ANY, "Enable ROM");
	cbRom->SetName("SND86ROM");
	gs->Add(cbRom, wxGBPosition(row++, 0), wxGBSpan(1, 4));

	auto *cbInt = new wxCheckBox(page, wxID_ANY, "Enable Interrupt");
	cbInt->SetName("SND86INT");
	gs->Add(cbInt, wxGBPosition(row++, 0), wxGBSpan(1, 4));

	m_snd86Dipsw = new BmpDipSwPanel(page);
	gs->Add(m_snd86Dipsw, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxALIGN_CENTER);

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}

static const char *s_io118[]  = {"0x088", "0x188", "0x288", "0x388"};
static const char *s_id118[]  = {"0x", "1x", "2x", "3x", "4x", "5x", "6x", "7x", "8x"};
static const char *s_dma118[] = {"DMA0", "DMA1", "DMA3"};
static const char *s_int118f[] = {"INT0(IRQ3)", "INT41(IRQ10)", "INT5(IRQ12)", "INT6(IRQ13)"};
static const char *s_int118p[] = {"INT0(IRQ3)", "INT1(IRQ5)", "INT41(IRQ10)", "INT5(IRQ12)"};
static const char *s_int118m[] = {"Disable", "INT41(IRQ10)"};

wxPanel *PrefFrame::BuildSndWSSPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addChoice = [&](const char *label, const char *name, const char **items, int count) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *ch = new wxChoice(page, wxID_ANY);
		for (int i = 0; i < count; i++) ch->Append(items[i]);
		ch->SetName(name);
		ch->SetSelection(0);
		gs->Add(ch, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxEXPAND);
	};

	addChoice("ID:",        "SNDWSSID",  s_id118,  (int)WXSIZEOF(s_id118));
	addChoice("DMA:",       "SNDWSSDMA", s_dma118, (int)WXSIZEOF(s_dma118));
	addChoice("Interrupt:", "SNDWSSINT", s_int118p,(int)WXSIZEOF(s_int118p));

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}

static const char *s_iosb16[] = {"0xD2", "0xD4", "0xD6", "0xD8", "0xDA", "0xDC", "0xDE"};
static const char *s_dmasb16[] = {"DMA0", "DMA3"};
static const char *s_intsb16[] = {"INT0(IRQ3)", "INT1(IRQ5)", "INT41(IRQ10)", "INT5(IRQ12)"};

wxPanel *PrefFrame::BuildSnd118Page(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addChoice = [&](const char *label, const char *name, const char **items, int count) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *ch = new wxChoice(page, wxID_ANY);
		for (int i = 0; i < count; i++) ch->Append(items[i]);
		ch->SetName(name);
		ch->SetSelection(0);
		gs->Add(ch, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxEXPAND);
	};

	addChoice("I/O Port:", "SND118IO",   s_io118,  (int)WXSIZEOF(s_io118));
	addChoice("ID:",       "SND118ID",   s_id118,  (int)WXSIZEOF(s_id118));
	addChoice("DMA:",      "SND118DMA",  s_dma118, (int)WXSIZEOF(s_dma118));
	addChoice("INT (FM):", "SND118INTF", s_int118f,(int)WXSIZEOF(s_int118f));
	addChoice("INT (PCM):","SND118INTP", s_int118p,(int)WXSIZEOF(s_int118p));
	addChoice("INT (MIDI):","SND118INTM",s_int118m,(int)WXSIZEOF(s_int118m));

	auto *cbRom = new wxCheckBox(page, wxID_ANY, "Enable ROM");
	cbRom->SetName("SND118ROM");
	gs->Add(cbRom, wxGBPosition(row++, 0), wxGBSpan(1, 4));

	m_snd118Dipsw = new BmpDipSwPanel(page);
	gs->Add(m_snd118Dipsw, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxALIGN_CENTER);

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}

wxPanel *PrefFrame::BuildSndSB16Page(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addChoice = [&](const char *label, const char *name, const char **items, int count) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *ch = new wxChoice(page, wxID_ANY);
		for (int i = 0; i < count; i++) ch->Append(items[i]);
		ch->SetName(name);
		ch->SetSelection(0);
		gs->Add(ch, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxEXPAND);
	};

	addChoice("I/O Port:", "SNDSB16IO", s_iosb16, (int)WXSIZEOF(s_iosb16));
	addChoice("DMA:",      "SNDSB16DMA",s_dmasb16,(int)WXSIZEOF(s_dmasb16));
	addChoice("Interrupt:","SNDSB16INT",s_intsb16,(int)WXSIZEOF(s_intsb16));

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}

wxPanel *PrefFrame::BuildSndSpbPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addChoice = [&](const char *label, const char *name, const char **items, int count) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *ch = new wxChoice(page, wxID_ANY);
		for (int i = 0; i < count; i++) ch->Append(items[i]);
		ch->SetName(name);
		ch->SetSelection(0);
		gs->Add(ch, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxEXPAND);
	};

	addChoice("I/O Port:", "SPBIO",  s_io26,  (int)WXSIZEOF(s_io26));
	addChoice("Interrupt:", "SPBINT", s_int26, (int)WXSIZEOF(s_int26));
	addChoice("ROM Address:","SPBROM", s_rom26, (int)WXSIZEOF(s_rom26));

	gs->Add(new wxStaticText(page, wxID_ANY, "VR Level:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *slVr = new wxSlider(page, wxID_ANY, 24, 0, 24,
	                          wxDefaultPosition, wxSize(200, -1), wxSL_HORIZONTAL | wxSL_LABELS);
	slVr->SetName("SPBVRLEVEL");
	gs->Add(slVr, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxEXPAND);

	auto *cbRev = new wxCheckBox(page, wxID_ANY, "Reverse Phase");
	cbRev->SetName("SPBREVERSE");
	gs->Add(cbRev, wxGBPosition(row++, 0), wxGBSpan(1, 4));

	m_sndSpbDipsw = new BmpDipSwPanel(page);
	gs->Add(m_sndSpbDipsw, wxGBPosition(row++, 1), wxGBSpan(1, 3), wxALIGN_CENTER);

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}

wxPanel *PrefFrame::BuildSndJoyPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addCheck = [&](const char *label, const char *name) {
		auto *cb = new wxCheckBox(page, wxID_ANY, label);
		cb->SetName(name);
		gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	};

	addCheck("Enable Joypad 1", "JOYPAD1");
	addCheck("Use POV as XY",    "PAD1_POVXY");
#if defined(SUPPORT_GAMEPORT)
	addCheck("Enable GamePort",  "PAD1_GAMEPORT");
	addCheck("Analog Joypad",    "PAD1_ANALOG");
#endif

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	return page;
}

#if defined(SUPPORT_FMGEN)
wxPanel *PrefFrame::BuildSndFMGenPage(wxNotebook *nb)
{
	wxPanel *page = new wxPanel(nb, wxID_ANY);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);
	auto *cb = new wxCheckBox(page, wxID_ANY, "Use fmgen (high quality FM generator)");
	cb->SetName("USEFMGEN");
	vs->Add(cb, 0, wxALL, 16);
	page->SetSizer(vs);
	return page;
}
#endif

wxPanel *PrefFrame::BuildInputPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	/* Keyboard type */
	gs->Add(new wxStaticText(page, wxID_ANY, "Keyboard:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *kbd = new wxChoice(page, wxID_ANY);
	kbd->SetName("keyboard");
	kbd->Append("Keyboard");
	kbd->Append("JoyKey-1");
	kbd->Append("JoyKey-2");
	kbd->SetSelection(0);
	gs->Add(kbd, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* Joypad */
	gs->Add(new wxStaticText(page, wxID_ANY, "Joypad device:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *joy = new wxChoice(page, wxID_ANY);
	joy->SetName("joypad");
	joy->Append("None");
	joy->SetSelection(0);
	gs->Add(joy, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* F12 key */
	gs->Add(new wxStaticText(page, wxID_ANY, "F12 key:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *f12 = new wxChoice(page, wxID_ANY);
	f12->SetName("F12KEY");
	for (auto &s : {"capture Mouse", "Copy key", "Stop key", "Tenkey =", "Tenkey ,", "User1", "User2", "No wait"})
		f12->Append(s);
	f12->SetSelection(0);
	gs->Add(f12, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* Mouse speed */
	gs->Add(new wxStaticText(page, wxID_ANY, "Mouse speed:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *mspd = new wxChoice(page, wxID_ANY);
	mspd->SetName("Mouse_sp");
	for (auto &s : {"x1/4", "x1/2", "x1", "x2", "x4"}) mspd->Append(s);
	mspd->SetSelection(2);
	gs->Add(mspd, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	auto addCheck = [&](const char *label, const char *name) {
		auto *cb = new wxCheckBox(page, wxID_ANY, label);
		cb->SetName(name);
		gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	};
	addCheck("Swap PageUp/PageDown", "XSHIFT"); /* XSHIFT used for swap in some ports */
	addCheck("Drag and Drop",        "DragDrop");
	addCheck("Rapid Button",         "btnRAPID");
	addCheck("Mouse Rapid",          "MS_RAPID");

	gs->AddGrowableCol(1, 1);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);
	vs->Add(gs, 0, wxEXPAND | wxALL, 8);
	vs->AddStretchSpacer(1);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildFddPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	for (int i = 0; i < 4; i++) {
		auto *cb = new wxCheckBox(page, wxID_ANY,
		    wxString::Format("FD Drive %d equipped", i + 1));
		cb->SetName(wxString::Format("FDDRIVE%d", i + 1));
		gs->Add(cb, wxGBPosition(row, 0), wxGBSpan(1, 4), wxALIGN_CENTER_VERTICAL);
		row++;
	}

	{
		auto *cb = new wxCheckBox(page, wxID_ANY, "Use 1.44MB FD");
		cb->SetName("USE144FD");
		gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	}

	auto *seeksnd = new wxCheckBox(page, wxID_ANY, "FDD Seek Sound");
	seeksnd->SetName("Seek_Snd");
	gs->Add(seeksnd, wxGBPosition(row++, 0), wxGBSpan(1, 4));

	gs->Add(new wxStaticText(page, wxID_ANY, "Seek Volume:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *seekvol = new wxSlider(page, wxID_ANY, 50, 0, 100,
	                             wxDefaultPosition, wxSize(200, -1));
	seekvol->SetName("Seek_Vol");
	gs->Add(seekvol, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	page->GetSizer()->AddStretchSpacer(1);
	page->GetSizer()->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	return page;
}

wxPanel *PrefFrame::BuildHddPage(wxNotebook *nb)
{
	wxPanel *page = new wxPanel(nb, wxID_ANY);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);

	wxNotebook *snb = new wxNotebook(page, wxID_ANY);

#if defined(SUPPORT_IDEIO)
	/* ---- IDE sub-tab ---- */
	{
		wxScrolledWindow *ide = new wxScrolledWindow(snb, wxID_ANY, wxDefaultPosition,
		                                             wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
		ide->SetScrollRate(0, 20);
		auto *gs = new wxGridBagSizer(4, 8);
		int row = 0;

		static const char *idelabels[4] = {
			"Master-Primary:", "Master-Secondary:", "Slave-Primary:", "Slave-Secondary:"
		};
		for (int i = 0; i < 4; i++) {
			gs->Add(new wxStaticText(ide, wxID_ANY, idelabels[i]),
			        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
			wxChoice *type = new wxChoice(ide, wxID_ANY);
			type->Append("None");
			type->Append("HDD");
			type->Append("CD-ROM");
			type->SetName(wxString::Format("IDE%dTYPE", i + 1));
			gs->Add(type, wxGBPosition(row, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

			auto *eq = new wxCheckBox(ide, wxID_ANY, "Equipped");
			eq->SetName(wxString::Format("IDE%dEQUIP", i + 1));
			gs->Add(eq, wxGBPosition(row, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
			row++;
		}

		auto *asyncCd = new wxCheckBox(ide, wxID_ANY, "Async CD-ROM Access");
		asyncCd->SetName("CD_ASYNC");
		gs->Add(asyncCd, wxGBPosition(row++, 0), wxGBSpan(1, 3));

		auto *ideBios = new wxCheckBox(ide, wxID_ANY, "Use IDE BIOS");
		ideBios->SetName("IDE_BIOS");
		gs->Add(ideBios, wxGBPosition(row++, 0), wxGBSpan(1, 3));

		auto *autoIdeBios = new wxCheckBox(ide, wxID_ANY, "Auto IDE BIOS");
		autoIdeBios->SetName("AIDEBIOS");
		gs->Add(autoIdeBios, wxGBPosition(row++, 0), wxGBSpan(1, 3));

#if defined(SUPPORT_LIBCDIO)
		{
			auto *cb = new wxCheckBox(ide, wxID_ANY, "Enable libcdio");
			cb->SetName("LIBCDIO");
			gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 3));
		}
#endif

		gs->AddGrowableCol(1, 1);
		auto *boxSz = new wxBoxSizer(wxVERTICAL);
		boxSz->Add(gs, 0, wxEXPAND | wxALL, 8);
		ide->SetSizer(boxSz);
		snb->AddPage(ide, "IDE");
	}
#else
	/* SASI fallback */
	{
		wxScrolledWindow *sasi = new wxScrolledWindow(snb, wxID_ANY, wxDefaultPosition,
		                                              wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
		sasi->SetScrollRate(0, 20);
		auto *gs = new wxGridBagSizer(4, 8);
		int row = 0;
		for (int i = 0; i < 2; i++) {
			gs->Add(new wxStaticText(sasi, wxID_ANY,
			    wxString::Format("SASI HDD %d:", i + 1)),
			    wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
			auto *eq = new wxCheckBox(sasi, wxID_ANY, "Equipped");
			eq->SetName(wxString::Format("SASI%dEQUIP", i + 1));
			gs->Add(eq, wxGBPosition(row, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
			row++;
		}
		auto *boxSz = new wxBoxSizer(wxVERTICAL);
		boxSz->Add(gs, 0, wxEXPAND | wxALL, 8);
		sasi->SetSizer(boxSz);
		snb->AddPage(sasi, "SASI");
	}
#endif

#if defined(SUPPORT_SCSI)
	/* ---- SCSI sub-tab ---- */
	{
		wxScrolledWindow *scsi = new wxScrolledWindow(snb, wxID_ANY, wxDefaultPosition,
		                                              wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
		scsi->SetScrollRate(0, 20);
		auto *gs = new wxGridBagSizer(4, 8);
		int row = 0;
		for (int i = 0; i < 4; i++) {
			gs->Add(new wxStaticText(scsi, wxID_ANY,
			    wxString::Format("ID%d:", i)),
			    wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
			wxChoice *type = new wxChoice(scsi, wxID_ANY);
			type->Append("None");
			type->Append("HDD");
			type->SetName(wxString::Format("SCSI%dTYPE", i));
			gs->Add(type, wxGBPosition(row, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

			auto *eq = new wxCheckBox(scsi, wxID_ANY, "Equipped");
			eq->SetName(wxString::Format("SCSI%dEQUIP", i + 1));
			gs->Add(eq, wxGBPosition(row, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
			row++;
		}
		gs->AddGrowableCol(1, 1);
		auto *boxSz = new wxBoxSizer(wxVERTICAL);
		boxSz->Add(gs, 0, wxEXPAND | wxALL, 8);
		scsi->SetSizer(boxSz);
		snb->AddPage(scsi, "SCSI");
	}
#endif

	vs->Add(snb, 1, wxEXPAND | wxALL, 4);
	vs->AddStretchSpacer(0);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

/* Helper: build one COM port box (vertical layout) */
static wxStaticBoxSizer *BuildComPortBox(wxWindow *parent, int portnum,
                                          const char *portName,
                                          const char *bpsName,
                                          const char *dsrName)
{
	wxString title = wxString::Format("COM%d", portnum);
	auto *box = new wxStaticBoxSizer(wxVERTICAL, parent, title);
	auto *gs  = new wxGridBagSizer(3, 6);
	int row = 0;

	const char *portOpts[] = {"N/C","COM1","COM2","COM3","COM4","MIDI","PenTab","Pipe","File"};
	gs->Add(new wxStaticText(parent, wxID_ANY, "Port:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *cp = new wxChoice(parent, wxID_ANY);
	cp->SetName(portName);
	for (auto &s : portOpts) cp->Append(s);
	gs->Add(cp, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	gs->Add(new wxStaticText(parent, wxID_ANY, "bps:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *cb = new wxChoice(parent, wxID_ANY);
	cb->SetName(bpsName);
	for (auto &s : {"110","300","600","1200","2400","4800","9600","14400","19200","28800","38400","57600","115200"})
		cb->Append(s);
	cb->SetSelection(8); /* default 19200 */
	gs->Add(cb, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	gs->Add(new wxStaticText(parent, wxID_ANY, "Data bits:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *cdb = new wxChoice(parent, wxID_ANY);
	cdb->SetName(wxString::Format("com%d_dbits", portnum));
	for (auto &s : {"5","6","7","8"}) cdb->Append(s);
	cdb->SetSelection(3); /* default 8 */
	gs->Add(cdb, wxGBPosition(row, 1), wxDefaultSpan);

	gs->Add(new wxStaticText(parent, wxID_ANY, "Parity:"),
	        wxGBPosition(row, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *cpar = new wxChoice(parent, wxID_ANY);
	cpar->SetName(wxString::Format("com%d_parity", portnum));
	for (auto &s : {"None","Odd","Even","Mark","Space"}) cpar->Append(s);
	cpar->SetSelection(2); /* default Even */
	gs->Add(cpar, wxGBPosition(row, 3), wxDefaultSpan);
	row++;

	gs->Add(new wxStaticText(parent, wxID_ANY, "Stop bits:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *csb = new wxChoice(parent, wxID_ANY);
	csb->SetName(wxString::Format("com%d_sbits", portnum));
	for (auto &s : {"1","1.5","2"}) csb->Append(s);
	csb->SetSelection(0); /* default 1 */
	gs->Add(csb, wxGBPosition(row, 1), wxDefaultSpan);
	row++;

	auto *dsr = new wxCheckBox(parent, wxID_ANY, "H/W DSR check");
	dsr->SetName(dsrName);
	dsr->SetValue(true);
	gs->Add(dsr, wxGBPosition(row, 0), wxGBSpan(1, 4));
	row++;

	gs->AddGrowableCol(1, 1);
	gs->AddGrowableCol(3, 1);
	box->Add(gs, 0, wxEXPAND | wxALL, 4);
	return box;
}

wxPanel *PrefFrame::BuildMidiPage(wxNotebook *nb)
{
	static const char *s_ioports[] = {
		"C0D0","C4D0","C8D0","CCD0","D0D0","D4D0","D8D0","DCD0",
		"E0D0","E4D0","E8D0","ECD0","F0D0","F4D0","F8D0","FCD0"
	};
	static const char *s_irqs[] = { "INT0","INT1","INT2","INT5" };
	static const char *s_modules[] = {
		"none","MT-32","CM-32L","CM-64","CM-300",
		"CM-500(LA)","CM-500(GS)","SC-55","SC-88","LA","GM","GS","XG"
	};

	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition,
	                                               wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);

	/* ---- MPU-PC98II ---- */
	auto *mpuBox = new wxStaticBoxSizer(wxVERTICAL, page, "MPU-PC98II");
	auto *mpuGs  = new wxGridBagSizer(4, 8);
	int row = 0;

	auto *mpuEn = new wxCheckBox(page, wxID_ANY, "Enable");
	mpuEn->SetName("USEMPU98");
	mpuGs->Add(mpuEn, wxGBPosition(row++, 0), wxGBSpan(1, 4));

	/* IO port */
	mpuGs->Add(new wxStaticText(page, wxID_ANY, "IO port:"),
	           wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *mpuPort = new wxChoice(page, wxID_ANY);
	mpuPort->SetName("MPU_PORT");
	for (auto &s : s_ioports) mpuPort->Append(s);
	mpuPort->SetSelection(8); /* E0D0 */
	mpuGs->Add(mpuPort, wxGBPosition(row, 1), wxDefaultSpan);

	/* Interrupt */
	mpuGs->Add(new wxStaticText(page, wxID_ANY, "Interrupt:"),
	           wxGBPosition(row, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *mpuIrq = new wxChoice(page, wxID_ANY);
	mpuIrq->SetName("MPU_IRQ");
	for (auto &s : s_irqs) mpuIrq->Append(s);
	mpuIrq->SetSelection(2); /* INT2 */
	mpuGs->Add(mpuIrq, wxGBPosition(row, 3), wxDefaultSpan);
	row++;

	/* MIDIOUT */
	mpuGs->Add(new wxStaticText(page, wxID_ANY, "MIDIOUT:"),
	           wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *mpuOut = new wxChoice(page, wxID_ANY);
	mpuOut->SetName("MPU_OUT");
	mpuOut->Append("N/C");
	mpuOut->Append("VERMOUTH");
	mpuOut->SetSelection(0);
	mpuGs->Add(mpuOut, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* MIDIIN */
	mpuGs->Add(new wxStaticText(page, wxID_ANY, "MIDIIN:"),
	           wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *mpuIn = new wxChoice(page, wxID_ANY);
	mpuIn->SetName("MPU_IN");
	mpuIn->Append("N/C");
	mpuIn->SetSelection(0);
	mpuGs->Add(mpuIn, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* Module */
	mpuGs->Add(new wxStaticText(page, wxID_ANY, "Module:"),
	           wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *mpuMdl = new wxChoice(page, wxID_ANY);
	mpuMdl->SetName("MPU_MDL");
	for (auto &s : s_modules) mpuMdl->Append(s);
	mpuMdl->SetSelection(0); /* none */
	mpuGs->Add(mpuMdl, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* Use program define file */
	auto *mpuDefEn = new wxCheckBox(page, wxID_ANY, "Use program define file (MIMPI define)");
	mpuDefEn->SetName("MPU_DEF_EN");
	mpuGs->Add(mpuDefEn, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	{
		auto *defBox = new wxBoxSizer(wxHORIZONTAL);
		auto *mpuDefPath = new wxTextCtrl(page, wxID_ANY, "");
		mpuDefPath->SetName("MPU_DEF");
		defBox->Add(mpuDefPath, 1, wxEXPAND);
		auto *btnBrowse = new wxButton(page, wxID_ANY, "...", wxDefaultPosition, wxSize(30,-1));
		btnBrowse->Bind(wxEVT_BUTTON, [mpuDefPath](wxCommandEvent &) {
			wxFileDialog dlg(mpuDefPath, "Select define file", "", "",
			    "Define files (*.def)|*.def|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
			if (dlg.ShowModal() == wxID_OK)
				mpuDefPath->SetValue(dlg.GetPath());
		});
		defBox->Add(btnBrowse, 0, wxLEFT, 4);
		mpuGs->Add(defBox, wxGBPosition(row++, 0), wxGBSpan(1, 4), wxEXPAND);
	}

	mpuGs->AddGrowableCol(1, 1);
	mpuGs->AddGrowableCol(3, 1);
	mpuBox->Add(mpuGs, 0, wxEXPAND | wxALL, 4);
	vs->Add(mpuBox, 0, wxEXPAND | wxALL, 6);

#if defined(SUPPORT_SMPU98)
	/* ---- S-MPUI ---- */
	auto *smpuBox = new wxStaticBoxSizer(wxVERTICAL, page, "S-MPUI");
	auto *smpuGs  = new wxGridBagSizer(4, 8);
	int srow = 0;

	auto *smpuEn = new wxCheckBox(page, wxID_ANY, "Enable");
	smpuEn->SetName("USE_SMPU");
	smpuGs->Add(smpuEn, wxGBPosition(srow, 0), wxGBSpan(1, 2));
	auto *smpuMuteB = new wxCheckBox(page, wxID_ANY, "Mute port B during MPU-401 mode");
	smpuMuteB->SetName("SMPUMUTB");
	smpuGs->Add(smpuMuteB, wxGBPosition(srow++, 2), wxGBSpan(1, 2));

	smpuGs->Add(new wxStaticText(page, wxID_ANY, "IO port:"),
	            wxGBPosition(srow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuPort = new wxChoice(page, wxID_ANY);
	smpuPort->SetName("SMPU_PORT");
	for (auto &s : s_ioports) smpuPort->Append(s);
	smpuPort->SetSelection(8);
	smpuGs->Add(smpuPort, wxGBPosition(srow, 1), wxDefaultSpan);
	smpuGs->Add(new wxStaticText(page, wxID_ANY, "Interrupt:"),
	            wxGBPosition(srow, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuIrq = new wxChoice(page, wxID_ANY);
	smpuIrq->SetName("SMPU_IRQ");
	for (auto &s : s_irqs) smpuIrq->Append(s);
	smpuIrq->SetSelection(2);
	smpuGs->Add(smpuIrq, wxGBPosition(srow++, 3), wxDefaultSpan);

	smpuGs->AddGrowableCol(1, 1);
	smpuGs->AddGrowableCol(3, 1);
	smpuBox->Add(smpuGs, 0, wxEXPAND | wxALL, 4);

	/* Port A sub-group */
	auto *smpuAGroup = new wxStaticBoxSizer(wxVERTICAL, page, "Port A");
	auto *sagGs = new wxGridBagSizer(4, 8);
	int sarow = 0;

	sagGs->Add(new wxStaticText(page, wxID_ANY, "MIDIOUT:"),
	           wxGBPosition(sarow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuAOut = new wxChoice(page, wxID_ANY);
	smpuAOut->SetName("SMPUA_OUT"); smpuAOut->Append("N/C"); smpuAOut->Append("VERMOUTH");
	smpuAOut->SetSelection(0);
	sagGs->Add(smpuAOut, wxGBPosition(sarow++, 1), wxGBSpan(1, 3), wxEXPAND);

	sagGs->Add(new wxStaticText(page, wxID_ANY, "MIDIIN:"),
	           wxGBPosition(sarow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuAIn = new wxChoice(page, wxID_ANY);
	smpuAIn->SetName("SMPUA_IN"); smpuAIn->Append("N/C"); smpuAIn->SetSelection(0);
	sagGs->Add(smpuAIn, wxGBPosition(sarow++, 1), wxGBSpan(1, 3), wxEXPAND);

	sagGs->Add(new wxStaticText(page, wxID_ANY, "Module:"),
	           wxGBPosition(sarow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuAMdl = new wxChoice(page, wxID_ANY);
	smpuAMdl->SetName("SMPUA_MDL");
	for (auto &s : s_modules) smpuAMdl->Append(s);
	smpuAMdl->SetSelection(0);
	sagGs->Add(smpuAMdl, wxGBPosition(sarow++, 1), wxGBSpan(1, 3), wxEXPAND);

	auto *smpuADefEn = new wxCheckBox(page, wxID_ANY, "Use program define file (MIMPI define)");
	smpuADefEn->SetName("SMPUA_DEF_EN");
	sagGs->Add(smpuADefEn, wxGBPosition(sarow++, 0), wxGBSpan(1, 4));
	{
		auto *defBox = new wxBoxSizer(wxHORIZONTAL);
		auto *smpuADefPath = new wxTextCtrl(page, wxID_ANY, "");
		smpuADefPath->SetName("SMPUA_DEF");
		defBox->Add(smpuADefPath, 1, wxEXPAND);
		auto *btnBrowse = new wxButton(page, wxID_ANY, "...", wxDefaultPosition, wxSize(30,-1));
		btnBrowse->Bind(wxEVT_BUTTON, [smpuADefPath](wxCommandEvent &) {
			wxFileDialog dlg(smpuADefPath, "Select define file", "", "",
			    "Define files (*.def)|*.def|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
			if (dlg.ShowModal() == wxID_OK)
				smpuADefPath->SetValue(dlg.GetPath());
		});
		defBox->Add(btnBrowse, 0, wxLEFT, 4);
		sagGs->Add(defBox, wxGBPosition(sarow++, 0), wxGBSpan(1, 4), wxEXPAND);
	}
	sagGs->AddGrowableCol(1, 1);
	sagGs->AddGrowableCol(3, 1);
	smpuAGroup->Add(sagGs, 0, wxEXPAND | wxALL, 4);
	smpuBox->Add(smpuAGroup, 0, wxEXPAND | wxALL, 4);

	/* Port B sub-group */
	auto *smpuBGroup = new wxStaticBoxSizer(wxVERTICAL, page, "Port B");
	auto *sbgGs = new wxGridBagSizer(4, 8);
	int sbrow = 0;

	sbgGs->Add(new wxStaticText(page, wxID_ANY, "MIDIOUT:"),
	           wxGBPosition(sbrow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuBOut = new wxChoice(page, wxID_ANY);
	smpuBOut->SetName("SMPUB_OUT"); smpuBOut->Append("N/C"); smpuBOut->Append("VERMOUTH");
	smpuBOut->SetSelection(0);
	sbgGs->Add(smpuBOut, wxGBPosition(sbrow++, 1), wxGBSpan(1, 3), wxEXPAND);

	sbgGs->Add(new wxStaticText(page, wxID_ANY, "MIDIIN:"),
	           wxGBPosition(sbrow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuBIn = new wxChoice(page, wxID_ANY);
	smpuBIn->SetName("SMPUB_IN"); smpuBIn->Append("N/C"); smpuBIn->SetSelection(0);
	sbgGs->Add(smpuBIn, wxGBPosition(sbrow++, 1), wxGBSpan(1, 3), wxEXPAND);

	sbgGs->Add(new wxStaticText(page, wxID_ANY, "Module:"),
	           wxGBPosition(sbrow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuBMdl = new wxChoice(page, wxID_ANY);
	smpuBMdl->SetName("SMPUB_MDL");
	for (auto &s : s_modules) smpuBMdl->Append(s);
	smpuBMdl->SetSelection(0);
	sbgGs->Add(smpuBMdl, wxGBPosition(sbrow++, 1), wxGBSpan(1, 3), wxEXPAND);

	auto *smpuBDefEn = new wxCheckBox(page, wxID_ANY, "Use program define file (MIMPI define)");
	smpuBDefEn->SetName("SMPUB_DEF_EN");
	sbgGs->Add(smpuBDefEn, wxGBPosition(sbrow++, 0), wxGBSpan(1, 4));
	{
		auto *defBox = new wxBoxSizer(wxHORIZONTAL);
		auto *smpuBDefPath = new wxTextCtrl(page, wxID_ANY, "");
		smpuBDefPath->SetName("SMPUB_DEF");
		defBox->Add(smpuBDefPath, 1, wxEXPAND);
		auto *btnBrowse = new wxButton(page, wxID_ANY, "...", wxDefaultPosition, wxSize(30,-1));
		btnBrowse->Bind(wxEVT_BUTTON, [smpuBDefPath](wxCommandEvent &) {
			wxFileDialog dlg(smpuBDefPath, "Select define file", "", "",
			    "Define files (*.def)|*.def|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
			if (dlg.ShowModal() == wxID_OK)
				smpuBDefPath->SetValue(dlg.GetPath());
		});
		defBox->Add(btnBrowse, 0, wxLEFT, 4);
		sbgGs->Add(defBox, wxGBPosition(sbrow++, 0), wxGBSpan(1, 4), wxEXPAND);
	}
	sbgGs->AddGrowableCol(1, 1);
	sbgGs->AddGrowableCol(3, 1);
	smpuBGroup->Add(sbgGs, 0, wxEXPAND | wxALL, 4);
	smpuBox->Add(smpuBGroup, 0, wxEXPAND | wxALL, 4);

	vs->Add(smpuBox, 0, wxEXPAND | wxALL, 6);
#endif

	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildSerialPage(wxNotebook *nb)
{
	wxPanel *page = new wxPanel(nb, wxID_ANY);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);

	wxNotebook *snb = new wxNotebook(page, wxID_ANY);

	/* COM1 tab */
	{
		wxScrolledWindow *tab = new wxScrolledWindow(snb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
		tab->SetScrollRate(0, 20);
		wxBoxSizer *ts = new wxBoxSizer(wxVERTICAL);
		ts->Add(BuildComPortBox(tab, 1, "com1port", "com1_bps", "com1_dsr"), 0, wxEXPAND | wxALL, 4);
		tab->SetSizer(ts);
		snb->AddPage(tab, "COM1");
	}

	/* PC-9861K tab (T.B.D.) */
	{
		wxPanel *tab = new wxPanel(snb, wxID_ANY);
		wxBoxSizer *ts = new wxBoxSizer(wxVERTICAL);
		ts->Add(new wxStaticText(tab, wxID_ANY, "PC-9861K: T.B.D."), 0, wxALL, 8);
		tab->SetSizer(ts);
		snb->AddPage(tab, "PC-9861K");
	}

	/* CH.1 tab (PC-9861K CH.1 = com[1]) */
	{
		wxScrolledWindow *tab = new wxScrolledWindow(snb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
		tab->SetScrollRate(0, 20);
		wxBoxSizer *ts = new wxBoxSizer(wxVERTICAL);
		ts->Add(BuildComPortBox(tab, 1, "com2port", "com2_bps", "com2_dsr"), 0, wxEXPAND | wxALL, 4);
		tab->SetSizer(ts);
		snb->AddPage(tab, "CH.1");
	}

	/* CH.2 tab (PC-9861K CH.2 = com[2]) */
	{
		wxScrolledWindow *tab = new wxScrolledWindow(snb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
		tab->SetScrollRate(0, 20);
		wxBoxSizer *ts = new wxBoxSizer(wxVERTICAL);
		ts->Add(BuildComPortBox(tab, 1, "com3port", "com3_bps", "com3_dsr"), 0, wxEXPAND | wxALL, 4);
		tab->SetSizer(ts);
		snb->AddPage(tab, "CH.2");
	}

	/* Parallel tab (T.B.D.) */
	{
		wxPanel *tab = new wxPanel(snb, wxID_ANY);
		wxBoxSizer *ts = new wxBoxSizer(wxVERTICAL);
		ts->Add(new wxStaticText(tab, wxID_ANY, "Parallel: T.B.D."), 0, wxALL, 8);
		tab->SetSizer(ts);
		snb->AddPage(tab, "Parallel");
	}

	vs->Add(snb, 1, wxEXPAND | wxALL, 4);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildNetworkPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	/* General */
	vs->Add(new wxStaticText(page, wxID_ANY, "General"), 0, wxLEFT | wxTOP, 8);
	gs->Add(new wxStaticText(page, wxID_ANY, "TAP device name:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *tapname = new wxTextCtrl(page, wxID_ANY, "");
	tapname->SetName("NP2NETTAP");
	gs->Add(tapname, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

#if defined(SUPPORT_LGY98)
	vs->Add(gs, 0, wxEXPAND | wxALL, 4);

	wxGridBagSizer *lgygs = new wxGridBagSizer(4, 8);
	int lrow = 0;
	vs->Add(new wxStaticText(page, wxID_ANY, "LGY-98"), 0, wxLEFT | wxTOP, 8);

	auto *lgyEn = new wxCheckBox(page, wxID_ANY, "Enable");
	lgyEn->SetName("USELGY98");
	lgygs->Add(lgyEn, wxGBPosition(lrow, 0), wxGBSpan(1, 4));
	lrow++;

	lgygs->Add(new wxStaticText(page, wxID_ANY, "I/O port:"),
	           wxGBPosition(lrow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *lgyio = new wxChoice(page, wxID_ANY);
	lgyio->SetName("LGY98_IO");
	for (auto &s : {"00D0","10D0","20D0","30D0","40D0","50D0","60D0","70D0"})
		lgyio->Append(s);
	lgyio->SetSelection(1); /* default 10D0 */
	lgygs->Add(lgyio, wxGBPosition(lrow, 1), wxDefaultSpan);

	lgygs->Add(new wxStaticText(page, wxID_ANY, "Interrupt:"),
	           wxGBPosition(lrow, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *lgyirq = new wxChoice(page, wxID_ANY);
	lgyirq->SetName("LGY98IRQ");
	for (auto &s : {"INT0","INT1","INT2","INT5"}) lgyirq->Append(s);
	lgyirq->SetSelection(1); /* default INT1 */
	lgygs->Add(lgyirq, wxGBPosition(lrow, 3), wxDefaultSpan);
	lrow++;

	lgygs->AddGrowableCol(1, 1);
	vs->Add(lgygs, 0, wxEXPAND | wxALL, 4);
#else
	gs->AddGrowableCol(1, 1);
	vs->Add(gs, 0, wxEXPAND | wxALL, 4);
	vs->Add(new wxStaticText(page, wxID_ANY, "LGY-98: not supported in this build"),
	        0, wxALL, 8);
#endif

	vs->AddStretchSpacer(1);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildHostdrvPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

#if defined(SUPPORT_HOSTDRV)
	auto *en = new wxCheckBox(page, wxID_ANY, "Enable Hostdrv");
	en->SetName("use_hdrv");
	en->SetValue(true);
	gs->Add(en, wxGBPosition(row, 0), wxGBSpan(1, 3));
	row++;

	gs->Add(new wxStaticText(page, wxID_ANY, "Shared Directory:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *dirpath = new wxTextCtrl(page, wxID_ANY, "");
	dirpath->SetName("hdrvroot");
	gs->Add(dirpath, wxGBPosition(row, 1), wxDefaultSpan, wxEXPAND);
	auto *btnBrowse = new wxButton(page, wxID_ANY, "...");
	btnBrowse->Bind(wxEVT_BUTTON, [dirpath](wxCommandEvent &) {
		wxDirDialog dlg(nullptr, "Select Shared Directory");
		if (dlg.ShowModal() == wxID_OK)
			dirpath->SetValue(dlg.GetPath());
	});
	gs->Add(btnBrowse, wxGBPosition(row, 2), wxDefaultSpan);
	row++;

	/* Permission checkboxes */
	auto *permRead = new wxCheckBox(page, wxID_ANY, "Read");
	auto *permWrite = new wxCheckBox(page, wxID_ANY, "Write");
	auto *permDel   = new wxCheckBox(page, wxID_ANY, "Delete");
	permRead->SetName("hdrv_acc_r");
	permWrite->SetName("hdrv_acc_w");
	permDel->SetName("hdrv_acc_d");
	wxBoxSizer *hs = new wxBoxSizer(wxHORIZONTAL);
	hs->Add(new wxStaticText(page, wxID_ANY, "Permission:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	hs->Add(permRead,  0, wxRIGHT, 8);
	hs->Add(permWrite, 0, wxRIGHT, 8);
	hs->Add(permDel,   0);
	gs->Add(hs, wxGBPosition(row, 0), wxGBSpan(1, 3));
	row++;

#if defined(SUPPORT_HOSTDRVNT)
	auto *forNt = new wxCheckBox(page, wxID_ANY, "Hostdrv for NT");
	forNt->SetName("hdrv_nt");
	gs->Add(forNt, wxGBPosition(row, 0), wxGBSpan(1, 3));
	row++;
#endif

	gs->AddGrowableCol(1, 1);
#else
	gs->Add(new wxStaticText(page, wxID_ANY, "Hostdrv: not supported in this build"),
	        wxGBPosition(row, 0), wxGBSpan(1, 3));
	row++;
#endif

	vs->Add(gs, 0, wxEXPAND | wxALL, 8);
	vs->AddStretchSpacer(1);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildDipswPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);

	/* Visual DIP switch picture */
	auto *pic = new DipswPicPanel(page, m_dipsw);
	m_dipswPanel = pic;
	vs->Add(pic, 0, wxALL | wxCENTER, 8);

	vs->Add(new wxStaticLine(page), 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

	/* Checkbox grid: 3 banks stacked vertically (one row per bank) */
	wxGridBagSizer *gs = new wxGridBagSizer(4, 6);
	for (int bank = 0; bank < 3; bank++) {
		gs->Add(new wxStaticText(page, wxID_ANY,
		    wxString::Format("SW%d:", bank + 1)),
		    wxGBPosition(bank, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		for (int bit = 0; bit < 8; bit++) {
			auto *cb = new wxCheckBox(page, ID_DIPSW_BASE + bank * 8 + bit,
			    wxString::Format("%d", bit + 1));
			m_dipsw[bank][bit] = cb;
			gs->Add(cb, wxGBPosition(bank, 1 + bit), wxDefaultSpan);
		}
	}
	vs->Add(gs, 0, wxALL | wxCENTER, 8);
	vs->AddStretchSpacer(1);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);

	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildCalendarPage(wxNotebook *nb)
{
	wxPanel *page = new wxPanel(nb, wxID_ANY);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);
	wxGridBagSizer *gs = new wxGridBagSizer(6, 8);
	int row = 0;

	/* Real / Virtual radio */
	auto *rbReal = new wxRadioButton(page, wxID_ANY, "Real", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	rbReal->SetName("cal_real");
	gs->Add(rbReal, wxGBPosition(row, 0), wxDefaultSpan);
	auto *rbVir  = new wxRadioButton(page, wxID_ANY, "Virtual");
	rbVir->SetName("cal_vir");
	gs->Add(rbVir, wxGBPosition(row, 1), wxDefaultSpan);
	row++;

	/* Date line: YY / MM / DD */
	gs->Add(new wxStaticText(page, wxID_ANY, "Date (YY/MM/DD):"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *dateBox = new wxBoxSizer(wxHORIZONTAL);
	auto *yyTc = new wxTextCtrl(page, wxID_ANY, "00", wxDefaultPosition, wxSize(40, -1));
	yyTc->SetName("cal_yy");
	auto *mmTc = new wxTextCtrl(page, wxID_ANY, "01", wxDefaultPosition, wxSize(40, -1));
	mmTc->SetName("cal_mm");
	auto *ddTc = new wxTextCtrl(page, wxID_ANY, "01", wxDefaultPosition, wxSize(40, -1));
	ddTc->SetName("cal_dd");
	dateBox->Add(yyTc, 0);
	dateBox->Add(new wxStaticText(page, wxID_ANY, " / "), 0, wxALIGN_CENTER_VERTICAL);
	dateBox->Add(mmTc, 0);
	dateBox->Add(new wxStaticText(page, wxID_ANY, " / "), 0, wxALIGN_CENTER_VERTICAL);
	dateBox->Add(ddTc, 0);
	gs->Add(dateBox, wxGBPosition(row, 1), wxGBSpan(1, 3));
	row++;

	/* Time line: HH : MM : SS */
	gs->Add(new wxStaticText(page, wxID_ANY, "Time (HH:MM:SS):"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *timeBox = new wxBoxSizer(wxHORIZONTAL);
	auto *hhTc = new wxTextCtrl(page, wxID_ANY, "00", wxDefaultPosition, wxSize(40, -1));
	hhTc->SetName("cal_hh");
	auto *miTc = new wxTextCtrl(page, wxID_ANY, "00", wxDefaultPosition, wxSize(40, -1));
	miTc->SetName("cal_mi");
	auto *ssTc = new wxTextCtrl(page, wxID_ANY, "00", wxDefaultPosition, wxSize(40, -1));
	ssTc->SetName("cal_ss");
	timeBox->Add(hhTc, 0);
	timeBox->Add(new wxStaticText(page, wxID_ANY, " : "), 0, wxALIGN_CENTER_VERTICAL);
	timeBox->Add(miTc, 0);
	timeBox->Add(new wxStaticText(page, wxID_ANY, " : "), 0, wxALIGN_CENTER_VERTICAL);
	timeBox->Add(ssTc, 0);
	gs->Add(timeBox, wxGBPosition(row, 1), wxGBSpan(1, 3));
	row++;

	/* Now button: sets virtual time to real-time */
	auto *btnNow = new wxButton(page, wxID_ANY, "Now");
	btnNow->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) {
		UINT8 cbuf[6];
		calendar_getreal(cbuf);
		char tmp[8];
		static const char *nms[6] = {"cal_yy","cal_mm","cal_dd","cal_hh","cal_mi","cal_ss"};
		for (int i = 0; i < 6; i++) {
			UINT8 v = cbuf[i];
			/* BCD: high nibble = tens, low = units */
			snprintf(tmp, sizeof(tmp), "%02X", v);
			if (auto *w = FindWindowByName(nms[i]))
				if (auto *t = wxDynamicCast(w, wxTextCtrl)) t->SetValue(tmp);
		}
	});
	gs->Add(btnNow, wxGBPosition(row, 1), wxDefaultSpan);
	row++;

	vs->Add(gs, 0, wxEXPAND | wxALL, 8);
	vs->AddStretchSpacer(1);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildMiscPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	auto addCheck = [&](const char *label, const char *name) {
		auto *cb = new wxCheckBox(page, wxID_ANY, label);
		cb->SetName(name);
		gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 2));
	};

#if defined(SUPPORT_PCI)
	/* PCI */
	gs->Add(new wxStaticText(page, wxID_ANY, "PCI"),
	        wxGBPosition(row++, 0), wxGBSpan(1, 2), wxALIGN_CENTER_VERTICAL);
	{
		auto *enPci = new wxCheckBox(page, wxID_ANY, "Enable PCI bus");
		enPci->SetName("USE_PCI");
		gs->Add(enPci, wxGBPosition(row++, 0), wxGBSpan(1, 2));

		gs->Add(new wxStaticText(page, wxID_ANY, "PCMC:"),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		wxChoice *pcmc = new wxChoice(page, wxID_ANY);
		pcmc->SetName("PCI_PCMC");
		pcmc->Append("Intel 82434LX");
		pcmc->Append("Intel 82441FX");
		pcmc->Append("VLSI Wildcat");
		pcmc->SetSelection(0);
		gs->Add(pcmc, wxGBPosition(row, 1), wxDefaultSpan, wxEXPAND);
		row++;

		auto *b32 = new wxCheckBox(page, wxID_ANY, "Use BIOS32 (not recommended)");
		b32->SetName("PCI_B32");
		gs->Add(b32, wxGBPosition(row++, 0), wxGBSpan(1, 2));
	}
	gs->Add(new wxStaticLine(page), wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND);
#endif

	addCheck("No Wait (max speed)",     "s_NOWAIT");
	addCheck("Resume on start",         "e_resume");
	addCheck("Save state on exit",      "STATSAVE");
	addCheck("JAST Sound mode",         "jast_snd");

	gs->Add(new wxStaticText(page, wxID_ANY, "Frame Skip:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *skip = new wxSpinCtrl(page, wxID_ANY, "0",
	                            wxDefaultPosition, wxDefaultSize,
	                            wxSP_ARROW_KEYS, 0, 9, 0);
	skip->SetName("SkpFrame");
	gs->Add(skip, wxGBPosition(row, 1), wxDefaultSpan);
	row++;

	/* Font file */
	gs->Add(new wxStaticText(page, wxID_ANY, "Font File:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *fontpath = new wxTextCtrl(page, wxID_ANY, "");
	fontpath->SetName("fontfile");
	gs->Add(fontpath, wxGBPosition(row, 1), wxDefaultSpan, wxEXPAND);
	row++;

	/* Cycle Screenshot */
	gs->Add(new wxStaticLine(page), wxGBPosition(row++, 0), wxGBSpan(1, 2), wxEXPAND | wxTOP | wxBOTTOM, 8);
	gs->Add(new wxStaticText(page, wxID_ANY, "Cycle Screenshot"),
	        wxGBPosition(row++, 0), wxGBSpan(1, 2));

	gs->Add(new wxStaticText(page, wxID_ANY, "Output Path:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *cyclepath = new wxFilePickerCtrl(page, wxID_ANY, "", "Select screenshot file",
	        "PNG files (*.png)|*.png|All files (*.*)|*.*",
	        wxDefaultPosition, wxDefaultSize, wxFLP_SAVE | wxFLP_OVERWRITE_PROMPT | wxFLP_USE_TEXTCTRL);
	cyclepath->SetName("CyclePath");
	gs->Add(cyclepath, wxGBPosition(row++, 1), wxDefaultSpan, wxEXPAND);

	gs->Add(new wxStaticText(page, wxID_ANY, "Interval (ms):"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *cycleint = new wxSpinCtrl(page, wxID_ANY, "3000",
	                                wxDefaultPosition, wxDefaultSize,
	                                wxSP_ARROW_KEYS, 100, 600000, 3000);
	cycleint->SetName("CycleInt");
	gs->Add(cycleint, wxGBPosition(row++, 1), wxDefaultSpan);

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	page->GetSizer()->AddStretchSpacer(1);
	page->GetSizer()->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	return page;
}

/* ------------------------------------------------------------ */
/*  DIP switch: update picture when a checkbox changes          */
/* ------------------------------------------------------------ */

void PrefFrame::OnDipswChange(wxCommandEvent &evt)
{
	int id  = evt.GetId() - ID_DIPSW_BASE;
	int bank = id / 8;
	int bit  = id % 8;
	bool on = evt.IsChecked();

	UINT8 &dw = np2cfg.dipsw[bank];
	if (on) dw |=  (UINT8)(1 << bit);
	else    dw &= ~(UINT8)(1 << bit);

	if (m_dipswPanel) m_dipswPanel->Refresh();
}

void PrefFrame::UpdateDipswPicture(void)
{
	for (int bank = 0; bank < 3; bank++) {
		for (int bit = 0; bit < 8; bit++) {
			if (m_dipsw[bank][bit]) {
				m_dipsw[bank][bit]->SetValue(
				    (np2cfg.dipsw[bank] >> bit) & 1);
			}
		}
	}
	if (m_dipswPanel) m_dipswPanel->Refresh();
}

/* ------------------------------------------------------------ */
/*  Config load / save                                          */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/*  Config load / save helpers                                  */
/* ------------------------------------------------------------ */

/* Helper: find child with name (using cache for speed) */
static wxWindow *FindByName(wxWindow *root, const wxString &name)
{
	/* Using wxWindow::FindWindowByName is generally faster than manual recursion
	 * if the library optimizes it, but we can also use a local recursive search
	 * and cache pointers if needed. For now, let's use the built-in one. */
	return root->FindWindowByName(name);
}

static void SetCheckByName(wxWindow *root, const char *name, bool val)
{
	auto *w = FindByName(root, name);
	if (auto *cb = wxDynamicCast(w, wxCheckBox)) cb->SetValue(val);
}

static void SetSpinByName(wxWindow *root, const char *name, int val)
{
	auto *w = FindByName(root, name);
	if (auto *sc = wxDynamicCast(w, wxSpinCtrl))   sc->SetValue(val);
	else if (auto *sl = wxDynamicCast(w, wxSlider)) sl->SetValue(val);
}

static int GetSpinByName(wxWindow *root, const char *name, int defval)
{
	auto *w = FindByName(root, name);
	if (auto *sc = wxDynamicCast(w, wxSpinCtrl))   return sc->GetValue();
	if (auto *sl = wxDynamicCast(w, wxSlider))      return sl->GetValue();
	return defval;
}

static bool GetCheckByName(wxWindow *root, const char *name)
{
	auto *w = FindByName(root, name);
	if (auto *cb = wxDynamicCast(w, wxCheckBox)) return cb->GetValue();
	return false;
}

static void SetTextByName(wxWindow *root, const char *name, const char *val)
{
	auto *w = FindByName(root, name);
	if (auto *tc = wxDynamicCast(w, wxTextCtrl)) tc->SetValue(wxString::FromUTF8(val));
}

static wxString GetTextByName(wxWindow *root, const char *name)
{
	auto *w = FindByName(root, name);
	if (auto *tc = wxDynamicCast(w, wxTextCtrl)) return tc->GetValue();
	return "";
}

void PrefFrame::UpdateDipswBmp(void)
{
	auto updatePanel = [&](wxPanel *p, UINT8 board) {
		if (!p) return;
		auto *panel = (BmpDipSwPanel *)p;
		void *data = nullptr;
		switch (board) {
		case 0x02: data = dipswbmp_getsnd26(np2cfg.snd26opt); break;
		case 0x04:
		case 0x06:
		case 0x14: data = dipswbmp_getsnd86(np2cfg.snd86opt); break;
		case 0x20: data = dipswbmp_getsndspb(np2cfg.spbopt, np2cfg.spb_vrc); break;
		case 0x08: data = dipswbmp_getsnd118(np2cfg.snd118io, np2cfg.snd118dma,
		                                     np2cfg.snd118irqf, np2cfg.snd118irqp,
		                                     np2cfg.snd118irqm, np2cfg.snd118rom); break;
		}
		if (data) {
			panel->SetBmpData(data);
			panel->Show(true);
			_MFREE(data);
		} else {
			panel->Show(false);
		}
	};

	updatePanel(m_snd26Dipsw, 0x02);
	updatePanel(m_snd86Dipsw, 0x04);
	updatePanel(m_snd118Dipsw, 0x08);
	updatePanel(m_sndSpbDipsw, 0x20);

	if (m_sndDipsw) {
		int sel = m_sndboard->GetSelection();
		if (sel >= 0 && sel < (int)NELEMENTS(s_sndboard_vals)) {
			updatePanel(m_sndDipsw, s_sndboard_vals[sel]);
		}
	}
}
void PrefFrame::UpdateMHz(void)
{
	if (!m_cpuMHz) return;
	double base = 1.9968;
	if (auto *w = FindByName(this, "clk_base")) {
		if (auto *ch = wxDynamicCast(w, wxChoice))
			if (ch->GetSelection() == 1) base = 2.4576;
	}
	int mult = GetSpinByName(this, "clk_mult", 20);
	m_cpuMHz->SetLabel(wxString::Format("%.4f MHz", base * mult));
}

void PrefFrame::OnDefault(wxCommandEvent & /*evt*/)
{
	int curTab = m_notebook ? m_notebook->GetSelection() : -1;
	NP2CFG    cfg_bk = np2cfg;
	NP2OSCFG  os_bk  = np2oscfg;
#if defined(SUPPORT_WAB)
	NP2WABCFG wab_bk = np2wabcfg;
#endif
	extern void pccore_setdefault(void);
	pccore_setdefault();
	np2oscfg_setdefault();
	np2wabcfg_setdefault();
	LoadFromConfig(curTab);
	np2cfg    = cfg_bk;
	np2oscfg  = os_bk;
#if defined(SUPPORT_WAB)
	np2wabcfg = wab_bk;
#endif
}

void PrefFrame::LoadFromConfig(int tabId)
{
	/* System */
	IF_TAB(TAB_SYSTEM) {
		{
			int sel = 1; // Default VX
			if (milstr_cmp(np2cfg.model, "VM") == 0) sel = 0;
			else if (milstr_cmp(np2cfg.model, "286") == 0) sel = 2;
			if (m_arch[sel]) m_arch[sel]->SetValue(true);
		}
		if (auto *w = FindByName(this, "clk_base")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2cfg.baseclock == 1996800 ? 0 : 1);
		}
		SetSpinByName(this, "clk_mult", (int)np2cfg.multiple);
		UpdateMHz();
		SetSpinByName(this, "extmem",   (int)np2cfg.EXTMEM);

#if defined(CPUCORE_IA32)
		if (auto *w = FindByName(this, "CPUType")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int idx = GetCpuTypeIndex();
				int sel = 0;
				for (int i = 0; i < (int)WXSIZEOF(s_cpu_indices); i++) {
					if (s_cpu_indices[i] == idx) {
						sel = i;
						break;
					}
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "FPU_TYPE")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2cfg.fpu_type & 3);
		}
#endif

		if (auto *w = FindByName(this, "EmuSpeed")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 1; // Default 100%
				if      (np2cfg.emuspeed <= 50)  sel = 0;
				else if (np2cfg.emuspeed <= 100) sel = 1;
				else if (np2cfg.emuspeed <= 200) sel = 2;
				else if (np2cfg.emuspeed <= 400) sel = 3;
				else if (np2cfg.emuspeed <= 800) sel = 4;
				else                             sel = 5;
				ch->SetSelection(sel);
			}
		}

#if defined(SUPPORT_FAST_MEMORYCHECK)
		SetCheckByName(this, "SUPPORT_FAST_MEMORYCHECK", np2cfg.memcheckspeed > 1);
#endif
		SetCheckByName(this, "SYSIOMSK",   np2cfg.sysiomsk != 0);
		SetCheckByName(this, "DisableMMX", np2oscfg.disablemmx != 0);
		SetCheckByName(this, "MULTITHREAD", np2wabcfg.multithread != 0);
		SetCheckByName(this, "USE144FD",   np2cfg.usefd144 != 0);
		SetCheckByName(this, "TIMERFIX",   np2cfg.timerfix != 0);
		SetCheckByName(this, "CONSTTSC",   np2cfg.consttsc != 0);
#if defined(SUPPORT_ASYNC_CPU)
		SetCheckByName(this, "ASYNCCPU",   np2cfg.asynccpu != 0);
#endif
	}

	/* Display */
	IF_TAB(TAB_DISPLAY) {
		SetCheckByName(this, "DispSync",     np2cfg.DISPSYNC != 0);
		SetCheckByName(this, "Real_Pal",     np2cfg.RASTER != 0);
		/* GDC radio */
		if (auto *w = FindByName(this, np2cfg.uPD72020 ? "gdc_72020" : "gdc_7220"))
			if (auto *rb = wxDynamicCast(w, wxRadioButton)) rb->SetValue(true);
		/* Graphic Charger choice */
		if (auto *w = FindByName(this, "GRCG_EGC"))
			if (auto *ch = wxDynamicCast(w, wxChoice)) ch->SetSelection(np2cfg.grcg & 3);
#if defined(SUPPORT_PEGC)
		SetCheckByName(this, "pegcplane",    np2cfg.usepegcplane != 0);
#endif
		SetCheckByName(this, "LCD_MODE_en",  (np2cfg.LCD_MODE & 1) != 0);
		SetCheckByName(this, "LCD_MODE_rev", (np2cfg.LCD_MODE & 2) != 0);
		SetCheckByName(this, "color16b",     np2cfg.color16 != 0);
		SetCheckByName(this, "skipline",     np2cfg.skipline != 0);
		SetCheckByName(this, "draw32bit",    draw32bit != 0);
		SetSpinByName(this,  "skplight",    (int)(SINT16)np2cfg.skiplight);
#if defined(SUPPORT_WAB)
		SetCheckByName(this, "MULTIWND",     np2wabcfg.multiwindow != 0);
#endif
#if defined(SUPPORT_CL_GD5430)
		SetCheckByName(this, "USE_CLGD",     np2cfg.usegd5430 != 0);
		if (auto *w = FindByName(this, "CLGDTYPE")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 4; /* default Xe10 */
				for (int i = 0; i < (int)NELEMENTS(s_clgd_vals); i++) {
					if (s_clgd_vals[i] == np2cfg.gd5430type) { sel = i; break; }
				}
				ch->SetSelection(sel);
			}
		}
		SetCheckByName(this, "CLGDFCUR",     np2cfg.gd5430fakecur != 0);
#endif

#if defined(SUPPORT_VIDEOFILTER)
		{
			uint8_t pno = np2cfg.vf1_pno;
			if (pno >= 3) pno = 0;
			for (int f = 0; f < 3; f++) {
				char nm[16];
				snprintf(nm, sizeof(nm), "vf_f%d_en", f);
				SetCheckByName(this, nm, np2cfg.vf1_param[pno][f][0] != 0);
				snprintf(nm, sizeof(nm), "vf_f%d_type", f);
				if (auto *w = FindByName(this, nm)) {
					if (auto *ch = wxDynamicCast(w, wxChoice)) {
						uint32_t t = np2cfg.vf1_param[pno][f][1];
						if (t > 7) t = 0;
						ch->SetSelection((int)t);
					}
				}
				for (int p = 0; p < 8; p++) {
					snprintf(nm, sizeof(nm), "vf_f%d_p%d", f, p);
					if (auto *w = FindByName(this, nm)) {
						if (auto *tx = wxDynamicCast(w, wxTextCtrl))
							tx->ChangeValue(wxString::Format("%u",
							    (unsigned)np2cfg.vf1_param[pno][f][p]));
					}
				}
			}
		}
#endif
	}

	/* Sound board */
	IF_TAB(TAB_SOUND) {
		if (m_sndboard) {
			int sel = 0;
			for (int i = 0; i < (int)NELEMENTS(s_sndboard_vals); i++) {
				if (s_sndboard_vals[i] == np2cfg.SOUND_SW) { sel = i; break; }
			}
			m_sndboard->SetSelection(sel);
		}
		if (auto *w = FindByName(this, "SampleHz")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				const UINT32 rates[] = {11025, 22050, 44100, 48000, 88200, 96000};
				int sel = 2; // Default 44100
				for (int i = 0; i < 6; i++) { if (np2cfg.samplingrate <= rates[i]) { sel = i; break; } }
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "Latencys")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				const UINT16 delays[] = {47, 93, 186, 372};
				int sel = 1;
				for (int i = 0; i < 4; i++) { if (np2cfg.delayms <= delays[i]) { sel = i; break; } }
				ch->SetSelection(sel);
			}
		}

		/* Mixer sub-tab */
		SetSpinByName(this, "vol_master", (int)np2cfg.vol_master);
		SetSpinByName(this, "vol_fm",     (int)np2cfg.vol_fm);
		SetSpinByName(this, "vol_ssg",    (int)np2cfg.vol_ssg);
		SetSpinByName(this, "vol_adpcm",  (int)np2cfg.vol_adpcm);
		SetSpinByName(this, "vol_pcm",    (int)np2cfg.vol_pcm);
		SetSpinByName(this, "vol_rhythm", (int)np2cfg.vol_rhythm);
		SetSpinByName(this, "davolume",   (int)np2cfg.davolume);
		SetSpinByName(this, "vol_midi",   (int)np2cfg.vol_midi);
		SetSpinByName(this, "MOTORVOL",   (int)np2cfg.MOTORVOL);
		{
			int vol = np2cfg.BEEP_VOL & 3;
			if (m_beepvol[vol]) m_beepvol[vol]->SetValue(true);
		}
#if defined(SUPPORT_WAB)
		SetCheckByName(this, "wabasw",   np2cfg.wabasw == 0);
#endif
		SetCheckByName(this, "Seek_Snd", np2cfg.MOTOR != 0);

		/* 14 sub-tab */
		for (int i = 0; i < 6; i++) {
			char name[16];
			snprintf(name, sizeof(name), "vol14_%d", i);
			SetSpinByName(this, name, (int)np2cfg.vol14[i]);
		}

		/* 26 sub-tab */
		if (auto *w = FindByName(this, "SND26IO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection((np2cfg.snd26opt & 0x10) ? 1 : 0);
		}
		if (auto *w = FindByName(this, "SND26INT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				switch (np2cfg.snd26opt & 0xc0) {
				case 0x00: sel = 0; break;
				case 0x80: sel = 1; break;
				case 0xc0: sel = 2; break;
				case 0x40: sel = 3; break;
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "SND26ROM")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2cfg.snd26opt & 0x07);
		}

		/* 86 sub-tab */
		if (auto *w = FindByName(this, "SND86IO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection((np2cfg.snd86opt & 0x01) ? 0 : 1); // 0188=0x01, 0288=0x00
		}
		if (auto *w = FindByName(this, "SND86INTA")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				switch (np2cfg.snd86opt & 0x0c) {
				case 0x00: sel = 0; break;
				case 0x04: sel = 1; break;
				case 0x0c: sel = 2; break;
				case 0x08: sel = 3; break;
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "SND86ID")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(((~np2cfg.snd86opt) >> 5) & 7); // 0x -> 0xe0, 7x -> 0x00
		}
		SetCheckByName(this, "SND86ROM", (np2cfg.snd86opt & 0x02) != 0);
		SetCheckByName(this, "SND86INT", (np2cfg.snd86opt & 0x10) != 0);

		/* 118 sub-tab */
		if (auto *w = FindByName(this, "SND118IO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				const UINT16 ios[] = {0x0088, 0x0188, 0x0288, 0x0388};
				int sel = 1;
				for (int i = 0; i < 4; i++) { if (ios[i] == np2cfg.snd118io) { sel = i; break; } }
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "SND118ID")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection((np2cfg.snd118id >> 4) & 0x0f);
		}
		if (auto *w = FindByName(this, "SND118DMA")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				if      (np2cfg.snd118dma == 1) sel = 1;
				else if (np2cfg.snd118dma == 3) sel = 2;
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "SND118INTF")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				switch (np2cfg.snd118irqf) {
				case 3:  sel = 0; break;
				case 10: sel = 1; break;
				case 12: sel = 2; break;
				case 13: sel = 3; break;
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "SND118INTP")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				switch (np2cfg.snd118irqp) {
				case 3:  sel = 0; break;
				case 5:  sel = 1; break;
				case 10: sel = 2; break;
				case 12: sel = 3; break;
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "SND118INTM")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2cfg.snd118irqm == 0xff ? 0 : 1);
		}
		SetCheckByName(this, "SND118ROM", np2cfg.snd118rom != 0);

		/* WSS sub-tab */
		if (auto *w = FindByName(this, "SNDWSSID")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection((np2cfg.sndwssid >> 4) & 0x0f);
		}
		if (auto *w = FindByName(this, "SNDWSSDMA")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				if (np2cfg.sndwssdma == 3) sel = 1;
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "SNDWSSINT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				switch (np2cfg.sndwssirq) {
				case 3:  sel = 0; break;
				case 10: sel = 1; break;
				case 12: sel = 2; break;
				}
				ch->SetSelection(sel);
			}
		}

		/* SB16 sub-tab */
		if (auto *w = FindByName(this, "SNDSB16IO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection((np2cfg.sndsb16io - 0xd2) / 2);
		}
		if (auto *w = FindByName(this, "SNDSB16DMA")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2cfg.sndsb16dma == 0 ? 0 : 1);
		}
		if (auto *w = FindByName(this, "SNDSB16INT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				switch (np2cfg.sndsb16irq) {
				case 3:  sel = 0; break;
				case 5:  sel = 1; break;
				case 10: sel = 2; break;
				case 12: sel = 3; break;
				}
				ch->SetSelection(sel);
			}
		}

		/* SpeakBoard sub-tab */
		if (auto *w = FindByName(this, "SPBIO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection((np2cfg.spbopt & 0x10) ? 1 : 0);
		}
		if (auto *w = FindByName(this, "SPBINT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				switch (np2cfg.spbopt & 0xc0) {
				case 0x00: sel = 0; break;
				case 0x80: sel = 1; break;
				case 0xc0: sel = 2; break;
				case 0x40: sel = 3; break;
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "SPBROM")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2cfg.spbopt & 0x07);
		}
		SetSpinByName(this, "SPBVRLEVEL", (int)np2cfg.spb_vrl);
		SetCheckByName(this, "SPBREVERSE", np2cfg.spb_x != 0);

		/* Joypad sub-tab */
		SetCheckByName(this, "JOYPAD1",    (np2oscfg.JOYPAD1 & 1) != 0);
		SetCheckByName(this, "PAD1_POVXY", (np2oscfg.JOYPAD1POVXY & 1) != 0);
#if defined(SUPPORT_GAMEPORT)
		SetCheckByName(this, "PAD1_GAMEPORT", (np2cfg.gameport & 1) != 0);
		SetCheckByName(this, "PAD1_ANALOG",   (np2cfg.analogjoy & 1) != 0);
#endif

		/* fmgen sub-tab */
#if defined(SUPPORT_FMGEN)
		SetCheckByName(this, "USEFMGEN", np2cfg.usefmgen != 0);
#endif
		UpdateDipswBmp();
	}

	/* MIDI */
	IF_TAB(TAB_MIDI) {
		SetCheckByName(this, "USEMPU98", np2cfg.mpuenable != 0);
		if (auto *w = FindByName(this, "MPU_PORT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection((np2cfg.mpuopt >> 4) & 0x0f);
		}
		if (auto *w = FindByName(this, "MPU_IRQ")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2cfg.mpuopt & 3);
		}
		if (auto *w = FindByName(this, "MPU_OUT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				if (np2oscfg.MIDIDEV[0][0]) {
					int idx = ch->FindString(wxString::FromUTF8(np2oscfg.MIDIDEV[0]));
					if (idx != wxNOT_FOUND) sel = idx;
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "MPU_IN")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				if (np2oscfg.MIDIDEV[1][0]) {
					int idx = ch->FindString(wxString::FromUTF8(np2oscfg.MIDIDEV[1]));
					if (idx != wxNOT_FOUND) sel = idx;
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "MPU_MDL")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				if (np2oscfg.mpu.mdl[0]) {
					int idx = ch->FindString(wxString::FromUTF8(np2oscfg.mpu.mdl));
					if (idx != wxNOT_FOUND) sel = idx;
				}
				ch->SetSelection(sel);
			}
		}
		SetCheckByName(this, "MPU_DEF_EN", np2oscfg.mpu.def_en != 0);
		SetTextByName(this, "MPU_DEF", np2oscfg.mpu.def);
#if defined(SUPPORT_SMPU98)
		SetCheckByName(this, "USE_SMPU",  np2cfg.smpuenable != 0);
		SetCheckByName(this, "SMPUMUTB",  np2cfg.smpumuteB  != 0);
		if (auto *w = FindByName(this, "SMPU_PORT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection((np2cfg.smpuopt >> 4) & 0x0f);
		}
		if (auto *w = FindByName(this, "SMPU_IRQ")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2cfg.smpuopt & 3);
		}
		if (auto *w = FindByName(this, "SMPUA_OUT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int idx = ch->FindString(wxString::FromUTF8(np2oscfg.MIDIDEVA[0]));
				ch->SetSelection(idx != wxNOT_FOUND ? idx : 0);
			}
		}
		if (auto *w = FindByName(this, "SMPUB_OUT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int idx = ch->FindString(wxString::FromUTF8(np2oscfg.MIDIDEVB[0]));
				ch->SetSelection(idx != wxNOT_FOUND ? idx : 0);
			}
		}
#endif
	}

	/* Input */
	IF_TAB(TAB_INPUT) {
		if (auto *w = FindByName(this, "keyboard")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 0;
				if      (np2oscfg.KEYBOARD == KEY_KEY101) sel = 1;
				else if (np2oscfg.KEYBOARD >= 2)          sel = 2; // Stub for JoyKey
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "joypad")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				ch->Clear();
				ch->Append("None");
				int sel = 0;
				joymng_devinfo_t **list = joymng_get_devinfo_list();
				if (list) {
					for (int i = 0; list[i]; i++) {
						ch->Append(wxString::FromUTF8(list[i]->devname));
						if (np2oscfg.JOYDEV[0][0] &&
						    strcmp(list[i]->devname, np2oscfg.JOYDEV[0]) == 0) {
							sel = i + 1;
						}
					}
				}
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "F12KEY")) {
			if (auto *ch = wxDynamicCast(w, wxChoice))
				ch->SetSelection(np2oscfg.F12KEY & 7);
		}
		if (auto *w = FindByName(this, "Mouse_sp")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = 2; /* default x1 (also for unset / value 4) */
				switch (np2oscfg.mouse_move_ratio) {
				case 1:  sel = 0; break;
				case 2:  sel = 1; break;
				case 8:  sel = 3; break;
				case 16: sel = 4; break;
				default: sel = 2; break;
				}
				ch->SetSelection(sel);
			}
		}
		SetCheckByName(this, "XSHIFT",   np2oscfg.xrollkey != 0);
		SetCheckByName(this, "DragDrop", np2oscfg.confirm != 0);
		SetCheckByName(this, "btnRAPID", np2cfg.BTN_RAPID != 0);
		SetCheckByName(this, "MS_RAPID", np2cfg.MOUSERAPID != 0);
	}

	/* FDD */
	IF_TAB(TAB_FDD) {
		for (int i = 0; i < 4; i++) {
			char name[16];
			snprintf(name, sizeof(name), "FDDRIVE%d", i + 1);
			SetCheckByName(this, name, ((np2cfg.fddequip >> i) & 1) != 0);
		}
		SetSpinByName(this, "Seek_Vol",  (int)np2cfg.MOTORVOL);
	}

	/* HDD / IDE type + HDD Equipment */
	IF_TAB(TAB_HDD) {
#if defined(SUPPORT_IDEIO)
		for (int i = 0; i < 4; i++) {
			char name[16];
			snprintf(name, sizeof(name), "IDE%dTYPE", i + 1);
			if (auto *w = FindByName(this, name)) {
				if (auto *ch = wxDynamicCast(w, wxChoice)) {
					int sel = 0;
					if      (np2cfg.idetype[i] == SXSIDEV_HDD)   sel = 1;
					else if (np2cfg.idetype[i] == SXSIDEV_CDROM) sel = 2;
					ch->SetSelection(sel);
				}
			}
		}
#endif
#if defined(SUPPORT_IDEIO)
		for (int i = 0; i < 4; i++) {
			char name[16];
			snprintf(name, sizeof(name), "IDE%dEQUIP", i + 1);
			SetCheckByName(this, name, np2cfg.idetype[i] != SXSIDEV_NC);
		}
#else
		for (int i = 0; i < 2; i++) {
			char name[16];
			snprintf(name, sizeof(name), "SASI%dEQUIP", i + 1);
			SetCheckByName(this, name, np2cfg.sasihdd[i][0] != 0);
		}
#endif
#if defined(SUPPORT_IDEIO)
		SetCheckByName(this, "CD_ASYNC", np2cfg.useasynccd != 0);
		SetCheckByName(this, "IDE_BIOS", np2cfg.idebios != 0);
		SetCheckByName(this, "AIDEBIOS", np2cfg.autoidebios != 0);
#endif
#if defined(SUPPORT_SCSI)
		for (int i = 0; i < 4; i++) {
			char name[16];
			snprintf(name, sizeof(name), "SCSI%dEQUIP", i + 1);
			SetCheckByName(this, name, np2cfg.scsihdd[i][0] != 0);
			snprintf(name, sizeof(name), "SCSI%dTYPE", i);
			if (auto *w = FindByName(this, name))
				if (auto *ch = wxDynamicCast(w, wxChoice))
					ch->SetSelection(np2cfg.scsihdd[i][0] != 0 ? 1 : 0);
		}
#endif
#if defined(SUPPORT_LIBCDIO)
		SetCheckByName(this, "LIBCDIO", np2cfg.libcdio != 0);
#endif
	}

	/* Serial: all 3 ports (port, bps, data bits, parity, stop bits, DSR check) */
	IF_TAB(TAB_SERIAL) {
		const UINT32 bpsRates[] = {110,300,600,1200,2400,4800,9600,14400,19200,28800,38400,57600,115200};
		for (int i = 0; i < 3; i++) {
			char portname[16], bpsname[16], dbname[16], parname[16], sbname[16], dsrname[16];
			snprintf(portname, sizeof(portname), "com%dport",   i + 1);
			snprintf(bpsname,  sizeof(bpsname),  "com%d_bps",   i + 1);
			snprintf(dbname,   sizeof(dbname),   "com%d_dbits", i + 1);
			snprintf(parname,  sizeof(parname),  "com%d_parity",i + 1);
			snprintf(sbname,   sizeof(sbname),   "com%d_sbits", i + 1);
			snprintf(dsrname,  sizeof(dsrname),  "com%d_dsr",   i + 1);

			if (auto *w = FindByName(this, portname))
				if (auto *ch = wxDynamicCast(w, wxChoice)) ch->SetSelection(np2oscfg.com[i].port);

			if (auto *w = FindByName(this, bpsname)) {
				if (auto *ch = wxDynamicCast(w, wxChoice)) {
					int sel = 8;
					for (int j = 0; j < 13; j++) { if (bpsRates[j] == np2oscfg.com[i].speed) { sel = j; break; } }
					ch->SetSelection(sel);
				}
			}

			if (auto *w = FindByName(this, dbname))
				if (auto *ch = wxDynamicCast(w, wxChoice))
					ch->SetSelection((np2oscfg.com[i].param >> 2) & 3);

			if (auto *w = FindByName(this, parname)) {
				if (auto *ch = wxDynamicCast(w, wxChoice)) {
					int psel = 0;
					switch (np2oscfg.com[i].param & 0x30) {
					case 0x10: psel = 1; break; /* Odd */
					case 0x30: psel = 2; break; /* Even */
					default: psel = 0; break;
					}
					ch->SetSelection(psel);
				}
			}

			if (auto *w = FindByName(this, sbname)) {
				if (auto *ch = wxDynamicCast(w, wxChoice)) {
					int ssel = 0;
					switch (np2oscfg.com[i].param & 0xC0) {
					case 0x80: ssel = 1; break; /* 1.5 */
					case 0xC0: ssel = 2; break; /* 2 */
					default: ssel = 0; break;
					}
					ch->SetSelection(ssel);
				}
			}

			SetCheckByName(this, dsrname, np2oscfg.com[i].direct != 0);
		}
	}

	/* Network */
	IF_TAB(TAB_NETWORK) {
#if defined(SUPPORT_NET)
		SetTextByName(this, "NP2NETTAP", np2cfg.np2nettap);
#endif
#if defined(SUPPORT_LGY98)
		SetCheckByName(this, "USELGY98", np2cfg.uselgy98 != 0);
		if (auto *w = FindByName(this, "LGY98_IO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				const UINT16 ios[] = {0x00D0,0x10D0,0x20D0,0x30D0,0x40D0,0x50D0,0x60D0,0x70D0};
				int sel = 1;
				for (int i = 0; i < 8; i++) { if (ios[i] == np2cfg.lgy98io) { sel = i; break; } }
				ch->SetSelection(sel);
			}
		}
		if (auto *w = FindByName(this, "LGY98IRQ")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				const UINT8 irqs[] = {0, 1, 2, 5};
				int sel = 1;
				for (int i = 0; i < 4; i++) { if (irqs[i] == np2cfg.lgy98irq) { sel = i; break; } }
				ch->SetSelection(sel);
			}
		}
#endif
	}

	/* Hostdrv */
	IF_TAB(TAB_HOSTDRV) {
#if defined(SUPPORT_HOSTDRV)
		SetCheckByName(this, "use_hdrv", np2cfg.hdrvenable != 0);
		SetTextByName(this, "hdrvroot", np2cfg.hdrvroot);
		SetCheckByName(this, "hdrv_acc_r", (np2cfg.hdrvacc & 1) != 0);
		SetCheckByName(this, "hdrv_acc_w", (np2cfg.hdrvacc & 2) != 0);
		SetCheckByName(this, "hdrv_acc_d", (np2cfg.hdrvacc & 4) != 0);
#if defined(SUPPORT_HOSTDRVNT)
		SetCheckByName(this, "hdrv_nt", np2cfg.hdrvntenable != 0);
#endif
#endif
	}

	/* PCI + Misc (both live on the Misc tab) */
	IF_TAB(TAB_MISC) {
#if defined(SUPPORT_PCI)
		SetCheckByName(this, "USE_PCI", np2cfg.usepci != 0);
		if (auto *w = FindByName(this, "PCI_PCMC"))
			if (auto *ch = wxDynamicCast(w, wxChoice)) ch->SetSelection(np2cfg.pci_pcmc < 3 ? np2cfg.pci_pcmc : 0);
		SetCheckByName(this, "PCI_B32", np2cfg.pci_bios32 != 0);
#endif
		SetCheckByName(this, "s_NOWAIT", np2oscfg.NOWAIT != 0);
		SetCheckByName(this, "e_resume", np2oscfg.resume != 0);
#if defined(SUPPORT_STATSAVE)
		SetCheckByName(this, "STATSAVE", np2cfg.statsave != 0);
#endif
		SetSpinByName(this, "SkpFrame", (int)np2oscfg.DRAW_SKIP);

		if (auto *w = FindByName(this, "CyclePath"))
			if (auto *fp = wxDynamicCast(w, wxFilePickerCtrl)) fp->SetPath(wxString::FromUTF8(cycle_shot_path));
		SetSpinByName(this, "CycleInt", (int)cycle_shot_interval);
	}

	/* DIP SW */
	IF_TAB(TAB_DIPSW) {
		UpdateDipswPicture();
	}

	/* Calendar */
	IF_TAB(TAB_CALENDAR) {
		SetCheckByName(this, "cal_real", np2cfg.calendar != 0);
		SetCheckByName(this, "cal_vir",  np2cfg.calendar == 0);
		UINT8 cbuf[6];
		calendar_getvir(cbuf);
		char tmp[8];
		static const char *nms[6] = {"cal_yy","cal_mm","cal_dd","cal_hh","cal_mi","cal_ss"};
		for (int i = 0; i < 6; i++) {
			snprintf(tmp, sizeof(tmp), "%02X", cbuf[i]);
			SetTextByName(this, nms[i], tmp);
		}
	}
}

void PrefFrame::SaveToConfig(void)
{
	/* System */
	{
		const char *model = "VX";
		if      (m_arch[0] && m_arch[0]->GetValue()) model = "VM";
		else if (m_arch[2] && m_arch[2]->GetValue()) model = "286";
		milstr_ncpy(np2cfg.model, model, sizeof(np2cfg.model));
	}
	if (auto *w = FindByName(this, "clk_base")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			np2cfg.baseclock = (sel == 0) ? 1996800 : 2457600;
		}
	}
	np2cfg.multiple  = (UINT)GetSpinByName(this, "clk_mult", 20);

#if defined(CPUCORE_IA32)
	if (auto *w = FindByName(this, "CPUType")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < (int)WXSIZEOF(s_cpu_indices)) {
				SetCpuTypeIndex(s_cpu_indices[sel]);
			}
		}
	}
	if (auto *w = FindByName(this, "FPU_TYPE")) {
		if (auto *ch = wxDynamicCast(w, wxChoice))
			np2cfg.fpu_type = (UINT8)ch->GetSelection();
	}
#endif

#if defined(SUPPORT_LARGE_MEMORY)
	np2cfg.EXTMEM    = (UINT16)GetSpinByName(this, "extmem", 16);
#else
	np2cfg.EXTMEM    = (UINT8)GetSpinByName(this, "extmem", 16);
#endif

	if (auto *w = FindByName(this, "EmuSpeed")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			const UINT32 speeds[] = {50, 100, 200, 400, 800, 1600};
			if (sel >= 0 && sel < 6) np2cfg.emuspeed = speeds[sel];
		}
	}

#if defined(SUPPORT_FAST_MEMORYCHECK)
	np2cfg.memcheckspeed = GetCheckByName(this, "SUPPORT_FAST_MEMORYCHECK") ? 8 : 1;
#endif
	np2cfg.sysiomsk = GetCheckByName(this, "SYSIOMSK") ? 0xFF00 : 0;
	np2oscfg.disablemmx = GetCheckByName(this, "DisableMMX") ? 1 : 0;
	np2wabcfg.multithread = GetCheckByName(this, "MULTITHREAD") ? 1 : 0;

	// np2cfg.grcg      = GetCheckByName(this, "grcg")    ? 3 : 0;
	np2cfg.color16   = GetCheckByName(this, "color16b") ? 1 : 0;
	np2cfg.calendar  = GetCheckByName(this, "cal_real") ? 1 : 0;

	/* Virtual calendar textboxes → calendar_set(BCD UINT8[6]) */
	{
		static const char *nms[6] = {"cal_yy","cal_mm","cal_dd","cal_hh","cal_mi","cal_ss"};
		UINT8 cbuf[6] = {0};
		bool got = false;
		for (int i = 0; i < 6; i++) {
			if (auto *w = FindByName(this, nms[i])) {
				if (auto *t = wxDynamicCast(w, wxTextCtrl)) {
					unsigned int v = 0;
					wxString s = t->GetValue();
					if (sscanf(s.c_str(), "%x", &v) == 1) {
						cbuf[i] = (UINT8)v;
						got = true;
					}
				}
			}
		}
		if (got) calendar_set(cbuf);
	}

	np2cfg.usefd144  = GetCheckByName(this, "USE144FD") ? 1 : 0;
	np2cfg.timerfix  = GetCheckByName(this, "TIMERFIX") ? 1 : 0;
	np2cfg.consttsc  = GetCheckByName(this, "CONSTTSC") ? 1 : 0;
#if defined(SUPPORT_ASYNC_CPU)
	np2cfg.asynccpu  = GetCheckByName(this, "ASYNCCPU") ? 1 : 0;
#endif

	/* Display */
	np2cfg.DISPSYNC  = GetCheckByName(this, "DispSync")     ? 1 : 0;
	np2cfg.RASTER    = GetCheckByName(this, "Real_Pal")     ? 1 : 0;
	/* GDC radio */
	{
		bool is72020 = false;
		if (auto *w = FindByName(this, "gdc_72020"))
			if (auto *rb = wxDynamicCast(w, wxRadioButton)) is72020 = rb->GetValue();
		np2cfg.uPD72020 = is72020 ? 1 : 0;
	}
	/* Graphic Charger */
	if (auto *w = FindByName(this, "GRCG_EGC")) {
#if defined(CPUCORE_IA32)
		np2cfg.grcg = 3; /* Always EGC */
#else
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			np2cfg.grcg = (UINT8)(sel >= 0 && sel < 3 ? sel : 2);
		}
#endif
	}
#if defined(SUPPORT_PEGC)
	np2cfg.usepegcplane = GetCheckByName(this, "pegcplane")   ? 1 : 0;
#endif
	np2cfg.LCD_MODE  = 0;
	if (GetCheckByName(this, "LCD_MODE_en"))  np2cfg.LCD_MODE |= 1;
	if (GetCheckByName(this, "LCD_MODE_rev")) np2cfg.LCD_MODE |= 2;
	np2cfg.color16   = GetCheckByName(this, "color16b")     ? 1 : 0;
	np2cfg.skipline  = GetCheckByName(this, "skipline")     ? 1 : 0;
	draw32bit        = GetCheckByName(this, "draw32bit")    ? 1 : 0;
	np2cfg.skiplight = (UINT16)GetSpinByName(this, "skplight", 0);
#if defined(SUPPORT_WAB)
	np2wabcfg.multiwindow = GetCheckByName(this, "MULTIWND") ? 1 : 0;
#endif
#if defined(SUPPORT_CL_GD5430)
	np2cfg.usegd5430  = GetCheckByName(this, "USE_CLGD")    ? 1 : 0;
	if (auto *w = FindByName(this, "CLGDTYPE")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < (int)NELEMENTS(s_clgd_vals))
				np2cfg.gd5430type = s_clgd_vals[sel];
		}
	}
	np2cfg.gd5430fakecur = GetCheckByName(this, "CLGDFCUR")  ? 1 : 0;
#endif

#if defined(SUPPORT_VIDEOFILTER)
	{
		uint8_t pno = np2cfg.vf1_pno;
		if (pno >= 3) pno = 0;
		for (int f = 0; f < 3; f++) {
			char nm[16];
			/* Param1..8 textboxes are authoritative for the 8 raw entries */
			for (int p = 0; p < 8; p++) {
				snprintf(nm, sizeof(nm), "vf_f%d_p%d", f, p);
				wxString s = GetTextByName(this, nm);
				if (s.IsEmpty()) {
					np2cfg.vf1_param[pno][f][p] = 0;
				} else {
					unsigned long v = 0;
					s.ToULong(&v);
					np2cfg.vf1_param[pno][f][p] = (uint32_t)v;
				}
			}
			/* Enable / Type override the first two indices */
			snprintf(nm, sizeof(nm), "vf_f%d_en", f);
			np2cfg.vf1_param[pno][f][0] = GetCheckByName(this, nm) ? 1 : 0;
			snprintf(nm, sizeof(nm), "vf_f%d_type", f);
			if (auto *w = FindByName(this, nm)) {
				if (auto *ch = wxDynamicCast(w, wxChoice)) {
					int sel = ch->GetSelection();
					if (sel >= 0 && sel < 8)
						np2cfg.vf1_param[pno][f][1] = (uint32_t)sel;
				}
			}
		}
	}
#endif

	/* Sound */
	if (m_sndboard) {
		int sel = m_sndboard->GetSelection();
		if (sel >= 0 && sel < (int)NELEMENTS(s_sndboard_vals))
			np2cfg.SOUND_SW = s_sndboard_vals[sel];
	}
	if (auto *w = FindByName(this, "SampleHz")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			const UINT32 rates[] = {11025, 22050, 44100, 48000, 88200, 96000};
			if (sel >= 0 && sel < 6) np2cfg.samplingrate = rates[sel];
		}
	}
	if (auto *w = FindByName(this, "Latencys")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT16 delays[] = {47, 93, 186, 372};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 4) np2cfg.delayms = delays[sel];
		}
	}

	/* Mixer sub-tab */
	np2cfg.vol_master = (UINT8)GetSpinByName(this, "vol_master", 100);
	np2cfg.vol_fm     = (UINT8)GetSpinByName(this, "vol_fm", 128);
	np2cfg.vol_ssg    = (UINT8)GetSpinByName(this, "vol_ssg", 128);
	np2cfg.vol_adpcm  = (UINT8)GetSpinByName(this, "vol_adpcm", 128);
	np2cfg.vol_pcm    = (UINT8)GetSpinByName(this, "vol_pcm", 128);
	np2cfg.vol_rhythm = (UINT8)GetSpinByName(this, "vol_rhythm", 128);
	np2cfg.davolume   = (UINT8)GetSpinByName(this, "davolume", 128);
	np2cfg.vol_midi   = (UINT8)GetSpinByName(this, "vol_midi", 128);
	np2cfg.MOTORVOL   = (UINT8)GetSpinByName(this, "MOTORVOL", 0);
	for (int i = 0; i < 4; i++) {
		if (m_beepvol[i] && m_beepvol[i]->GetValue()) {
			np2cfg.BEEP_VOL = (UINT8)i; break;
		}
	}
#if defined(SUPPORT_WAB)
	np2cfg.wabasw    = GetCheckByName(this, "wabasw") ? 0 : 1;
#endif
	np2cfg.MOTOR     = GetCheckByName(this, "Seek_Snd") ? 1 : 0;

	/* 14 sub-tab */
	for (int i = 0; i < 6; i++) {
		char name[16];
		snprintf(name, sizeof(name), "vol14_%d", i);
		np2cfg.vol14[i] = (UINT8)GetSpinByName(this, name, 12);
	}

	/* 26 sub-tab */
	{
		UINT8 val = np2cfg.snd26opt & 0x08; // Keep some bits? Actually Windows port sets it fully.
		val = 0; // Reset
		if (auto *w = FindByName(this, "SND26IO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) if (ch->GetSelection() == 1) val |= 0x10;
		}
		if (auto *w = FindByName(this, "SND26INT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				const UINT8 ints[] = {0x00, 0x80, 0xc0, 0x40};
				int sel = ch->GetSelection();
				if (sel >= 0 && sel < 4) val |= ints[sel];
			}
		}
		if (auto *w = FindByName(this, "SND26ROM")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) val |= (ch->GetSelection() & 0x07);
		}
		np2cfg.snd26opt = val;
	}

	/* 86 sub-tab */
	{
		UINT8 val = 0;
		if (auto *w = FindByName(this, "SND86IO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) if (ch->GetSelection() == 0) val |= 0x01;
		}
		if (auto *w = FindByName(this, "SND86INTA")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				const UINT8 ints[] = {0x00, 0x04, 0x0c, 0x08};
				int sel = ch->GetSelection();
				if (sel >= 0 && sel < 4) val |= ints[sel];
			}
		}
		if (auto *w = FindByName(this, "SND86ID")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) val |= ((~ (ch->GetSelection() << 5)) & 0xe0);
		}
		if (GetCheckByName(this, "SND86ROM")) val |= 0x02;
		if (GetCheckByName(this, "SND86INT")) val |= 0x10;
		np2cfg.snd86opt = val;
	}

	/* 118 sub-tab */
	if (auto *w = FindByName(this, "SND118IO")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT16 ios[] = {0x0088, 0x0188, 0x0288, 0x0388};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 4) np2cfg.snd118io = ios[sel];
		}
	}
	if (auto *w = FindByName(this, "SND118ID")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) np2cfg.snd118id = (UINT8)(ch->GetSelection() << 4);
	}
	if (auto *w = FindByName(this, "SND118DMA")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT8 dmas[] = {0, 1, 3};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 3) np2cfg.snd118dma = dmas[sel];
		}
	}
	if (auto *w = FindByName(this, "SND118INTF")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT8 irqs[] = {3, 10, 12, 13};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 4) np2cfg.snd118irqf = irqs[sel];
		}
	}
	if (auto *w = FindByName(this, "SND118INTP")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT8 irqs[] = {3, 5, 10, 12};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 4) np2cfg.snd118irqp = irqs[sel];
		}
	}
	if (auto *w = FindByName(this, "SND118INTM")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) np2cfg.snd118irqm = (ch->GetSelection() == 0 ? 0xff : 10);
	}
	np2cfg.snd118rom = GetCheckByName(this, "SND118ROM") ? 1 : 0;

	/* WSS sub-tab */
	if (auto *w = FindByName(this, "SNDWSSID")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) np2cfg.sndwssid = (UINT8)(ch->GetSelection() << 4);
	}
	if (auto *w = FindByName(this, "SNDWSSDMA")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) np2cfg.sndwssdma = (ch->GetSelection() == 0 ? 1 : 3);
	}
	if (auto *w = FindByName(this, "SNDWSSINT")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT8 irqs[] = {3, 10, 12};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 3) np2cfg.sndwssirq = irqs[sel];
		}
	}

	/* SB16 sub-tab */
	if (auto *w = FindByName(this, "SNDSB16IO")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) np2cfg.sndsb16io = (UINT8)(0xd2 + ch->GetSelection() * 2);
	}
	if (auto *w = FindByName(this, "SNDSB16DMA")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) np2cfg.sndsb16dma = (UINT8)(ch->GetSelection() == 0 ? 0 : 3);
	}
	if (auto *w = FindByName(this, "SNDSB16INT")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT8 irqs[] = {3, 5, 10, 12};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 4) np2cfg.sndsb16irq = irqs[sel];
		}
	}

	/* SpeakBoard sub-tab */
	{
		UINT8 val = 0;
		if (auto *w = FindByName(this, "SPBIO")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) if (ch->GetSelection() == 1) val |= 0x10;
		}
		if (auto *w = FindByName(this, "SPBINT")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				const UINT8 ints[] = {0x00, 0x80, 0xc0, 0x40};
				int sel = ch->GetSelection();
				if (sel >= 0 && sel < 4) val |= ints[sel];
			}
		}
		if (auto *w = FindByName(this, "SPBROM")) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) val |= (ch->GetSelection() & 0x07);
		}
		np2cfg.spbopt = val;
	}
	np2cfg.spb_vrl = (UINT8)GetSpinByName(this, "SPBVRLEVEL", 24);
	np2cfg.spb_x   = GetCheckByName(this, "SPBREVERSE") ? 1 : 0;

	/* Joypad sub-tab */
	if (GetCheckByName(this, "JOYPAD1")) np2oscfg.JOYPAD1 |= 1; else np2oscfg.JOYPAD1 &= ~1;
	if (GetCheckByName(this, "PAD1_POVXY")) np2oscfg.JOYPAD1POVXY |= 1; else np2oscfg.JOYPAD1POVXY &= ~1;
#if defined(SUPPORT_GAMEPORT)
	if (GetCheckByName(this, "PAD1_GAMEPORT")) np2cfg.gameport |= 1; else np2cfg.gameport &= ~1;
	if (GetCheckByName(this, "PAD1_ANALOG")) np2cfg.analogjoy |= 1; else np2cfg.analogjoy &= ~1;
#endif

	/* fmgen sub-tab */
#if defined(SUPPORT_FMGEN)
	np2cfg.usefmgen  = GetCheckByName(this, "USEFMGEN") ? 1 : 0;
#endif

	/* Update core sound state */
	soundmng_setvolume(np2cfg.vol_master);
	opngen_setvol(np2cfg.vol_fm * np2cfg.vol_master / 100);
	psggen_setvol(np2cfg.vol_ssg * np2cfg.vol_master / 100);
	rhythm_setvol(np2cfg.vol_rhythm * np2cfg.vol_master / 100);
	adpcm_setvol(np2cfg.vol_adpcm * np2cfg.vol_master / 100);
	pcm86gen_setvol(np2cfg.vol_pcm * np2cfg.vol_master / 100);
	oplgen_setvol(np2cfg.vol_fm * np2cfg.vol_master / 100);
#if defined(SUPPORT_FMGEN)
	opna_fmgen_setallvolumeFM_linear(np2cfg.vol_fm * np2cfg.vol_master / 100);
	opna_fmgen_setallvolumePSG_linear(np2cfg.vol_ssg * np2cfg.vol_master / 100);
	opna_fmgen_setallvolumeRhythmTotal_linear(np2cfg.vol_rhythm * np2cfg.vol_master / 100);
	opna_fmgen_setallvolumeADPCM_linear(np2cfg.vol_adpcm * np2cfg.vol_master / 100);
#endif
	tms3631_setvol(np2cfg.vol14);
	beep_setvol(np2cfg.BEEP_VOL);

	/* MIDI */
	np2cfg.mpuenable = GetCheckByName(this, "USEMPU98") ? 1 : 0;
	{
		int port = 8, irq = 2;
		if (auto *w = FindByName(this, "MPU_PORT"))
			if (auto *ch = wxDynamicCast(w, wxChoice)) port = ch->GetSelection();
		if (auto *w = FindByName(this, "MPU_IRQ"))
			if (auto *ch = wxDynamicCast(w, wxChoice)) irq  = ch->GetSelection();
		np2cfg.mpuopt = (UINT8)(((port & 0x0f) << 4) | (irq & 3));
	}
	if (auto *w = FindByName(this, "MPU_OUT")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			wxString s = ch->GetSelection() > 0 ? ch->GetString(ch->GetSelection()) : "";
			milstr_ncpy(np2oscfg.MIDIDEV[0], s.ToUTF8().data(), MAX_PATH);
		}
	}
	if (auto *w = FindByName(this, "MPU_IN")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			wxString s = ch->GetSelection() > 0 ? ch->GetString(ch->GetSelection()) : "";
			milstr_ncpy(np2oscfg.MIDIDEV[1], s.ToUTF8().data(), MAX_PATH);
		}
	}
	if (auto *w = FindByName(this, "MPU_MDL")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			wxString s = ch->GetSelection() > 0 ? ch->GetString(ch->GetSelection()) : "";
			milstr_ncpy(np2oscfg.mpu.mdl, s.ToUTF8().data(), sizeof(np2oscfg.mpu.mdl));
		}
	}
	np2oscfg.mpu.def_en = GetCheckByName(this, "MPU_DEF_EN") ? 1 : 0;
	{
		wxString def = GetTextByName(this, "MPU_DEF");
		milstr_ncpy(np2oscfg.mpu.def, def.ToUTF8().data(), MAX_PATH);
	}
#if defined(SUPPORT_SMPU98)
	np2cfg.smpuenable = GetCheckByName(this, "USE_SMPU")  ? 1 : 0;
	np2cfg.smpumuteB  = GetCheckByName(this, "SMPUMUTB") ? 1 : 0;
	{
		int sport = 8, sirq = 2;
		if (auto *w = FindByName(this, "SMPU_PORT"))
			if (auto *ch = wxDynamicCast(w, wxChoice)) sport = ch->GetSelection();
		if (auto *w = FindByName(this, "SMPU_IRQ"))
			if (auto *ch = wxDynamicCast(w, wxChoice)) sirq  = ch->GetSelection();
		np2cfg.smpuopt = (UINT8)(((sport & 0x0f) << 4) | (sirq & 3));
	}
	if (auto *w = FindByName(this, "SMPUA_OUT")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			wxString s = ch->GetSelection() > 0 ? ch->GetString(ch->GetSelection()) : "";
			milstr_ncpy(np2oscfg.MIDIDEVA[0], s.ToUTF8().data(), MAX_PATH);
		}
	}
	if (auto *w = FindByName(this, "SMPUB_OUT")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			wxString s = ch->GetSelection() > 0 ? ch->GetString(ch->GetSelection()) : "";
			milstr_ncpy(np2oscfg.MIDIDEVB[0], s.ToUTF8().data(), MAX_PATH);
		}
	}
#endif

	/* Input */
	if (auto *w = FindByName(this, "keyboard")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			if      (sel == 0) np2oscfg.KEYBOARD = KEY_KEY106;
			else if (sel == 1) np2oscfg.KEYBOARD = KEY_KEY101;
			else               np2oscfg.KEYBOARD = 2; // JoyKey stub
		}
	}
	if (auto *w = FindByName(this, "joypad")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			if (sel > 0) {
				wxString s = ch->GetString(sel);
				milstr_ncpy(np2oscfg.JOYDEV[0], s.ToUTF8().data(), MAX_PATH);
			} else {
				np2oscfg.JOYDEV[0][0] = '\0';
			}
		}
	}
	if (auto *w = FindByName(this, "F12KEY")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) np2oscfg.F12KEY = (UINT8)ch->GetSelection();
	}
	if (auto *w = FindByName(this, "Mouse_sp")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT8 ratios[] = {1, 2, 4, 8, 16};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 5) np2oscfg.mouse_move_ratio = ratios[sel];
		}
	}
	np2oscfg.xrollkey = GetCheckByName(this, "XSHIFT") ? 1 : 0;
	np2oscfg.confirm  = GetCheckByName(this, "DragDrop") ? 1 : 0;
	np2cfg.BTN_RAPID  = GetCheckByName(this, "btnRAPID") ? 1 : 0;
	np2cfg.MOUSERAPID = GetCheckByName(this, "MS_RAPID") ? 1 : 0;

	/* FDD */
	np2cfg.fddequip = 0;
	for (int i = 0; i < 4; i++) {
		char name[16];
		snprintf(name, sizeof(name), "FDDRIVE%d", i + 1);
		if (GetCheckByName(this, name)) np2cfg.fddequip |= (UINT8)(1 << i);
	}
	np2cfg.MOTORVOL = (UINT8)GetSpinByName(this, "Seek_Vol", 50);

	/* HDD / IDE type */
#if defined(SUPPORT_IDEIO)
	for (int i = 0; i < 4; i++) {
		char name[16];
		snprintf(name, sizeof(name), "IDE%dTYPE", i + 1);
		if (auto *w = FindByName(this, name)) {
			if (auto *ch = wxDynamicCast(w, wxChoice)) {
				int sel = ch->GetSelection();
				if      (sel == 0) np2cfg.idetype[i] = SXSIDEV_NC;
				else if (sel == 1) np2cfg.idetype[i] = SXSIDEV_HDD;
				else if (sel == 2) np2cfg.idetype[i] = SXSIDEV_CDROM;
			}
		}
	}
	np2cfg.useasynccd  = GetCheckByName(this, "CD_ASYNC") ? 1 : 0;
	np2cfg.idebios     = GetCheckByName(this, "IDE_BIOS") ? 1 : 0;
	np2cfg.autoidebios = GetCheckByName(this, "AIDEBIOS") ? 1 : 0;
#endif

	/* Serial: all 3 ports (port, bps, data bits, parity, stop bits, DSR check) */
	{
		const UINT32 bpsRates[] = {110,300,600,1200,2400,4800,9600,14400,19200,28800,38400,57600,115200};
		for (int i = 0; i < 3; i++) {
			char portname[16], bpsname[16], dbname[16], parname[16], sbname[16], dsrname[16];
			snprintf(portname, sizeof(portname), "com%dport",   i + 1);
			snprintf(bpsname,  sizeof(bpsname),  "com%d_bps",   i + 1);
			snprintf(dbname,   sizeof(dbname),   "com%d_dbits", i + 1);
			snprintf(parname,  sizeof(parname),  "com%d_parity",i + 1);
			snprintf(sbname,   sizeof(sbname),   "com%d_sbits", i + 1);
			snprintf(dsrname,  sizeof(dsrname),  "com%d_dsr",   i + 1);

			if (auto *w = FindByName(this, portname))
				if (auto *ch = wxDynamicCast(w, wxChoice)) np2oscfg.com[i].port = (UINT8)ch->GetSelection();

			if (auto *w = FindByName(this, bpsname)) {
				if (auto *ch = wxDynamicCast(w, wxChoice)) {
					int sel = ch->GetSelection();
					if (sel >= 0 && sel < 13) np2oscfg.com[i].speed = bpsRates[sel];
				}
			}

			if (auto *w = FindByName(this, dbname)) {
				if (auto *ch = wxDynamicCast(w, wxChoice))
					np2oscfg.com[i].param = (UINT8)((np2oscfg.com[i].param & ~0x0C) |
					                                 ((ch->GetSelection() & 3) << 2));
			}

			if (auto *w = FindByName(this, parname)) {
				if (auto *ch = wxDynamicCast(w, wxChoice)) {
					np2oscfg.com[i].param &= ~0x30;
					switch (ch->GetSelection()) {
					case 1: np2oscfg.com[i].param |= 0x10; break; /* Odd */
					case 2: np2oscfg.com[i].param |= 0x30; break; /* Even */
					default: break;
					}
				}
			}

			if (auto *w = FindByName(this, sbname)) {
				if (auto *ch = wxDynamicCast(w, wxChoice)) {
					np2oscfg.com[i].param &= ~0xC0;
					switch (ch->GetSelection()) {
					case 1: np2oscfg.com[i].param |= 0x80; break; /* 1.5 */
					case 2: np2oscfg.com[i].param |= 0xC0; break; /* 2 */
					default: break;
					}
				}
			}

			np2oscfg.com[i].direct = GetCheckByName(this, dsrname) ? 1 : 0;
		}
	}

	/* Network */
#if defined(SUPPORT_NET)
	{
		wxString tap = GetTextByName(this, "NP2NETTAP");
		strncpy(np2cfg.np2nettap, tap.ToUTF8().data(), MAX_PATH - 1);
		np2cfg.np2nettap[MAX_PATH - 1] = '\0';
	}
#endif
#if defined(SUPPORT_LGY98)
	np2cfg.uselgy98 = GetCheckByName(this, "USELGY98") ? 1 : 0;
	if (auto *w = FindByName(this, "LGY98_IO")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT16 ios[] = {0x00D0,0x10D0,0x20D0,0x30D0,0x40D0,0x50D0,0x60D0,0x70D0};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 8) np2cfg.lgy98io = ios[sel];
		}
	}
	if (auto *w = FindByName(this, "LGY98IRQ")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			const UINT8 irqs[] = {0, 1, 2, 5};
			int sel = ch->GetSelection();
			if (sel >= 0 && sel < 4) np2cfg.lgy98irq = irqs[sel];
		}
	}
#endif

	/* Hostdrv */
#if defined(SUPPORT_HOSTDRV)
	np2cfg.hdrvenable = GetCheckByName(this, "use_hdrv") ? 1 : 0;
	{
		wxString root = GetTextByName(this, "hdrvroot");
		strncpy(np2cfg.hdrvroot, root.ToUTF8().data(), MAX_PATH - 1);
		np2cfg.hdrvroot[MAX_PATH - 1] = '\0';
	}
	np2cfg.hdrvacc = 0;
	if (GetCheckByName(this, "hdrv_acc_r")) np2cfg.hdrvacc |= 1;
	if (GetCheckByName(this, "hdrv_acc_w")) np2cfg.hdrvacc |= 2;
	if (GetCheckByName(this, "hdrv_acc_d")) np2cfg.hdrvacc |= 4;
#if defined(SUPPORT_HOSTDRVNT)
	np2cfg.hdrvntenable = GetCheckByName(this, "hdrv_nt") ? 1 : 0;
#endif
#endif

	/* HDD Equipment: if unchecked, clear the drive type / image path */
#if defined(SUPPORT_IDEIO)
	for (int i = 0; i < 4; i++) {
		char name[16];
		snprintf(name, sizeof(name), "IDE%dEQUIP", i + 1);
		if (!GetCheckByName(this, name)) np2cfg.idetype[i] = SXSIDEV_NC;
	}
#endif
#if defined(SUPPORT_SCSI)
	for (int i = 0; i < 4; i++) {
		char name[16];
		snprintf(name, sizeof(name), "SCSI%dEQUIP", i + 1);
		if (!GetCheckByName(this, name)) np2cfg.scsihdd[i][0] = '\0';
	}
#endif
#if defined(SUPPORT_LIBCDIO)
	np2cfg.libcdio = GetCheckByName(this, "LIBCDIO") ? 1 : 0;
#endif

	/* PCI */
#if defined(SUPPORT_PCI)
	np2cfg.usepci     = GetCheckByName(this, "USE_PCI") ? 1 : 0;
	if (auto *w = FindByName(this, "PCI_PCMC"))
		if (auto *ch = wxDynamicCast(w, wxChoice)) np2cfg.pci_pcmc = (UINT8)(ch->GetSelection() < 3 ? ch->GetSelection() : 0);
	np2cfg.pci_bios32 = GetCheckByName(this, "PCI_B32") ? 1 : 0;
#endif

	/* Misc */
	np2oscfg.NOWAIT    = GetCheckByName(this, "s_NOWAIT") ? 1 : 0;
	np2oscfg.resume    = GetCheckByName(this, "e_resume") ? 1 : 0;
#if defined(SUPPORT_STATSAVE)
	np2cfg.statsave    = GetCheckByName(this, "STATSAVE") ? 1 : 0;
#endif
	np2oscfg.DRAW_SKIP = (UINT8)GetSpinByName(this, "SkpFrame", 0);

	if (auto *w = FindByName(this, "CyclePath"))
		if (auto *fp = wxDynamicCast(w, wxFilePickerCtrl)) milstr_ncpy(cycle_shot_path, fp->GetPath().ToUTF8().data(), 512);
	cycle_shot_interval = (UINT32)GetSpinByName(this, "CycleInt", 3000);

	/* DIP switches already written on-change */

	sysmng_update(SYS_UPDATECFG | SYS_UPDATEOSCFG | SYS_UPDATESBUF |
	              SYS_UPDATEMIDI | SYS_UPDATESBOARD | SYS_UPDATEFDD | SYS_UPDATEHDD);
	initsave();
}

/* ------------------------------------------------------------ */
/*  Buttons                                                     */
/* ------------------------------------------------------------ */

void PrefFrame::OnOK(wxCommandEvent & /*evt*/)
{
	SaveToConfig();
	EndModal(wxID_OK);
}

void PrefFrame::OnCancel(wxCommandEvent & /*evt*/)
{
	EndModal(wxID_CANCEL);
}

void PrefFrame::OnClose(wxCloseEvent &evt)
{
	EndModal(wxID_CANCEL);
	evt.Skip();
}
