/* === Key display sub-window for wx port === */

#include <compiler.h>
#include "keydisp.h"
#include <keystat.h>
#include <wx/dcclient.h>

/* PC-98 key labels for scan code display.
 * Index = PC-98 make code (0x00-0x7f).
 * Empty string means hidden.
 */
static const char *s_keynames[128] = {
/*00*/ "ESC",  "1",   "2",   "3",   "4",   "5",   "6",   "7",
/*08*/ "8",    "9",   "0",   "-",   "^",   "\\",  "BS",  "TAB",
/*10*/ "Q",    "W",   "E",   "R",   "T",   "Y",   "U",   "I",
/*18*/ "O",    "P",   "@",   "[",   "ENT", "A",   "S",   "D",
/*20*/ "F",    "G",   "H",   "J",   "K",   "L",   ";",   ":",
/*28*/ "]",    "Z",   "X",   "C",   "V",   "B",   "N",   "M",
/*30*/ ",",    ".",   "/",   "_",   "SP",  "↑",  "↓",  "←",
/*38*/ "→",  "INS", "DEL", "↑2", "→2","N7", "N8", "N9",
/*40*/ "-N",  "N4",  "N5",  "N6",  "+N",  "N1",  "N2",  "N3",
/*48*/ "N0",  ".",   "ENT2","",    "",    "",    "VF1", "VF2",
/*50*/ "VF3", "VF4", "VF5", "",    "",    "",    "",    "",
/*58*/ "",    "",    "",    "",    "",    "",    "",    "",
/*60*/ "STOP","COPY","F1",  "F2",  "F3",  "F4",  "F5",  "F6",
/*68*/ "F7",  "F8",  "F9",  "F10", "",    "",    "",    "",
/*70*/ "SHFT","CAPS","KANA","GRPH","CTRL","",    "",    "",
/*78*/ "",    "",    "",    "",    "",    "",    "",    "",
};

/* Layout: simplified keyboard-like grid */
struct KeyPos { int x, y, w, h; int sc; };

/* A small representative set of keys for the display */
static const KeyPos s_keys[] = {
	/* Row 0: ESC + F keys */
	{  2,  2, 22, 18, 0x00 }, /* ESC */
	{ 30,  2, 20, 18, 0x62 }, /* F1  */
	{ 52,  2, 20, 18, 0x63 }, /* F2  */
	{ 74,  2, 20, 18, 0x64 }, /* F3  */
	{ 96,  2, 20, 18, 0x65 }, /* F4  */
	{118,  2, 20, 18, 0x66 }, /* F5  */
	{144,  2, 20, 18, 0x67 }, /* F6  */
	{166,  2, 20, 18, 0x68 }, /* F7  */
	{188,  2, 20, 18, 0x69 }, /* F8  */
	{210,  2, 20, 18, 0x6a }, /* F9  */
	{232,  2, 20, 18, 0x6b }, /* F10 */
	/* Row 1: numbers */
	{  2, 24, 18, 18, 0x01 }, {  22, 24, 18, 18, 0x02 }, {  42, 24, 18, 18, 0x03 },
	{  62, 24, 18, 18, 0x04 }, {  82, 24, 18, 18, 0x05 }, { 102, 24, 18, 18, 0x06 },
	{ 122, 24, 18, 18, 0x07 }, { 142, 24, 18, 18, 0x08 }, { 162, 24, 18, 18, 0x09 },
	{ 182, 24, 18, 18, 0x0a }, { 202, 24, 18, 18, 0x0b }, { 222, 24, 18, 18, 0x0c },
	{ 242, 24, 28, 18, 0x0e }, /* BS */
	/* Row 2: QWERTY */
	{  2, 44, 28, 18, 0x0f }, /* TAB */
	{  32, 44, 18, 18, 0x10 }, {  52, 44, 18, 18, 0x11 }, {  72, 44, 18, 18, 0x12 },
	{  92, 44, 18, 18, 0x13 }, { 112, 44, 18, 18, 0x14 }, { 132, 44, 18, 18, 0x15 },
	{ 152, 44, 18, 18, 0x16 }, { 172, 44, 18, 18, 0x17 }, { 192, 44, 18, 18, 0x18 },
	{ 212, 44, 18, 18, 0x19 }, { 232, 44, 18, 18, 0x1a }, { 252, 44, 18, 18, 0x1c }, /* ENT*/
	/* Row 3: ASDF */
	{  8, 64, 24, 18, 0x74 }, /* CTRL */
	{  34, 64, 18, 18, 0x1d }, {  54, 64, 18, 18, 0x1e }, {  74, 64, 18, 18, 0x1f },
	{  94, 64, 18, 18, 0x20 }, { 114, 64, 18, 18, 0x21 }, { 134, 64, 18, 18, 0x22 },
	{ 154, 64, 18, 18, 0x23 }, { 174, 64, 18, 18, 0x24 }, { 194, 64, 18, 18, 0x25 },
	{ 214, 64, 18, 18, 0x26 }, { 234, 64, 18, 18, 0x27 }, { 254, 64, 18, 18, 0x28 },
	/* Row 4: ZXCV + modifiers */
	{  2, 84, 34, 18, 0x70 }, /* SHIFT */
	{  38, 84, 18, 18, 0x29 }, {  58, 84, 18, 18, 0x2a }, {  78, 84, 18, 18, 0x2b },
	{  98, 84, 18, 18, 0x2c }, { 118, 84, 18, 18, 0x2d }, { 138, 84, 18, 18, 0x2e },
	{ 158, 84, 18, 18, 0x2f }, { 178, 84, 18, 18, 0x30 }, { 198, 84, 18, 18, 0x31 },
	{ 218, 84, 18, 18, 0x32 },
	{ 238, 84, 34, 18, 0x70 }, /* SHIFT (right) */
	/* Row 5: bottom row */
	{  2, 104, 22, 18, 0x71 }, /* CAPS */
	{ 26, 104, 22, 18, 0x72 }, /* KANA */
	{ 50, 104, 22, 18, 0x73 }, /* GRPH */
	{ 74, 104, 100, 18, 0x35 }, /* SPACE */
};

