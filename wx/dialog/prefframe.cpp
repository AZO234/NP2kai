/* === Preference dialog for wx port === */

#include <compiler.h>
#include "prefframe.h"
#include "np2.h"
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

#include <wx/artprov.h>
#include <wx/gbsizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/colordlg.h>
#include <wx/filedlg.h>
#include <wx/statbmp.h>
#include <wx/dc.h>
#include <wx/mstream.h>

/* ---- IDs ---- */
enum {
	ID_PREF_OK      = wxID_OK,
	ID_PREF_CANCEL  = wxID_CANCEL,
	ID_PREF_DEFAULT = wxID_LOWEST + 100,

	ID_DIPSW_BASE = 2000,  /* 2000..2023 = dipsw[0][0..7], [1][0..7], [2][0..7] */
};

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
	"PC-9821Be",
	"PC-9821Xe",
	"PC-9821Cb",
	"PC-9821Cf",
	"PC-9821Xe10,Xa7e,Xb10 build-in",
	"PC-9821Cb2",
	"PC-9821Cx2",
	"PC-9821 PCI",
	"WAB",
	"WSN-A2F",
	"WSN",
	"GA-98NB I/C",
	"GA-98NB II",
	"GA-98NB IV",
	"PC-9821-96",
	"Auto XE G1 PCI",
	"Auto XE G2 PCI",
	"Auto XE G4 PCI",
	"Auto XE WA PCI",
	"Auto XE WS PCI",
	"Auto XE W4 PCI",
	"Auto XE10 WABS",
	"Auto XE10 WSN2",
	"Auto XE10 WSN4",
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
	, m_cpumodel(nullptr)
{
	memset(m_dipsw, 0, sizeof(m_dipsw));

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
		// 10 Misc
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
	m_notebook->AddPage(BuildMiscPage(m_notebook),     "Misc",     false, 11);

	rootSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 4);

	/* Buttons (OK / Cancel) */
	wxStdDialogButtonSizer *btnSizer = new wxStdDialogButtonSizer();
	btnSizer->AddButton(new wxButton(root, ID_PREF_OK,     wxGetStockLabel(wxID_OK)));
	btnSizer->AddButton(new wxButton(root, ID_PREF_CANCEL, wxGetStockLabel(wxID_CANCEL)));
	btnSizer->Realize();
	rootSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 4);

	root->SetSizer(rootSizer);

	LoadFromConfig();
	Centre();
}

PrefFrame::~PrefFrame() {}

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
		for (auto &s : {"None", "GRCG", "GRCG+", "EGC"}) grcg->Append(s);
		grcg->SetSelection(2); /* default GRCG+ */
		gs->Add(grcg, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	}
	row++;

