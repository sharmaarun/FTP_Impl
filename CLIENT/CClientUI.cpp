#include "CClientUI.h"
#include "wx/protocol/ftp.h"


CClientUI::CClientUI(const wxString& title) : wxFrame(NULL ,wxID_ANY,title,wxDefaultPosition,wxSize(800,600))
{
	ftpClient = new wxFTP();
	this->connectionInfoObj = new SConnectionInfo();
	
	
};

bool CClientUI::init()
{



	this->mainPanel = new wxPanel(this);
	

	//menu bar
	//--create all menu items
	this->mainMenuBar = new wxMenuBar();

	//--+--file menu
	this->fileMenu = new wxMenu();
	this->connectMI = new wxMenuItem(fileMenu,MENU_CONNECT_ID,wxT("Connect"));
	this->quitMI = new wxMenuItem(fileMenu,MENU_QUIT_ID,wxT("Exit"));
	fileMenu->Append(this->connectMI);
	
	fileMenu->Append(this->quitMI);
	this->mainMenuBar->Append(this->fileMenu,wxT("&File"));
	
	
	
	//--+--help Menu
	this->helpMenu = new wxMenu();
	this->aboutMI = new wxMenuItem(helpMenu,MENU_ABOUT_ID,wxT("&About"));
	helpMenu->Append(aboutMI);
	this->mainMenuBar->Append(helpMenu,wxT("&Help"));

	this->SetMenuBar(mainMenuBar);


	this->mainToolBar = this->CreateToolBar();

	wxBoxSizer* vBoxMain = new wxBoxSizer(wxVERTICAL);
	
	wxBoxSizer* hBoxTop = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* vBoxBottom = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* vBoxCVS = new wxBoxSizer(wxVERTICAL);		//server  view sizer
	wxBoxSizer* vBoxCVC = new wxBoxSizer(wxVERTICAL);		//client  view szer
	wxBoxSizer* hBoxCVS_TEXT = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* hBoxCVC_TEXT = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* hBoxCVS_TV = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* hBoxCVC_TV = new wxBoxSizer(wxHORIZONTAL);

	wxImage::AddHandler(new wxPNGHandler);
	mainToolBar->AddTool(ID_TOOL_CONNECT,wxBitmap(wxT("clientconnect.png"),wxBITMAP_TYPE_PNG),wxT("Connect to Server"));
	mainToolBar->AddTool(ID_TOOL_DOWNLOAD,wxBitmap(wxT("download.png"),wxBITMAP_TYPE_PNG),wxT("Download File"));
	mainToolBar->AddTool(ID_TOOL_UPLOAD,wxBitmap(wxT("upload.png"),wxBITMAP_TYPE_PNG),wxT("Upload File"));
	mainToolBar->AddTool(ID_TOOL_NEWFOLDER,wxBitmap(wxT("newfolder.png"),wxBITMAP_TYPE_PNG),wxT("Create New Folder"));
	mainToolBar->AddTool(ID_TOOL_RENAME,wxBitmap(wxT("rename.png"),wxBITMAP_TYPE_PNG),wxT("Rename Selected File"));
	mainToolBar->AddTool(ID_TOOL_DELETE,wxBitmap(wxT("delete.png"),wxBITMAP_TYPE_PNG),wxT("Delete Selected File"));
	this->mainToolBar->SetBackgroundColour(wxColour(0xCFCFCF));
	
	mainToolBar->Realize();
	
	wxStaticText* server = new wxStaticText(mainPanel,wxID_ANY,wxT("Server View"));
	

	serverPathText = new wxStaticText(mainPanel,wxID_ANY,wxT("--"));
	//localSystemList = new wxGenericDirCtrl(mainPanel,ID_TREE_VIEW_CLIENT,wxDirDialogDefaultFolderStr);



	serverListCtrl = new wxListCtrl(mainPanel,ID_TREE_VIEW_SERVER,wxDefaultPosition,wxSize(wxDefaultSize.x,250),wxLC_REPORT);
	serverListCtrl->InsertColumn(0,"Name",0,330);
	serverListCtrl->InsertColumn(1,"Type");
	serverListCtrl->InsertColumn(2,"Permissions");
	serverListCtrl->InsertColumn(3,"Time");
	serverListCtrl->InsertColumn(4,"Date");
	serverListCtrl->InsertColumn(5,"Size");
	serverListCtrl->InsertItem(0,"---");
	serverListCtrl->SetItem(0,1,"---");
	serverListCtrl->SetItem(0,2,"---");
	serverListCtrl->SetItem(0,3,"---");
	serverListCtrl->SetItem(0,4,"---");
	serverListCtrl->SetItem(0,5,"---");



	wxStaticBox* infoPanel = new wxStaticBox(mainPanel,ID_INFO_PANEL,wxT("File Status"),wxDefaultPosition,wxSize(wxDefaultSize.x,40));
	
	logBox = new wxTextCtrl(mainPanel,ID_LOG_BOX,"",wxDefaultPosition,wxSize(wxDefaultSize.x,40),wxTE_MULTILINE | wxTE_RICH2);
	logBox->SetEditable(false);
	logBox->SetBackgroundColour(wxColour(0xFFFFFF));
	
	this->Connect(ID_TOOL_CONNECT,wxEVT_COMMAND_TOOL_CLICKED,wxCommandEventHandler(CClientUI::showConnectDialog));
	this->Connect(ID_TOOL_DOWNLOAD,wxEVT_COMMAND_TOOL_CLICKED,wxCommandEventHandler(CClientUI::downloadFile));
	this->Connect(ID_TOOL_UPLOAD,wxEVT_COMMAND_TOOL_CLICKED,wxCommandEventHandler(CClientUI::uploadFile));
	this->Connect(ID_TOOL_NEWFOLDER,wxEVT_COMMAND_TOOL_CLICKED,wxCommandEventHandler(CClientUI::createFolder));
	this->Connect(ID_TOOL_DELETE,wxEVT_COMMAND_TOOL_CLICKED,wxCommandEventHandler(CClientUI::deleteFile));
	this->Connect(ID_TOOL_RENAME,wxEVT_COMMAND_TOOL_CLICKED,wxCommandEventHandler(CClientUI::renameFile));
	this->Connect(MENU_CONNECT_ID,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(CClientUI::showConnectDialog));
	this->Connect(MENU_QUIT_ID,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(CClientUI::exitApp));
	this->Connect(MENU_ABOUT_ID,wxEVT_COMMAND_MENU_SELECTED,wxCommandEventHandler(CClientUI::aboutApp));
	this->Connect(ID_TREE_VIEW_SERVER,wxEVT_COMMAND_LIST_ITEM_ACTIVATED,wxCommandEventHandler(CClientUI::itemSelctedList));
	this->Connect(ID_TREE_VIEW_SERVER,wxEVT_COMMAND_LIST_ITEM_SELECTED,wxCommandEventHandler(CClientUI::itemSelChangedList));
	this->Connect(ID_TREE_VIEW_SERVER,wxEVT_COMMAND_LIST_ITEM_DESELECTED,wxCommandEventHandler(CClientUI::itemUnselectedList));
	
	this->CreateStatusBar(2);
	this->SetStatusText("Not Connected to Server!",1);

	

	//sizers
	hBoxCVS_TEXT->Add(server,0,wxALIGN_LEFT,0);

	hBoxCVS_TEXT->Add(serverPathText,0,wxALIGN_LEFT | wxLEFT,10);
	hBoxCVS_TV->Add(serverListCtrl,1,wxEXPAND | wxALL,0);
	//hBoxCVC_TV->Add(localSystemList,1,wxEXPAND | wxALL,0);
	vBoxCVS->Add(hBoxCVS_TEXT,0,wxALIGN_LEFT,0);
	vBoxCVS->Add(hBoxCVS_TV,1,wxEXPAND | wxALL,0);
	//vBoxCVC->Add(hBoxCVC_TEXT,0,wxALIGN_LEFT,0);
	//vBoxCVC->Add(hBoxCVC_TV,1,wxEXPAND | wxALL,0);

	vBoxBottom->Add(infoPanel,1,wxEXPAND | wxALL , 10);
	vBoxBottom->Add(logBox,1,wxEXPAND | wxALL , 10);

	hBoxTop->Add(vBoxCVS,1, wxEXPAND | wxALIGN_LEFT | wxRIGHT | wxLEFT, 10);
	hBoxTop->Add(vBoxCVC,1, wxEXPAND | wxALIGN_LEFT | wxRIGHT | wxLEFT, 10);

	vBoxMain->Add(hBoxTop,1,wxEXPAND | wxTOP | wxRIGHT | wxLEFT , 10);
	vBoxMain->Add(vBoxBottom,1,wxEXPAND | wxALL , 10);

	mainPanel->SetSizer(vBoxMain);

	
	this->Show(true);
	Refresh();
	this->SetSize(this->GetSize());
	Center();
	this->Maximize();
	this->Restore();

	return true;

};


