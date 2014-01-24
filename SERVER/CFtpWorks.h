#ifndef C_FTP_WORKS
#define C_FTP_WORKS

#include "wx/wx.h"
#include "wx/socket.h"
#include "Enums.h"
#include "CFtpServer.h"
#include "mysql++.h"



class CFtpWorks
{

private:
	wxDialog* getInfoDialog;
	wxFrame* parentFrame;
	wxString hostID;
	

	CFtpServer* serverMain;
	ServerConfig* configObj;
	wxTextCtrl* pLogTextBox;
	CFtpServer::UserNode **FtpUser;
	

public:
	CFtpWorks();

	bool startStopServer();
	void setLogTextBox(wxTextCtrl* txtCtrl);
	CFtpServer* getFtpServer();
};

#endif