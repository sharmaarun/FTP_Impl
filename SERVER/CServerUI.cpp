#include "CServerUI.h"
#include "CFtpWorks.h"

CServerUI::CServerUI(const wxString& title) : wxFrame(NULL,wxID_ANY,title,wxPoint(100,100),wxSize(800,600))
{
	
	this->mainPanel = NULL;
	ftpObject = new CFtpWorks();
	
};

bool CServerUI::init()
{
	

	


	//attach default panel to it
	this->mainPanel = new wxPanel(this,MAIN_PANEL_ID);

	//sizers
	wxBoxSizer* mainBox = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* actionSizer = new wxStaticBoxSizer(wxVERTICAL,mainPanel,"Settings");
	 resultSizer = new wxStaticBoxSizer(wxVERTICAL,mainPanel,"...");
	wxBoxSizer *userAccItems = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *uInfoSizer = new wxBoxSizer(wxVERTICAL);
	
	//attach menu to the window

	//--create all menu items
	this->mainMenuBar = new wxMenuBar();

	//--+--file menu
	this->fileMenu = new wxMenu();
	this->startServerMI = new wxMenuItem(fileMenu,MENU_START_SERVER_ID,wxT("Start/Stop Server"));
	this->serverInfoMI = new wxMenuItem(fileMenu,MENU_SERVER_INFO_ID,wxT("Server Information"));
	this->quitMI = new wxMenuItem(fileMenu,MENU_QUIT_ID,wxT("Exit"));
	fileMenu->Append(this->startServerMI);
	fileMenu->Append(this->serverInfoMI);
	fileMenu->Append(this->quitMI);
	this->mainMenuBar->Append(this->fileMenu,wxT("&File"));
	

	
	//--+--help Menu
	this->helpMenu = new wxMenu();
	this->aboutMI = new wxMenuItem(helpMenu,MENU_ABOUT_ID,wxT("&About"));
	helpMenu->Append(aboutMI);
	this->mainMenuBar->Append(helpMenu,wxT("&Help"));

	this->SetMenuBar(mainMenuBar);


	//attach toolbar
	this->mainToolBar = this->CreateToolBar();

	//--create some buttons
	wxImage::AddHandler(new wxPNGHandler);
	wxBitmap serverStartButton(wxT("serveroff.png"),wxBITMAP_TYPE_PNG);
	wxBitmap infoButton(wxT("info.png"),wxBITMAP_TYPE_PNG);
	this->mainToolBar->AddTool(START_STOP_SERVER_BUTTON,serverStartButton,wxT("Start/Stop"));
	this->mainToolBar->AddTool(INFO_DIALOG_BUTTON_ID,infoButton,wxT("Add Info"));
	this->mainToolBar->SetBackgroundColour(wxColour(wxT("GREY")));
	this->mainToolBar->Realize();

	
	

	wxBitmap serverLogBMP(wxT("serverico.png"),wxBITMAP_TYPE_PNG);
	this->serverLogButton = new wxButton(actionSizer->GetStaticBox(),SERVER_LOG_BUTTON,wxT("Server Log"),wxPoint(15,40),wxSize(140,50));
	this->serverLogButton->SetBitmap(serverLogBMP);

	wxBitmap userAccountsBMP(wxT("users.png"),wxBITMAP_TYPE_PNG);
	this->userAccountsButton = new wxButton(actionSizer->GetStaticBox(),USER_ACCOUNTS_BUTTON,wxT("User Accounts"),wxPoint(15,95),wxSize(140,50));
	this->userAccountsButton->SetBitmap(userAccountsBMP);

	wxBitmap configButtonBMP(wxT("config.png"),wxBITMAP_TYPE_PNG);
	this->configButton = new wxButton(actionSizer->GetStaticBox(),CONFIG_BUTTON,wxT("Configuration"),wxPoint(15,150),wxSize(140,50));
	this->configButton->SetBitmap(configButtonBMP);
	this->configButton->Hide();

	//result box items :

	//1 serverlogbox
	serverLogTextBox = new wxTextCtrl(resultSizer->GetStaticBox(),wxID_ANY,wxT("Server is Offline!"),wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE);
	serverLogTextBox->SetPosition(wxPoint(serverLogTextBox->GetPosition()+wxPoint(15,40)));

	//2 user account panel
	
	userList = new wxListCtrl(resultSizer->GetStaticBox(),USER_LIST,wxDefaultPosition,wxSize(200,850), wxLC_REPORT);

	userList->InsertColumn(0,"S.No.");
	userList->InsertColumn(1,"User Name");

	uname_stxt = new wxStaticText(resultSizer->GetStaticBox(),wxID_ANY,"User Name:");
	unameField = new wxTextCtrl(resultSizer->GetStaticBox(),USER_NAME_FIELD);
	path_stxt = new wxStaticText(resultSizer->GetStaticBox(),wxID_ANY,"Assigned Path:");
	pathField = new wxTextCtrl(resultSizer->GetStaticBox(),PATH_FIELD);
	 
	

	serverLogTextBox->SetEditable(false);
	serverLogTextBox->SetBackgroundColour(wxColour(wxT("White")));
	this->ftpObject->setLogTextBox(serverLogTextBox);

	
	
	
	//status bar
	this->CreateStatusBar(3);
	this->SetStatusText("Server Offline!",1);

	//connections to events
	this->Connect(INFO_DIALOG_BUTTON_ID,wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(CServerUI::getNeededInfo));
	this->Connect(START_STOP_SERVER_BUTTON,wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(CServerUI::startServer));
	this->Connect(CONFIG_BUTTON,wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CServerUI::changePanel));
	this->Connect(SERVER_LOG_BUTTON,wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CServerUI::changePanel));
	this->Connect(USER_ACCOUNTS_BUTTON,wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CServerUI::changePanel));
	this->Connect(USER_LIST,wxEVT_COMMAND_LIST_ITEM_SELECTED, wxCommandEventHandler(CServerUI::listSel));

	this->Connect(MENU_START_SERVER_ID,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CServerUI::startServer));
	this->Connect(MENU_SERVER_INFO_ID,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CServerUI::getNeededInfo));

	this->Connect(MENU_QUIT_ID,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CServerUI::quitMe));
	this->Connect(MENU_ABOUT_ID,wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(CServerUI::aboutMe));
	
	actionSizer->Add(serverLogButton,0,   wxALIGN_LEFT | wxALL, 10);
	actionSizer->Add(userAccountsButton,0,   wxALIGN_LEFT | wxALL, 10);
	actionSizer->Add(configButton,0,  wxALIGN_LEFT | wxALL, 10);

	resultSizer->Add(100,5);
	resultSizer->Add(serverLogTextBox,1,wxEXPAND | wxALIGN_LEFT | wxALL, 10);

	userAccItems->Add(userList,0,wxALIGN_LEFT |  wxRIGHT,30);

	uInfoSizer->Add(uname_stxt,0 , wxALIGN_LEFT |  wxTOP,0);
	uInfoSizer->Add(unameField,0 , wxEXPAND| wxALIGN_LEFT |  wxTOP,2);
	uInfoSizer->Add(path_stxt,0 , wxALIGN_LEFT |  wxTOP,10);
	uInfoSizer->Add(pathField,0 , wxEXPAND |wxALIGN_LEFT |  wxTOP,2);

	userAccItems->Add(uInfoSizer,1,wxEXPAND | wxALIGN_LEFT |  wxALL,0);

	resultSizer->Add(userAccItems,1,wxEXPAND | wxALIGN_LEFT |  wxALL,5);
	
	
	
	mainBox->Add(actionSizer,0,wxALIGN_LEFT | wxALL, 10);
	mainBox->Add(resultSizer,1,wxEXPAND | wxALIGN_LEFT | wxALL, 10);
	mainPanel->SetSizer(mainBox);

	//hide unwanted items at first sight
	userList->Hide();
	unameField->Hide();
	pathField->Hide();
	uname_stxt->Hide();
	path_stxt->Hide();

	//show the window
	this->Show(true);
	this->Refresh();
	this->SetMinSize(this->GetSize()-(wxSize(100,150)));
	this->Maximize();
	
	this->Restore();
	Centre();

	return true;
};

