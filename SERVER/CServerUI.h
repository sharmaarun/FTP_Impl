#ifndef C_SERVER_UI
#define C_SERVER_UI

#include "wx/wx.h"
#include "Enums.h"
#include "CFtpWorks.h"
#include "CServerInfoDialog.h"
#include "string.h"
#include "wx/listctrl.h"
#include "mysql++.h"


class CServerUI : public wxFrame
{
private:

	// all the UI elements 

	//--main items
	
	wxPanel* mainPanel;
	
	wxAnyButton *serverLogButton,*userAccountsButton,*configButton;
	
	wxTextCtrl* serverLogTextBox;

	//user acc panel items
	wxListCtrl* userList;
	wxStaticText* uname_stxt,*path_stxt;
	wxTextCtrl* unameField,*pathField;
	wxButton* updateButton;
	wxStaticBoxSizer* userInfoSizer;
	wxStaticBoxSizer* resultSizer;

	//--menu and toolbar items
	wxMenuBar* mainMenuBar;
	wxMenu  *fileMenu, *helpMenu;
	wxMenuItem* startServerMI, *quitMI, *serverInfoMI, *aboutMI; 


	wxToolBar* mainToolBar;
	wxStaticBox* resultBox;



	CServerInfoDialog* getInfoDialog;

	CFtpWorks* ftpObject;

public:

	CServerUI(const wxString& title);
	bool init();
	wxPanel* getMainPanel();
	bool doFillUserPanel();

	void changePanel(wxCommandEvent& e);
	void getNeededInfo(wxCommandEvent& e);
	void startServer(wxCommandEvent& e);
	void quitMe(wxCommandEvent& e);
	void aboutMe(wxCommandEvent& e);
	void listSel(wxCommandEvent& e);
};


#endif