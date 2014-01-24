#include "CServerInfoDialog.h"

CServerInfoDialog::CServerInfoDialog(const wxString & title, wxFrame* parent) : wxDialog(parent, wxID_ANY,title,wxDefaultPosition,wxSize(320,250))
{

	wxPanel* dialogPanel = new wxPanel(this);



	wxStaticBox* infoBox = new wxStaticBox(dialogPanel, wxID_ANY,wxT("Enter Server Information"),wxPoint(5,5),wxSize(300,150));

	wxStaticText* serverNameText = new wxStaticText(dialogPanel,wxID_ANY,wxT("Server Name : "),wxPoint(20,40));
	wxTextCtrl* serverNameTextCtrl = new wxTextCtrl(dialogPanel,wxID_ANY,wxT(""),wxPoint(160,40));

	wxStaticText* serverMsgText = new wxStaticText(dialogPanel,wxID_ANY,wxT("Greeting Message : "),wxPoint(20,70));
	wxTextCtrl* serverMsgTextCtrl = new wxTextCtrl(dialogPanel,wxID_ANY,wxT(""),wxPoint(160,70));

	wxButton* btnOK= new wxButton(dialogPanel,wxID_ANY,wxT("OK"),wxPoint(150,170),wxSize(70,24));
	wxButton* btnCancel= new wxButton(dialogPanel,BASIC_INFO_DIALOG_CANCEL_BUTTON,wxT("Cancel"),wxPoint(230,170),wxSize(70,24));

	this->Connect(BASIC_INFO_DIALOG_CANCEL_BUTTON,wxEVT_COMMAND_BUTTON_CLICKED,wxCommandEventHandler(CServerInfoDialog::onCancel));

	this->ShowModal();
};

void CServerInfoDialog::onCancel(wxCommandEvent& e)
{
	this->Destroy();
};