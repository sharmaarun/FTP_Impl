#ifndef CLIENT_UI
#define CLIENT_UI

#include "wx/wx.h"
#include "conio.h"
#include "wx/protocol/ftp.h"
#include "Enums.h"
#include "CConnectDialog.h"
#include "wx/treectrl.h"
#include  "wx/listctrl.h"
#include "wx/tokenzr.h"
#include "wx/dirctrl.h"
#include "wx/file.h"
#include "wx/textfile.h"
#include "wx/filefn.h"
#include "wx/choicdlg.h"

class CClientUI : public wxFrame
{

private:
	//ui items
	wxPanel* mainPanel;

	wxMenuBar* mainMenuBar;
	wxMenu  *fileMenu, *helpMenu;
	wxMenuItem* connectMI, *quitMI,   *aboutMI; 

	wxToolBar* mainToolBar;

	//Connection items
	wxTextCtrl *hostTxtCtrl, *userTxtCtrl, *passTxtCtrl, *portTxtCtrl;
	wxButton* connectButton;
	SConnectionInfo* connectionInfoObj;
	CConnectDialog* dialogConnect;

	//ftp obbject and tools
	wxFTP* ftpClient;
	wxTreeCtrl* clientView_local,* clientView;
	wxTreeItemId crid,srid;
	wxTextCtrl* logBox;
	wxListCtrl* serverListCtrl;
	wxArrayString clientList, serverList;
	wxStaticText* serverPathText;

	wxArrayString selectedFileName;

	//local ctrl
	wxGenericDirCtrl* localSystemList;

public:
	CClientUI(const wxString& title);
	bool init();
	//event functions
	void showConnectDialog(wxCommandEvent& e);
	void itemSelctedList(wxCommandEvent& e);
	void itemSelChangedList(wxCommandEvent& e);
	void itemUnselectedList(wxCommandEvent& e);
	void itemSelctedTree(wxTreeEvent& e);
	void downloadFile(wxCommandEvent& e);
	void uploadFile(wxCommandEvent& e);
	void createFolder(wxCommandEvent& e);
	void renameFile(wxCommandEvent& e);
	void deleteFile(wxCommandEvent& e);
	void exitApp(wxCommandEvent& e);
	void aboutApp(wxCommandEvent& e);

	//main functions
	bool connectToServer();
	bool changeToDir(wxString str);
	void logMessage(wxString str,int type=0);
	bool downloadSelectedFile();
	bool uploadNewFile(wxArrayString str,wxString path);
	
};


#endif