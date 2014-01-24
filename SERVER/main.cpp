#include "main.h"
#include "CFtpWorks.h"

IMPLEMENT_APP(wdmServer);

#pragma warning(disable: 4996)

bool wdmServer::OnInit()
{

	CServerUI* mainUI = new CServerUI(wxT("WDM-SERVER"));
	CFtpWorks* ftpObject = new CFtpWorks();

	if(!mainUI->init())
		return false;


	return true;
};