/* === Preference dialog for wx port ===
 * wxNotebook-based settings dialog (modal).
 */

#ifndef NP2_WX_DIALOG_PREFFRAME_H
#define NP2_WX_DIALOG_PREFFRAME_H

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/sizer.h>

/* PrefFrame - modal settings dialog */
class PrefFrame : public wxDialog
{
public:
	explicit PrefFrame(wxWindow *parent);
	~PrefFrame();

private:
	wxNotebook *m_notebook;

	/* Pages */
	wxPanel *BuildSystemPage(wxNotebook *nb);
	wxPanel *BuildDisplayPage(wxNotebook *nb);
	wxPanel *BuildSoundPage(wxNotebook *nb);
	wxPanel *BuildInputPage(wxNotebook *nb);
	wxPanel *BuildFddPage(wxNotebook *nb);
	wxPanel *BuildHddPage(wxNotebook *nb);
	wxPanel *BuildMidiPage(wxNotebook *nb);
	wxPanel *BuildSerialPage(wxNotebook *nb);
	wxPanel *BuildNetworkPage(wxNotebook *nb);
	wxPanel *BuildHostdrvPage(wxNotebook *nb);
	wxPanel *BuildDipswPage(wxNotebook *nb);
	wxPanel *BuildCalendarPage(wxNotebook *nb);
	wxPanel *BuildMiscPage(wxNotebook *nb);

	/* DIP switch picture */
	wxPanel *m_dipswPanel;
	void     UpdateDipswPicture(void);

	/* Buttons */
	void OnOK(wxCommandEvent &evt);
	void OnDefault(wxCommandEvent &evt);
	void OnCancel(wxCommandEvent &evt);
	void OnClose(wxCloseEvent &evt);

	/* DIP switch checkboxes (3 bytes × 8 bits = 24) */
	wxCheckBox *m_dipsw[3][8];
	void OnDipswChange(wxCommandEvent &evt);

	/* Sound board choice */
	wxChoice *m_sndboard;
	wxPanel  *m_sndDipsw;
	wxRadioButton *m_beepvol[4];

	/* CPU model choice */
	wxChoice *m_cpumodel;
	wxRadioButton *m_arch[3];
	wxStaticText  *m_cpuMHz;
	void UpdateMHz(void);

	void LoadFromConfig(int tabId = -1);
	void SaveToConfig(void);

	void UpdateDipswBmp(void);

	wxDECLARE_EVENT_TABLE();
};

#endif /* NP2_WX_DIALOG_PREFFRAME_H */