/* ---- event table ---- */
wxBEGIN_EVENT_TABLE(KeyDispFrame, wxFrame)
	EVT_TIMER(wxID_ANY, KeyDispFrame::OnTimer)
	EVT_CLOSE(KeyDispFrame::OnClose)
wxEND_EVENT_TABLE()

KeyDispFrame::KeyDispFrame(wxWindow *parent)
	: wxFrame(parent, wxID_ANY, "Key Display",
	          wxDefaultPosition, wxSize(290, 130),
	          wxCAPTION | wxCLOSE_BOX | wxFRAME_FLOAT_ON_PARENT |
	          wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP)
	, m_refreshTimer(this)
{
	m_panel = new wxPanel(this);
	m_panel->Bind(wxEVT_PAINT, &KeyDispFrame::OnPaint, this);
	m_panel->SetBackgroundStyle(wxBG_STYLE_PAINT);

	wxBoxSizer *sz = new wxBoxSizer(wxVERTICAL);
	sz->Add(m_panel, 1, wxEXPAND);
	SetSizer(sz);

	m_refreshTimer.Start(50); /* ~20 fps refresh */
}

KeyDispFrame::~KeyDispFrame()
{
	m_refreshTimer.Stop();
}

void KeyDispFrame::OnTimer(wxTimerEvent & /*evt*/)
{
	if (m_panel) m_panel->Refresh(false);
}

void KeyDispFrame::OnPaint(wxPaintEvent & /*evt*/)
{
	wxPaintDC dc(m_panel);
	DrawKeys(dc);
}

void KeyDispFrame::DrawKeys(wxDC &dc)
{
	dc.SetBackground(wxBrush(wxColour(30, 30, 30)));
	dc.Clear();

	dc.SetFont(wxFont(6, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	for (auto &k : s_keys) {
		if (k.sc < 0 || k.sc >= 128) continue;
		const char *lbl = s_keynames[k.sc];
		if (!lbl || !lbl[0]) continue;

		bool pressed = (keystat.ref[k.sc] != NKEYREF_NC);

		/* Key background */
		if (pressed) {
			dc.SetBrush(wxBrush(wxColour(100, 200, 100)));
			dc.SetPen(wxPen(wxColour(0, 150, 0)));
		} else {
			dc.SetBrush(wxBrush(wxColour(80, 80, 80)));
			dc.SetPen(wxPen(wxColour(140, 140, 140)));
		}
		dc.DrawRoundedRectangle(k.x, k.y, k.w, k.h, 2);

		/* Key label */
		dc.SetTextForeground(pressed ? *wxBLACK : *wxWHITE);
		wxString lstr(lbl, wxConvUTF8);
		wxCoord tw, th;
		dc.GetTextExtent(lstr, &tw, &th);
		dc.DrawText(lstr, k.x + (k.w - tw) / 2, k.y + (k.h - th) / 2);
	}
}

void KeyDispFrame::OnClose(wxCloseEvent &evt)
{
	m_refreshTimer.Stop();
	Hide();    /* hide rather than destroy, so parent can re-show */
	evt.Veto();
}
