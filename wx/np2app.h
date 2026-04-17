/* === wxApp class for NP2kai wx port === */

#ifndef NP2_WX_NP2APP_H
#define NP2_WX_NP2APP_H

#include <wx/wx.h>
#include <wx/app.h>

class Np2App : public wxApp
{
public:
	virtual bool OnInit() wxOVERRIDE;
	virtual int  OnExit() wxOVERRIDE;

private:
	wxString m_configFile;  /* optional -c <file> override */
	bool     m_initialized;

	bool ParseArgs(void);
};

wxDECLARE_APP(Np2App);

#endif /* NP2_WX_NP2APP_H */
