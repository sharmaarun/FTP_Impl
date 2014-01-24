#ifndef ENUMS_H
#define ENUMS_H

#include "wx/wx.h"

enum
{
	ID_TOOL_HOST_ID = 100,
	ID_TOOL_USER_ID,
	ID_TOOL_PASSWORD,
	ID_TOOL_PORT,
	ID_TOOL_CONNECT,
	ID_TOOL_DOWNLOAD,
	ID_TOOL_UPLOAD,
	ID_TOOL_DELETE,
	ID_TOOL_NEWFOLDER,
	ID_TOOL_RENAME,
	MENU_CONNECT_ID,
	MENU_QUIT_ID,
	MENU_CUT_ID,
	MENU_COPY_ID,
	MENU_PASTE_ID,
	MENU_ABOUT_ID,


	ID_TREE_VIEW_CLIENT,
	ID_TREE_VIEW_SERVER,

	ID_INFO_PANEL,
	ID_LOG_BOX
};

struct SConnectionInfo
{
	wxString hostid,userid,password,port;
	bool setConnect ;
	wxString currentServerPath,currentClientPath;
};

#endif