void CServerUI::getNeededInfo(wxCommandEvent &e)
{
	
	this->getInfoDialog = new CServerInfoDialog(wxT("Server Information Setup..."),this);
	//this->serverLogTextBox->SetLabelText(this->serverLogTextBox->GetLabelText()+"\n Waiting for connection!");
};


void CServerUI::startServer(wxCommandEvent& e)
{

	this->ftpObject->startStopServer();
	if(this->ftpObject->getFtpServer()->IsListening())
	{
		this->mainToolBar->SetToolNormalBitmap(START_STOP_SERVER_BUTTON,wxBitmap(wxT("serveron.png"),wxBITMAP_TYPE_PNG));
		this->SetStatusText(wxT("Server Online!"),1);
		//this->serverLogButton->SetBitmap(wxBitmap(wxT("serveron.png"),wxBITMAP_TYPE_PNG));
	}
	else
	{
		this->mainToolBar->SetToolNormalBitmap(START_STOP_SERVER_BUTTON,wxBitmap(wxT("serveroff.png"),wxBITMAP_TYPE_PNG));
		this->SetStatusText(wxT("Server Offline!"),1);
	}
}



bool CServerUI::doFillUserPanel()
{

		mysqlpp::Connection conn(false);
        if(!conn.connect("wdmdb", "localhost", "root", ""))
			return false;
        mysqlpp::Query query = conn.query();
 
        /* Now SELECT */
        query << "SELECT * FROM users";
        mysqlpp::StoreQueryResult ares = query.store();
      


		this->userList->DeleteAllItems();
		wxString str="";
		for(int i=0;i<(int)ares.num_rows();i++)
		{			
			str.Printf("%d",i);
			userList->InsertItem(i,"li");
			userList->SetItem(i,0,str);
			userList->SetItem(i,1,ares[i]["username"].c_str());
			str="";
		};
		
return true;
};



