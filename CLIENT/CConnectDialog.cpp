#include "CConnectDialog.h"

CConnectDialog::CConnectDialog(const wxString& title, wxFrame* parent,SConnectionInfo* connectionInfoObj,wxFTP* ftpObj) : wxDialog(parent,wxID_ANY,title,wxDefaultPosition,wxSize(250,210))
{

	this->infoObj = connectionInfoObj;
	infoObj->setConnect=false;
	this->ftpObjMain = ftpObj;

	wxPanel* connectPanel= new wxPanel(this);

	wxStaticText* host = new wxStaticText(this,wxID_ANY,wxT("Host \t : "),wxPoint(20,25));	
	wxStaticText* user = new wxStaticText(this,wxID_ANY,wxT("UserName \t : "),wxPoint(20,55));	
	wxStaticText* pass = new wxStaticText(this,wxID_ANY,wxT("Password \t : "),wxPoint(20,85));	
	wxStaticText* portt = new wxStaticText(this,wxID_ANY,wxT("Port \t : "),wxPoint(20,115));	

	infoObj->hostid="localhost";
	infoObj->userid="demo";
	infoObj->password="demo";

	hostT = new wxTextCtrl(this,wxID_ANY,infoObj->hostid,wxPoint(120,25));
	userT = new wxTextCtrl(this,wxID_ANY,infoObj->userid,wxPoint(120,55));
	passT = new wxTextCtrl(this,wxID_ANY,infoObj->password,wxPoint(120,85));
	portT = new wxTextCtrl(this,wxID_ANY,infoObj->port,wxPoint(120,115));

	wxButton* connectButton= new wxButton(this,ID_CONNECT,wxT("Connect"),wxPoint(80,145),wxSize(80,30));

	this->Connect(ID_CONNECT,wxEVT_COMMAND_BUTTON_CLICKED,wxCommandEventHandler(CConnectDialog::onConnect));

	this->SetParent(parent);
	Center();
	this->ShowModal();
	
};

void CConnectDialog::onConnect(wxCommandEvent& e)
{
	this->infoObj->hostid  = hostT->GetValue();
	this->infoObj->userid  = userT->GetValue();
	this->infoObj->password  = passT->GetValue();
	this->infoObj->port  = portT->GetValue();

	if(infoObj->hostid=="")
	{
		wxMessageBox("Error : Please enter complete HOST ID","Error");
	}
	else
	{
		this->infoObj->setConnect = true;
		if(ftpObjMain->IsConnected())
		{
			if(!ftpObjMain->Close())
			{
				wxMessageBox("Could not close the connection!");
			}
			else
			{
				//this->infoObj->userid="";
				//this->infoObj->password="";
			}
		}
		this->Destroy();
	};
	
	
};