void CClientUI::showConnectDialog(wxCommandEvent& e)
{
	
	this->dialogConnect =new CConnectDialog(wxT("Connect to server!"),this,connectionInfoObj,this->ftpClient);
	if(this->connectionInfoObj->setConnect==true)
	{
		
		this->ftpClient->SetUser(this->connectionInfoObj->userid);
		if(this->connectionInfoObj->userid=="")
			this->ftpClient->SetUser("anonymous");
		this->ftpClient->SetPassword(this->connectionInfoObj->password);
		
		if(this->ftpClient->Connect(this->connectionInfoObj->hostid,21))
		{
			logMessage(wxT("Connected : "+ftpClient->GetLastResult()),2);
			this->SetStatusText("Conected to server",1);
			serverListCtrl->DeleteAllItems();
			this->connectionInfoObj->currentServerPath = "..";

			if( !this->changeToDir(".."))
			{
				logMessage("Error setting up",1);
			}
		}

		else
		{
			logMessage("Connection Error!\nServer Reply : "+ftpClient->GetLastResult(),1);
		}
		
	};
}


void CClientUI::logMessage(wxString str,int type)
{
	//text colors :   2= green(special) 1=red(error) 0=black(normal)
	if(type == 0)
		logBox->SetDefaultStyle(wxTextAttr(*wxBLACK));
	else if(type == 1)
		logBox->SetDefaultStyle(wxTextAttr(*wxRED));
	else if(type == 2)
		logBox->SetDefaultStyle(wxTextAttr(*wxGREEN));

	*logBox<<">> "+str+"\n";
	
};


