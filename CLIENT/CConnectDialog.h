#ifndef C_CONNECT_DIALOG
#define C_CONNECT_DIALOG

#include "wx/wx.h"
#include "wx/protocol/ftp.h"
#include "Enums.h"
class CConnectDialog : wxDialog
{

private:

	SConnectionInfo* infoObj;
	wxTextCtrl* hostT,*userT,*passT,*portT;

	wxFTP* ftpObjMain;

public:
	
	enum
	{
		ID_CONNECT = 100
	};

	CConnectDialog(const wxString& title, wxFrame* parent,SConnectionInfo* connectInfoObj, wxFTP* ftpObj);
	
	void onConnect(wxCommandEvent& e);
	
	
};

#endif