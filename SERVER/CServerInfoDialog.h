#ifndef C_SERVER_INFO_DIALOG
#define C_SERVER_INFO_DIALOG

#include "wx/wx.h"
#include "Enums.h"
class CServerInfoDialog : wxDialog
{
public:
	
	CServerInfoDialog(const wxString& title, wxFrame* parent);
	void onCancel(wxCommandEvent& e);
};

#endif