void CClientUI::itemSelctedList(wxCommandEvent& e)
{
	long itemIndex = -1;

	 for (;;) {
    itemIndex = serverListCtrl->GetNextItem(itemIndex,
                                         wxLIST_NEXT_ALL,
                                         wxLIST_STATE_SELECTED);
 
    if (itemIndex == -1) break;
	
	if(serverListCtrl->GetItemText(itemIndex,0)!="..")
	this->selectedFileName.Add(serverListCtrl->GetItemText(itemIndex,0));

	//change to directory
	if(serverListCtrl->GetItemText(itemIndex,2).substr(0,1)=="d" ||serverListCtrl->GetItemText(itemIndex,2).substr(0,1)=="-" || serverListCtrl->GetItemText(itemIndex,0)=="..")
	{
	if(this->ftpClient->IsConnected())
	{
		
		if(serverListCtrl->GetItemText(itemIndex,2).substr(0,1)=="-")
		{
			wxTextFile* commands = new wxTextFile();
			
			commands->Create("tempFiles/commands.txt");
			commands->AddLine(this->connectionInfoObj->userid);
			commands->AddLine(this->connectionInfoObj->password);
			commands->AddLine("get");
			commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+serverListCtrl->GetItemText(itemIndex,0)+"\"");
			commands->AddLine("\""+wxGetCwd()+"\\tempFiles\\"+serverListCtrl->GetItemText(itemIndex,0)+"\"");
			commands->AddLine("quit");
			commands->Write();
			commands->Close();
			
			logMessage("File Download : "+serverListCtrl->GetItemText(itemIndex,0));

			if(system("ftp -s:"+wxGetCwd()+"\\tempFiles\\commands.txt localhost")==0)
			{
				if(wxFileExists(wxGetCwd()+"\\tempFiles\\"+serverListCtrl->GetItemText(itemIndex,0)))
					{
						wxString ext = serverListCtrl->GetItemText(itemIndex,0).substr(serverListCtrl->GetItemText(itemIndex,0).find("."),serverListCtrl->GetItemText(itemIndex,0).length()-1);
						if(ext==".txt" || ext==".htm" || ext==".html" || ext==".php")
						{
							wxExecute("notepad "+wxGetCwd()+"\\tempFiles\\"+serverListCtrl->GetItemText(itemIndex,0),wxEXEC_SYNC);
							commands->Create("tempFiles/commands.txt");
							commands->AddLine(this->connectionInfoObj->userid);
							commands->AddLine(this->connectionInfoObj->password);
							commands->AddLine("put");
							commands->AddLine("\""+wxGetCwd()+"\\tempFiles\\"+serverListCtrl->GetItemText(itemIndex,0)+"\"");
							commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+serverListCtrl->GetItemText(itemIndex,0)+"\"");
							commands->AddLine("quit");
							commands->Write();
							commands->Close();
							logMessage("File Upload : "+serverListCtrl->GetItemText(itemIndex,0));
							if(system("ftp -s:"+wxGetCwd()+"\\tempFiles\\commands.txt localhost")==0)
								logMessage("File updated Successfully!");
						}
						else
						if(ext ==".png" || ext == ".bmp" || ext == ".jpg" || ext == ".jpeg")
						{
							
							wxFileDialog *dig = new wxFileDialog(NULL,"Select program to open file with","C:\\");
							if(dig->ShowModal() == wxID_OK)
								wxExecute(dig->GetPath()+" "+wxGetCwd()+"\\tempFiles\\"+serverListCtrl->GetItemText(itemIndex,0));

							delete dig;
						}
						else
						{
							wxFileDialog *dig = new wxFileDialog(NULL,"Select program to open file with","C:\\");
							if(dig->ShowModal() == wxID_OK)
								wxExecute(dig->GetPath()+" "+wxGetCwd()+"\\tempFiles\\"+serverListCtrl->GetItemText(itemIndex,0));

							delete dig;
						}
						
					}

			}

			
		}

		if(serverListCtrl->GetItemText(itemIndex,0)=="..")
		{
			int posl;
			
			posl = connectionInfoObj->currentServerPath.Find('/',true);
			
			connectionInfoObj->currentServerPath = connectionInfoObj->currentServerPath.substr(0,posl);
			
		}		

		if(serverListCtrl->GetItemText(itemIndex,2).substr(0,1)=="d")
		{
			connectionInfoObj->currentServerPath=connectionInfoObj->currentServerPath+"/"+serverListCtrl->GetItemText(itemIndex,0);
		}

		
		
		if(this->changeToDir(connectionInfoObj->currentServerPath))
		{
			if(connectionInfoObj->currentServerPath.length()<size_t(45))
			{
				this->serverPathText->SetLabel(connectionInfoObj->currentServerPath);
				
			}
			else
			{
				this->serverPathText->SetLabel(connectionInfoObj->currentServerPath.substr(0,45)+"...");
			
			};
			this->serverPathText->SetToolTip(connectionInfoObj->currentServerPath);
			
		}
	}

	}

	 }
 
	
};