void CServerUI::changePanel(wxCommandEvent &e)
{

	wxButton* btn = (wxButton*)(e.GetEventObject());

	wxString lbl = btn->GetLabelText();

	resultSizer->GetStaticBox()->SetLabel(lbl);

	

	if(lbl=="User Accounts")
	{
		userList->SetColumnWidth(0,userList->GetSize().x/4);
		userList->SetColumnWidth(1,userList->GetSize().x*3/4);

		if(!this->doFillUserPanel())
			*this->serverLogTextBox<<"Can not retrive users from db!\n";

		serverLogTextBox->Hide();
		userList->Show();
		unameField->Show();
		pathField->Show();
		uname_stxt->Show();
		path_stxt->Show();
	}
	else
	{
		serverLogTextBox->Show();
		userList->Hide();
		unameField->Hide();
		pathField->Hide();
		uname_stxt->Hide();
		path_stxt->Hide();
	}
	mainPanel->GetSizer()->SetDimension(this->mainPanel->GetSizer()->GetPosition(),mainPanel->GetSizer()->GetSize());
	
};



void CServerUI::aboutMe(wxCommandEvent& e)
{
	wxMessageBox("WDM Server","About");
};

void CServerUI::quitMe(wxCommandEvent& e)
{
	exit(0);
};

void CServerUI::listSel(wxCommandEvent &e)
{
	long item = -1;
    for ( ;; )
    {
        item = userList->GetNextItem(item,
                                     wxLIST_NEXT_ALL,
                                     wxLIST_STATE_SELECTED);
        if ( item == -1 )
            break;

        // this item is selected - do whatever is needed with it
		mysqlpp::Connection conn(false);
        if(!conn.connect("wdmdb", "localhost", "root", ""))
			return;
        mysqlpp::Query query = conn.query();
 
        /* Now SELECT */
		query << "SELECT * FROM users where username='"+userList->GetItemText(item,1)+"'";

		mysqlpp::StoreQueryResult ares = query.store();
      
		
		for(int i=0;i<(int)ares.num_rows();i++)
		{		
			 std::string str=ares[i]["username"];
			unameField->SetValue(str);
			str = ares[i]["path"];
			pathField->SetValue(str);
			str="";

		};
    }
};