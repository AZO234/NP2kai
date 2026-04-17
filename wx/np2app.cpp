/* === wxApp class for NP2kai wx port === */

#include <compiler.h>
#include "np2app.h"
#include "np2frame.h"
#include "np2.h"
#include "ini.h"

wxIMPLEMENT_APP(Np2App);

bool Np2App::OnInit()
{
	m_initialized = false;

	/* wxWidgets image handlers (for PNG icons etc.) */
	wxInitAllImageHandlers();

	/* SDL: init audio only (no video - that's wxWidgets' job) */
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) < 0) {
		wxMessageBox(wxString::Format("SDL_Init failed: %s", SDL_GetError()),
		             "Error", wxOK | wxICON_ERROR);
		return false;
	}

	/* Parse command-line options */
	if (!ParseArgs()) return false;

	/* Initialize emulator core */
	wxString argv0 = argv[0];
	if (np2_initialize(argv0.ToUTF8().data()) != SUCCESS) {
		wxMessageBox("Failed to initialize emulator core.", "Error",
		             wxOK | wxICON_ERROR);
		SDL_Quit();
		return false;
	}
	m_initialized = true;

	/* Determine initial window size */
	int w = (int)np2oscfg.winwidth;
	int h = (int)np2oscfg.winheight;
	if (w < 320) w = 640;
	if (h < 200) h = 400;

	wxPoint pos(wxDefaultPosition);
	if (np2oscfg.winx >= 0 && np2oscfg.winy >= 0)
		pos = wxPoint(np2oscfg.winx, np2oscfg.winy);

#if defined(CPUCORE_IA32)
	wxString title = "wx NP21kai (IA-32)";
#else
	wxString title = "wx NP2kai (286)";
#endif

	Np2Frame *frame = new Np2Frame(title, pos, wxSize(w, h));
	/* w/h are client-area dimensions; adjust frame size accordingly */
	frame->SetClientSize(w, h);
	frame->Show(true);
	SetTopWindow(frame);

	return true;
}

int Np2App::OnExit()
{
	/* np2_terminate() is called from Np2Frame::OnClose */
	SDL_Quit();
	return wxApp::OnExit();
}

bool Np2App::ParseArgs(void)
{
	for (int i = 1; i < argc; i++) {
		wxString arg = argv[i];
		if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
			/* Override config path via environment variable picked up by initgetfile */
			wxString next = argv[++i];
			wxSetEnv("NP2KAI_CONFIG", next);
		} else if (arg == "-h" || arg == "--help") {
			wxMessageBox(
			    wxString::Format(
			        "Usage: %s [options]\n"
			        "  -c <file>   Config file path\n"
			        "  -h          Help\n",
			        argv[0]),
			    "NP2kai wx", wxOK | wxICON_INFORMATION);
			return false;
		}
	}
	return true;
}