bool CClientUI::changeToDir(wxString str)
{

	serverListCtrl->DeleteAllItems();
	
		wxStringTokenizer *tokenString;
	int tokenCounter=0;
	char fileType='f';
	wxString date="",fname="";
	if(this->serverListCtrl!=NULL && ftpClient->IsConnected())
	{
		
		this->serverListCtrl->InsertItem(0,"..");
			
		if(!this->ftpClient->ChDir(str))
			return false;

		if(!this->ftpClient->GetDirList(serverList))
			return false;

		for(int i=0;i<(int)serverList.size();i++)
			{
				date="";
				fname="";
				int pos=-1;
				tokenString = new wxStringTokenizer(serverList[i]," ");
				serverListCtrl->InsertItem(i+1,"Server Item");
				while(tokenString->HasMoreTokens())
				{
					wxString str = tokenString->GetNextToken();
					if(tokenCounter==8)
					{
						pos = serverList[i].First(str);
						fname = serverList[i].substr(pos,serverList[i].length());
						
					}
					if(tokenCounter==7)
					{
						serverListCtrl->SetItem(i+1,3,str);
					}
					if(tokenCounter==5)
					{
						date+=str;
					}
					if(tokenCounter==4)
					{
						serverListCtrl->SetItem(i+1,5,str+" Bytes");
					}
					if(tokenCounter==6)
						serverListCtrl->SetItem(i+1,4,date+str);
					if(tokenCounter==0)
					{
						serverListCtrl->SetItem(i+1,2,str);
						if(serverListCtrl->GetItemText(i+1,2).substr(0,1)=="d")
						{
							fileType = 'd';
						}
						else
						{
							fileType = 'f';
						}
					}
					if(tokenCounter==1)
					{
						if(fileType == 'd')
						{
							serverListCtrl->SetItem(i+1,1,"Directory");
						}
						else
							if(fileType=='f')
							serverListCtrl->SetItem(i+1,1,"File");
					}
					tokenCounter+=1;
				}
				serverListCtrl->SetItem(i+1,0,fname);
				tokenCounter=0;
				delete tokenString;
			};
	}
	else
	{
		return false;
	};

	
	return true;

};