#if defined(SUPPORT_PEGC)
	addCheck("Enable PEGC plane mode", "pegcplane");
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
	vs->AddStretchSpacer(1);
	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildSoundPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxGridBagSizer *gs = new wxGridBagSizer(4, 8);
	int row = 0;

	/* Sound board */
	gs->Add(new wxStaticText(page, wxID_ANY, "Sound Board:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	m_sndboard = new wxChoice(page, wxID_ANY);
	for (auto &n : s_sndboard_names) m_sndboard->Append(n);
	m_sndboard->SetSelection(0);
	gs->Add(m_sndboard, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

	/* Sound DIP switch picture */
	m_sndDipsw = new BmpDipSwPanel(page);
	gs->Add(m_sndDipsw, wxGBPosition(row, 1), wxGBSpan(1, 3), wxALIGN_CENTER);
	m_sndboard->Bind(wxEVT_CHOICE, [this](wxCommandEvent &) { UpdateDipswBmp(); });
	row++;

	/* Sample rate */
	gs->Add(new wxStaticText(page, wxID_ANY, "Sample Rate:"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxChoice *rate = new wxChoice(page, wxID_ANY);
	rate->SetName("SampleHz");
	for (auto &r : {"11025", "22050", "44100", "48000", "88200", "96000"})
		rate->Append(wxString::Format("%s Hz", r));
	rate->SetSelection(2);
	gs->Add(rate, wxGBPosition(row, 1), wxDefaultSpan);
	row++;

	/* Latency (Buffer) */
	gs->Add(new wxStaticText(page, wxID_ANY, "Buffer (ms):"),
	        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *slLat = new wxSlider(page, wxID_ANY, 150, 0, 1000,
	                           wxDefaultPosition, wxSize(200, -1), wxSL_HORIZONTAL | wxSL_LABELS);
	slLat->SetName("Latencys");
	gs->Add(slLat, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
	row++;

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

	/* Volumes */
	auto addVol = [&](const char *label, const char *name, int defval) {
		gs->Add(new wxStaticText(page, wxID_ANY, label),
		        wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *sl = new wxSlider(page, wxID_ANY, defval, 0, 128,
		                        wxDefaultPosition, wxSize(200, -1), wxSL_HORIZONTAL);
		sl->SetName(name);
		gs->Add(sl, wxGBPosition(row, 1), wxGBSpan(1, 3), wxEXPAND);
		row++;
	};
	addVol("FM Volume:",    "volume_F", 128);
	addVol("SSG Volume:",   "volume_S", 128);
	addVol("ADPCM Volume:", "volume_A", 128);
	addVol("PCM Volume:",   "volume_P", 128);
	addVol("Rhythm Volume:","volume_R", 128);

	/* Checkboxes */
	auto addCheck = [&](const char *label, const char *name) {
		auto *cb = new wxCheckBox(page, wxID_ANY, label);
		cb->SetName(name);
		gs->Add(cb, wxGBPosition(row++, 0), wxGBSpan(1, 4));
	};
#if defined(SUPPORT_WAB)
	addCheck("CRT relay sound",     "wabasw");
#endif
#if defined(SUPPORT_FMGEN)
	addCheck("Use fmgen",           "USEFMGEN");
#endif

	gs->AddGrowableCol(1, 1);
	page->SetSizer(new wxBoxSizer(wxVERTICAL));
	page->GetSizer()->Add(gs, 0, wxEXPAND | wxALL, 8);
	page->GetSizer()->AddStretchSpacer(1);
	page->GetSizer()->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	return page;
}

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
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);
	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);

	auto *gs = new wxGridBagSizer(4, 8);
	int row = 0;

#if defined(SUPPORT_IDEIO)
	for (int i = 0; i < 4; i++) {
		gs->Add(new wxStaticText(page, wxID_ANY,
		    wxString::Format("IDE Slot %d:", i + 1)),
		    wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

		wxChoice *type = new wxChoice(page, wxID_ANY);
		type->Append("None");
		type->Append("HDD");
		type->Append("CD-ROM");
		type->SetName(wxString::Format("IDE%dTYPE", i + 1));
		gs->Add(type, wxGBPosition(row, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

		/* Equipment checkbox */
		auto *eq = new wxCheckBox(page, wxID_ANY, "Equipped");
		eq->SetName(wxString::Format("IDE%dEQUIP", i + 1));
		gs->Add(eq, wxGBPosition(row, 2), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		row++;
	}
#else
	for (int i = 0; i < 2; i++) {
		gs->Add(new wxStaticText(page, wxID_ANY,
		    wxString::Format("SASI HDD %d:", i + 1)),
		    wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		auto *eq = new wxCheckBox(page, wxID_ANY, "Equipped");
		eq->SetName(wxString::Format("SASI%dEQUIP", i + 1));
		gs->Add(eq, wxGBPosition(row, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		row++;
	}
#endif

	gs->AddGrowableCol(1, 1);
	vs->Add(gs, 0, wxEXPAND | wxALL, 8);
	vs->AddStretchSpacer(1);
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

	/* Port A */
	smpuGs->Add(new wxStaticText(page, wxID_ANY, "Port A MIDIOUT:"),
	            wxGBPosition(srow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuAOut = new wxChoice(page, wxID_ANY);
	smpuAOut->SetName("SMPUA_OUT"); smpuAOut->Append("N/C"); smpuAOut->Append("VERMOUTH");
	smpuAOut->SetSelection(0);
	smpuGs->Add(smpuAOut, wxGBPosition(srow++, 1), wxGBSpan(1, 3), wxEXPAND);

	/* Port B */
	smpuGs->Add(new wxStaticText(page, wxID_ANY, "Port B MIDIOUT:"),
	            wxGBPosition(srow, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto *smpuBOut = new wxChoice(page, wxID_ANY);
	smpuBOut->SetName("SMPUB_OUT"); smpuBOut->Append("N/C"); smpuBOut->Append("VERMOUTH");
	smpuBOut->SetSelection(0);
	smpuGs->Add(smpuBOut, wxGBPosition(srow++, 1), wxGBSpan(1, 3), wxEXPAND);

	smpuGs->AddGrowableCol(1, 1);
	smpuGs->AddGrowableCol(3, 1);
	smpuBox->Add(smpuGs, 0, wxEXPAND | wxALL, 4);
	vs->Add(smpuBox, 0, wxEXPAND | wxALL, 6);
#endif

	vs->Add(new wxButton(page, ID_PREF_DEFAULT, "Default"), 0, wxALIGN_RIGHT | wxALL, 8);
	page->SetSizer(vs);
	return page;
}

wxPanel *PrefFrame::BuildSerialPage(wxNotebook *nb)
{
	wxScrolledWindow *page = new wxScrolledWindow(nb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxVSCROLL);
	page->SetScrollRate(0, 20);

	wxBoxSizer *vs = new wxBoxSizer(wxVERTICAL);

	/* COM1 full settings */
	vs->Add(BuildComPortBox(page, 1, "com1port", "com1_bps", "com1_dsr"), 0, wxEXPAND | wxALL, 4);
	/* COM2 */
	vs->Add(BuildComPortBox(page, 2, "com2port", "com2_bps", "com2_dsr"), 0, wxEXPAND | wxALL, 4);
	/* COM3 */
	vs->Add(BuildComPortBox(page, 3, "com3port", "com3_bps", "com3_dsr"), 0, wxEXPAND | wxALL, 4);

	/* PC-9861K T.B.D. */
	vs->Add(new wxStaticText(page, wxID_ANY, "PC-9861K, CH.1, CH.2, Parallel: T.B.D."),
	        0, wxALL, 8);

	vs->AddStretchSpacer(1);
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

	/* Checkbox grid: 3 banks × 8 bits */
	wxGridBagSizer *gs = new wxGridBagSizer(2, 4);
	for (int bank = 0; bank < 3; bank++) {
		gs->Add(new wxStaticText(page, wxID_ANY,
		    wxString::Format("SW%d:", bank + 1)),
		    wxGBPosition(0, bank * 9), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
		for (int bit = 0; bit < 8; bit++) {
			auto *cb = new wxCheckBox(page, ID_DIPSW_BASE + bank * 8 + bit,
			    wxString::Format("%d", bit + 1));
			m_dipsw[bank][bit] = cb;
			gs->Add(cb, wxGBPosition(1, bank * 9 + bit), wxDefaultSpan);
		}
	}
	vs->Add(gs, 0, wxALL | wxCENTER, 8);
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

/* Helper: recursively find first child with name */
static wxWindow *FindByName(wxWindow *parent, const wxString &name)
{
	if (parent->GetName() == name) return parent;
	for (auto *child : parent->GetChildren()) {
		auto *found = FindByName(child, name);
		if (found) return found;
	}
	return nullptr;
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
	if (!m_sndDipsw) return;
	auto *panel = (BmpDipSwPanel *)m_sndDipsw;

	int sel = m_sndboard->GetSelection();
	if (sel < 0 || sel >= (int)NELEMENTS(s_sndboard_vals)) return;
	UINT8 board = s_sndboard_vals[sel];
	void *data = nullptr;

	switch (board) {
	case 0x02: // 26K
		data = dipswbmp_getsnd26(np2cfg.snd26opt);
		break;
	case 0x04: // 86
	case 0x06: // 86 + 26K
	case 0x14: // 86 with ADPCM
		data = dipswbmp_getsnd86(np2cfg.snd86opt);
		break;
	case 0x20: // Speak Board
		data = dipswbmp_getsndspb(np2cfg.spbopt, np2cfg.spb_vrc);
		break;
	case 0x08: // 118
		data = dipswbmp_getsnd118(np2cfg.snd118io, np2cfg.snd118dma, 
		                          np2cfg.snd118irqf, np2cfg.snd118irqp, 
		                          np2cfg.snd118irqm, np2cfg.snd118rom);
		break;
	default:
		break;
	}

	if (data) {
		panel->SetBmpData(data);
		panel->Show(true);
		_MFREE(data);
	} else {
		panel->Show(false);
	}
	panel->GetParent()->Layout();
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
	if (wxMessageBox("Reset all settings to default?", "Preference", wxYES_NO | wxICON_QUESTION) != wxYES)
		return;

	extern void pccore_setdefault(void);
	pccore_setdefault();
	LoadFromConfig();
}

void PrefFrame::LoadFromConfig(void)
{
	/* System */
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
	SetCheckByName(this, "MULTITHREAD", np2wabcfg.multithread != 0);
	SetCheckByName(this, "calendar",   np2cfg.calendar != 0);
	SetCheckByName(this, "USE144FD",   np2cfg.usefd144 != 0);
	SetCheckByName(this, "TIMERFIX",   np2cfg.timerfix != 0);
	SetCheckByName(this, "CONSTTSC",   np2cfg.consttsc != 0);
#if defined(SUPPORT_ASYNC_CPU)
	SetCheckByName(this, "ASYNCCPU",   np2cfg.asynccpu != 0);
#endif

	/* Display */
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

	/* Sound board */
	if (m_sndboard) {
		int sel = 0;
		for (int i = 0; i < (int)(sizeof(s_sndboard_vals)); i++) {
			if (s_sndboard_vals[i] == np2cfg.SOUND_SW) { sel = i; break; }
		}
		m_sndboard->SetSelection(sel);
		UpdateDipswBmp();
	}
	if (auto *w = FindByName(this, "SampleHz")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = 2; // Default 44100
			if      (np2cfg.samplingrate <= 11025) sel = 0;
			else if (np2cfg.samplingrate <= 22050) sel = 1;
			else if (np2cfg.samplingrate <= 44100) sel = 2;
			else if (np2cfg.samplingrate <= 48000) sel = 3;
			else if (np2cfg.samplingrate <= 88200) sel = 4;
			else                                   sel = 5;
			ch->SetSelection(sel);
		}
	}
	SetSpinByName(this, "Latencys",  (int)np2cfg.delayms);
	/* Beep Volume */
	{
		int vol = np2cfg.BEEP_VOL & 3;
		if (m_beepvol[vol]) m_beepvol[vol]->SetValue(true);
	}
	
#if defined(SUPPORT_FMGEN)
	SetCheckByName(this, "USEFMGEN", np2cfg.usefmgen != 0);
#endif
	SetCheckByName(this, "Seek_Snd", np2cfg.MOTOR != 0);
#if defined(SUPPORT_WAB)
	SetCheckByName(this, "wabasw",   np2cfg.wabasw == 0); // "CRT relay sound" checked means wabasw=0
#endif

	/* MIDI */
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

	/* Input */
	if (auto *w = FindByName(this, "keyboard")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = 0;
			if      (np2oscfg.KEYBOARD == KEY_KEY101) sel = 1;
			else if (np2oscfg.KEYBOARD >= 2)          sel = 2; // Stub for JoyKey
			ch->SetSelection(sel);
		}
	}
	if (auto *w = FindByName(this, "F12KEY")) {
		if (auto *ch = wxDynamicCast(w, wxChoice))
			ch->SetSelection(np2oscfg.F12KEY & 7);
	}
	if (auto *w = FindByName(this, "Mouse_sp")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = 2;
			if      (np2oscfg.mouse_move_ratio <= 1) sel = 0;
			else if (np2oscfg.mouse_move_ratio <= 2) sel = 1;
			else if (np2oscfg.mouse_move_ratio <= 4) sel = 2;
			else if (np2oscfg.mouse_move_ratio <= 8) sel = 3;
			else                                     sel = 4;
			ch->SetSelection(sel);
		}
	}
	SetCheckByName(this, "XSHIFT",   np2oscfg.xrollkey != 0);
	SetCheckByName(this, "DragDrop", np2oscfg.confirm != 0);
	SetCheckByName(this, "btnRAPID", np2cfg.BTN_RAPID != 0);
	SetCheckByName(this, "MS_RAPID", np2cfg.MOUSERAPID != 0);

	/* FDD */
	for (int i = 0; i < 4; i++) {
		char name[16];
		snprintf(name, sizeof(name), "FDDRIVE%d", i + 1);
		SetCheckByName(this, name, ((np2cfg.fddequip >> i) & 1) != 0);
	}
	SetSpinByName(this, "Seek_Vol",  (int)np2cfg.MOTORVOL);

	/* HDD / IDE type */
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

	/* Hostdrv */
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

	/* HDD Equipment */
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

	/* PCI */
#if defined(SUPPORT_PCI)
	SetCheckByName(this, "USE_PCI", np2cfg.usepci != 0);
	if (auto *w = FindByName(this, "PCI_PCMC"))
		if (auto *ch = wxDynamicCast(w, wxChoice)) ch->SetSelection(np2cfg.pci_pcmc < 3 ? np2cfg.pci_pcmc : 0);
	SetCheckByName(this, "PCI_B32", np2cfg.pci_bios32 != 0);
#endif

	/* Misc */
	SetCheckByName(this, "s_NOWAIT", np2oscfg.NOWAIT != 0);
	SetCheckByName(this, "e_resume", np2oscfg.resume != 0);
#if defined(SUPPORT_STATSAVE)
	SetCheckByName(this, "STATSAVE", np2cfg.statsave != 0);
#endif
	SetSpinByName(this, "SkpFrame", (int)np2oscfg.DRAW_SKIP);

	UpdateDipswPicture();
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
	np2wabcfg.multithread = GetCheckByName(this, "MULTITHREAD") ? 1 : 0;

	// np2cfg.grcg      = GetCheckByName(this, "grcg")    ? 3 : 0;
	np2cfg.color16   = GetCheckByName(this, "color16b") ? 1 : 0;
	np2cfg.calendar  = GetCheckByName(this, "calendar") ? 1 : 0;
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
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			np2cfg.grcg = (UINT8)(sel >= 0 && sel < 4 ? sel : 0);
		}
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

	/* Sound */
	if (m_sndboard) {
		int sel = m_sndboard->GetSelection();
		if (sel >= 0 && sel < (int)(sizeof(s_sndboard_vals)))
			np2cfg.SOUND_SW = s_sndboard_vals[sel];
	}
	if (auto *w = FindByName(this, "SampleHz")) {
		if (auto *ch = wxDynamicCast(w, wxChoice)) {
			int sel = ch->GetSelection();
			const UINT32 rates[] = {11025, 22050, 44100, 48000, 88200, 96000};
			if (sel >= 0 && sel < 6) np2cfg.samplingrate = rates[sel];
		}
	}
	np2cfg.delayms   = (UINT16)GetSpinByName(this, "Latencys", 150);
	/* Beep Volume */
	for (int i = 0; i < 4; i++) {
		if (m_beepvol[i] && m_beepvol[i]->GetValue()) {
			np2cfg.BEEP_VOL = (UINT8)i;
			break;
		}
	}
#if defined(SUPPORT_FMGEN)
	np2cfg.usefmgen  = GetCheckByName(this, "USEFMGEN") ? 1 : 0;
#endif
	np2cfg.MOTOR     = GetCheckByName(this, "Seek_Snd") ? 1 : 0;
#if defined(SUPPORT_WAB)
	np2cfg.wabasw    = GetCheckByName(this, "wabasw") ? 0 : 1; // Checked means relay sound (wabasw=0)
#endif

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

	/* HDD Equipment: if unchecked, clear the drive type */
#if defined(SUPPORT_IDEIO)
	for (int i = 0; i < 4; i++) {
		char name[16];
		snprintf(name, sizeof(name), "IDE%dEQUIP", i + 1);
		if (!GetCheckByName(this, name)) np2cfg.idetype[i] = SXSIDEV_NC;
	}
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
