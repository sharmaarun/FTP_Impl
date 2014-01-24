#include "CFtpWorks.h"

CFtpWorks::CFtpWorks()
{
	this->getInfoDialog = NULL;
	
	WSADATA WSAData;
		if( WSAStartup( MAKEWORD(1, 0), &WSAData) != 0 ) {
			*this->pLogTextBox<<("-WSAStartup failure: WSAGetLastError=%d\r\n", WSAGetLastError() );
			
		}

		this->configObj = new ServerConfig();
		configObj->StartupMessage = "\nWelcome to WDM server!";

			this->serverMain = new CFtpServer();
			serverMain->configObj= this->configObj;

		if( serverMain->AllowAnonymous( true, "C:/anonymous" ) == false ) { // allow anonymous user
		*this->pLogTextBox<<("-Unable to create anonymous user; Make sure its StartPath exists!(C:/anonymous)\r\n");
		}

		mysqlpp::Connection conn(false);
        conn.connect("wdmdb", "localhost", "root", "");
        mysqlpp::Query query = conn.query();
 
     
 
        /* Now SELECT */
        query << "SELECT * FROM users";
        mysqlpp::StoreQueryResult ares = query.store();
      
		FtpUser = new CFtpServer::UserNode*[ares.num_rows()];
		for(int i=0;i<ares.num_rows();i++)
		{
			
			FtpUser[i] = serverMain->AddUser( ares[i]["username"],  ares[i]["password"],  ares[i]["path"]);
		serverMain->SetUserMaxClient( FtpUser[i], CFtpServer::Unlimited );

		serverMain->SetUserPriv( FtpUser[i], CFtpServer::READFILE |
			CFtpServer::WRITEFILE |	CFtpServer::LIST | CFtpServer::DELETEFILE |
			CFtpServer::CREATEDIR | CFtpServer::DELETEDIR );
		serverMain->SetDataPortRange(3000,4000);
		}
	
};


bool CFtpWorks::startStopServer()
{

	if(this->serverMain->IsListening()==false)
	{
	
		if(serverMain->StartListening(INADDR_ANY,21))
		{

			*this->pLogTextBox<<"\nListening to users for connection!";
			if(serverMain->StartAccepting())
			{
				*this->pLogTextBox<<"\nNow accepting connections!";
			}

			*this->pLogTextBox<<"\nServer Started Successfully!";
		};
	
	}
	else
	{
		*this->pLogTextBox<<"\nStopping Server!";
		this->serverMain->StopListening();
	}

	

	return true;
};


void CFtpWorks::setLogTextBox(wxTextCtrl* txtCtrl)
{
	this->pLogTextBox = txtCtrl;
};

CFtpServer* CFtpWorks::getFtpServer()
{
	return this->serverMain;
};