void CClientUI::itemSelChangedList(wxCommandEvent& e)
{
	long itemIndex = -1;
	
	selectedFileName.Clear();

    itemIndex = serverListCtrl->GetNextItem(itemIndex,
                                         wxLIST_NEXT_ALL,
                                         wxLIST_STATE_SELECTED);
 
	
		for(int i=0;i<serverListCtrl->GetSelectedItemCount();i++)
		{
			if(serverListCtrl->GetItemText(itemIndex+i,0)!="..")
			 this->selectedFileName.Add(serverListCtrl->GetItemText(itemIndex+i,0));
		}
	 
	
	 
	 
};

void CClientUI::itemUnselectedList(wxCommandEvent& e)
{

	this->selectedFileName.Clear();
	 
};


void CClientUI::downloadFile(wxCommandEvent& e)
{
	if(this->selectedFileName.GetCount()>0)
	{
	
		logMessage("File Download : ");
		for(int i=0;i<(int)selectedFileName.GetCount();i++)
			logMessage(selectedFileName[i]);

		if(downloadSelectedFile())
		{
			logMessage("File(s) Downloaded Successfully",2);
		}
		else
		{
			logMessage("Download Canceled!",1);
		}
		
	}
	else
	{
		logMessage("Please select an item first!",1);
	}

};


void CClientUI::uploadFile(wxCommandEvent& e)
{

	wxFileDialog* selFiles = new wxFileDialog(this,"Select files to upload",wxEmptyString,wxEmptyString,wxFileSelectorDefaultWildcardStr,wxFD_MULTIPLE);
	if(selFiles->ShowModal() == wxID_OK)
	{
		wxArrayString str;
		wxString path ;

		selFiles->GetFilenames(str);
		path = selFiles->GetPath().substr(0,selFiles->GetPath().find_last_of("\\"));
		if(str.GetCount()>0)
		{

			if(uploadNewFile(str,path))
			{
				logMessage("File(s) Uploaded Successfully!",2);
			}
		}
		else
		{
			logMessage("Please select an item first!",1);
		}

	}

};


