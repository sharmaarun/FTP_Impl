#include "main.h"
#include "CClientUI.h"

IMPLEMENT_APP(wdmClient);

bool wdmClient::OnInit()
{

	CClientUI* mainClient = new CClientUI(wxT("WDM-Client"));
	if(!mainClient)
		return false;

	if(!mainClient->init())
		return false;

	return true;

};