bool CClientUI::downloadSelectedFile()
{

	wxDirDialog* ask = new wxDirDialog(this,"Save file(s) to");
	if(ask->ShowModal()==wxID_OK)
	{
		int ch=0;
		wxString path = ask->GetPath();
		for(int i=0;i<(int)selectedFileName.GetCount();i++)
			{
				if(wxFileExists(path+"\\"+selectedFileName[i]))
				{
					
					logMessage("one or more files already exist!",1);
					wxArrayString choice;
					choice.Add(wxT("Replace all"));
					choice.Add("Let me select");
					choice.Add("Cancel");
					

					wxSingleChoiceDialog *dlg = new wxSingleChoiceDialog(this,"One or more files already exist under selected directory.\n what do you want to do?","Error!",choice);
					dlg->SetSelection(0);
					if(dlg->ShowModal()==wxID_OK)
					{
						ch= dlg->GetSelection();
						break;
					}
				}
			}
		if(ch==0 || ch==1 )
		{
		wxTextFile* commands = new wxTextFile();
			
		commands->Create("tempFiles/commands.txt");
		commands->AddLine(this->connectionInfoObj->userid);
		commands->AddLine(this->connectionInfoObj->password);			

		
			if(ch==0)
			{
				for(int i=0;i<(int)selectedFileName.GetCount();i++)
			{
			commands->AddLine("get");
			commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+selectedFileName[i]+"\"");	
			commands->AddLine("\""+path+"\\"+selectedFileName[i]+"\"");
				}
			}
			else if(ch==1)
			{
				wxString replname="";
				for(int i=0;i<(int)selectedFileName.GetCount();i++)
				{
					if(wxFileExists(path+"\\"+selectedFileName[i]))
					{
						wxTextEntryDialog* dlgtxt = new wxTextEntryDialog(this,"Enter replacement name for the file(with extension) :\n"+selectedFileName[i]+"\n(Empty name will result in download cancelation!)","Rename",selectedFileName[i]);
						if(dlgtxt->ShowModal()==wxID_OK)
						{
							replname = dlgtxt->GetValue();
						}

					if(replname!="")
					{
						commands->AddLine("get");
						commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+selectedFileName[i]+"\"");	
						commands->AddLine(path+"\\"+replname);
					}
					else
					{
						logMessage("Download Canceled for file: "+selectedFileName[i],1);
					}
					}
					else
					{
						commands->AddLine("get");
						commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+selectedFileName[i]+"\"");	
						commands->AddLine("\""+path+"\\"+selectedFileName[i]+"\"");
					}

				
				}
			}
		
						
		commands->AddLine("quit");
		commands->Write();
		commands->Close();

		///download thru ftp.exe
		if(system("ftp -s:"+wxGetCwd()+"\\tempFiles\\commands.txt localhost")==0)
		{
			return true;
		}

		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}


bool CClientUI::uploadNewFile(wxArrayString str,wxString path)
{
	int ch=1;
	bool exists=false;
	logMessage("File Upload:");
	for(int i=0;i<(int)str.GetCount();i++)
	{
		logMessage(str[i]);
	}

	wxTextFile* commands = new wxTextFile();
			
		commands->Create("tempFiles/commands.txt");
		commands->AddLine(this->connectionInfoObj->userid);
		commands->AddLine(this->connectionInfoObj->password);	

	for(int i=1;i<serverListCtrl->GetItemCount();i++)
	{	
		for(int j=0;j<(int)str.GetCount();j++)
		if(serverListCtrl->GetItemText(i,0)==str[j])
		{
			logMessage("one or more files already exist!",1);
			exists = true;
			break;
		}
		if(exists==true)
			break;
	}

	if(exists)
	{
		
		wxArrayString choice;
		choice.Add(wxT("Replace all"));
		choice.Add("Let me select");
		choice.Add("Cancel");
					

		wxSingleChoiceDialog *dlg = new wxSingleChoiceDialog(this,"One or more files already exist under selected directory.\n what do you want to do?","Error!",choice);
		dlg->SetSelection(0);
		if(dlg->ShowModal()==wxID_OK)
		{
			ch= dlg->GetSelection();
			
		}


		
		if(ch==0)
		{
			for(int i=0;i<(int)str.GetCount();i++)
			{
			commands->AddLine("put");
			commands->AddLine("\""+path+"\\"+str[i]+"\"");
			commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+str[i]+"\"");
			}

		}
		else if(ch==1)
		{
			wxString replname="";

			for(int j=0;j<(int)str.GetCount();j++)
			{	
				for(int i=1;i<(int)serverListCtrl->GetItemCount();i++)
				{
				if(serverListCtrl->GetItemText(i,0)==str[j])
				{
					wxTextEntryDialog* dlgtxt = new wxTextEntryDialog(this,"Enter replacement name for the file(with extension) :\n"+str[j]+"\n(Empty name will result in upload cancelation!)","Rename",str[j]);
					if(dlgtxt->ShowModal()==wxID_OK)
					{
						replname = dlgtxt->GetValue();
					}
					
					if(replname!="")
					{
						commands->AddLine("put");
						commands->AddLine("\""+path+"\\"+str[j]+"\"");
						commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+replname+"\"");

					}
					else
					{
						logMessage("Upload Canceled for file : "+str[j],1);
					}
				}
				}
				commands->AddLine("put");
				commands->AddLine("\""+path+"\\"+str[j]+"\"");
				commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+str[j]+"\"");
			}

			
		}
		else
		{
			logMessage("Uploading aborted!",1);
			return false;
		}
		
	}
	else
	{
		for(int i=0;i<(int)str.GetCount();i++)
			{
			commands->AddLine("put");
			commands->AddLine("\""+path+"\\"+str[i]+"\"");
			commands->AddLine("\""+this->connectionInfoObj->currentServerPath+"/"+str[i]+"\"");
			}
	}

		commands->AddLine("quit");
		commands->Write();
		commands->Close();

	
		if(system("ftp -s:"+wxGetCwd()+"\\tempFiles\\commands.txt localhost")==0)
		{
			return true;
		}
		else
		{
			return false;
		}

	return true;
}


void CClientUI::createFolder(wxCommandEvent& e)
{
	bool exist = false;
	if(this->ftpClient->IsConnected())
	{
		wxTextEntryDialog* dlg = new wxTextEntryDialog(this,"Enter Folder Name","Create Folder","New Folder");
		if(dlg->ShowModal()==wxID_OK)
		{
			wxString dirname = dlg->GetValue();
			for(int i=0;i<this->serverListCtrl->GetItemCount();i++)
			{
				if(dirname==serverListCtrl->GetItemText(i,0))
				{
					exist = true;
					wxMessageBox("Directory with same name already exists! Please Try with different name");
					logMessage("Directory with same name already exists! Please Try with different name",1);
					break;
				}
			
			}
			if(!exist)
			{
				if(ftpClient->MkDir(dirname))
					logMessage("Directory Created : "+dirname,2);
			}
			

		}
		dlg->Destroy();
		delete dlg;
		changeToDir("..");
	}
	else
	{
		logMessage("Please connect to server first!",1);
	}
	//
};

void CClientUI::renameFile(wxCommandEvent& e)
{

	bool exist = false;
	if(this->ftpClient->IsConnected() && selectedFileName.GetCount()>0)
	{

		wxTextEntryDialog* dlg = new wxTextEntryDialog(this,"Enter New File Name","Rename File",selectedFileName[0]);
		if(dlg->ShowModal()==wxID_OK)
		{
			wxString fname = dlg->GetValue();
			for(int i=0;i<this->serverListCtrl->GetItemCount();i++)
			{
				if(fname==serverListCtrl->GetItemText(i,0))
				{
					exist = true;
					wxMessageBox("File with same name already exists! Please Try with different name");
					logMessage("File with same name already exists! Please Try with different name",1);
					break;
				}
			
			}
			if(!exist)
			{
				if(ftpClient->Rename(selectedFileName[0],fname))
					logMessage("File Renamed  : "+selectedFileName[0]+" -> "+fname);
			}
			

		}
		dlg->Destroy();
		delete dlg;
		changeToDir("..");
	}
	else
	{
		logMessage("Please connect to server first!",1);
	}
	//

}

void CClientUI::deleteFile(wxCommandEvent& e)
{
	int code;
	if(this->ftpClient->IsConnected())
	{
		if(selectedFileName.GetCount()>0)
		{
		 code = wxMessageBox("Do you really want to delete : "+selectedFileName[0],"Delete File",wxYES_NO | wxNO_DEFAULT);
		 if(code==wxYES)
		 {
			 ftpClient->RmFile(selectedFileName[0]);
			 logMessage("File Deleted : "+selectedFileName[0],1);
		 }
		}
		changeToDir("..");
	}
	else
	{
		logMessage("Please connect to server first!",1);
	}
	//

};


void CClientUI::aboutApp(wxCommandEvent& e)
{
	wxMessageBox("Web Directory Manager - Client (by Arun Sharma)");
}

void CClientUI::exitApp(wxCommandEvent& e)
{
	exit(0);
}