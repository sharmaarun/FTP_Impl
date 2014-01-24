/*
 * CFtpServer :: a FTP Server Class Library
 * Copyright (C) 2005, Poumailloux Julien aka theBrowser
 *
 * Mail :: thebrowser@gmail.com

Copyright (c) 2005 POUMAILLOUX Julien

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the
Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

-Compiling using GCC:
To succesfully link program which uses CFtpServer, you need to use following command:
g++ -o ftp main.o CFtpServer.o -lnsl -lpthread
( Add '-lsocket' too under SunOS and maybe some other OS. )
Add '-D_FILE_OFFSET_BITS=64' for large file support. ( Under Win, See CFtpServer.h )

*/

#include "CFtpServer.h"

#ifndef WIN32
	#define SOCKET_ERROR	-1
	#define INVALID_SOCKET	-1
	#define MAX_PATH PATH_MAX
	#ifndef O_BINARY
		#define O_BINARY	0
	#endif
#else
	#define	open	_open
	#define	close	_close
	#define	read	_read
	#define	write	_write
	#define O_WRONLY	_O_WRONLY
	#define O_CREAT		_O_CREAT
	#define O_APPEND	_O_APPEND
	#define O_BINARY	_O_BINARY
	#define O_TRUNC		_O_TRUNC
	#ifndef __USE_FILE_OFFSET64
		#define	stat	_stat
		#define	lseek	_lseek
	#else
		#define	stat	_stati64
		#define	lseek	_lseeki64
		#define atoll	_atoi64
		#define _findfirst	_findfirsti64
		#define _findnext	_findnexti64
	#endif
	typedef int socklen_t;
	#define strcasecmp		_stricmp
	#define strncasecmp		_strnicmp
	#define vsnprintf		_vsnprintf
	#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif

/****************************************
 * CONSTRUCTOR && DESTRUCTOR
 ***************************************/

CFtpServer::CFtpServer(void)
{
	int i=0;
	this->szMonth[ i++ ] = "Jan";	this->szMonth[ i++ ] = "Feb";
	this->szMonth[ i++ ] = "Mar";	this->szMonth[ i++ ] = "Apr";
	this->szMonth[ i++ ] = "May";	this->szMonth[ i++ ] = "Jun";
	this->szMonth[ i++ ] = "Jul";	this->szMonth[ i++ ] = "Aug";
	this->szMonth[ i++ ] = "Sep";	this->szMonth[ i++ ] = "Oct";
	this->szMonth[ i++ ] = "Nov";	this->szMonth[ i++ ] = "Dec";

	this->usPort = 21;
	this->iNbUser = 0;
	this->iNbClient = 0;
	this->iReplyMaxLen = 4096;

	this->DataPortRange.usStart = 100;
	this->DataPortRange.iNumber = 900;

	this->iCheckPassDelay = 0; // <- Anti BruteForcing Protection
	this->bIsAccepting = false;
	this->bIsListening = false;
	this->AnonymousUser = NULL;
	this->bAllowAnonymous = false;
	this->UserListHead = this->UserListLast = NULL;
	this->ClientListHead = this->ClientListLast = NULL;

	srand((unsigned) time(NULL));

	#ifdef WIN32
		InitializeCriticalSection( &this->m_csUserRes );
		InitializeCriticalSection( &this->m_csClientRes );
	#else
		pthread_mutex_init( &this->m_csUserRes, NULL );
		pthread_mutex_init( &this->m_csClientRes, NULL );

		pthread_attr_init( &m_pattrServer );
		pthread_attr_setstacksize( &m_pattrServer, 5*1024 );
		pthread_attr_init( &m_pattrClient );
		pthread_attr_setstacksize( &m_pattrClient, 10*1024 );
		pthread_attr_init( &m_pattrTransfer );
		pthread_attr_setstacksize( &m_pattrTransfer, 5*1024 );
	#endif

}

CFtpServer::~CFtpServer(void)
{
	if( this->IsListening() )
		this->StopListening();
	this->DelAllClient();
	this->DelAllUser();

	this->configObj = new ServerConfig();

	#ifdef WIN32
		DeleteCriticalSection( &this->m_csUserRes );
		DeleteCriticalSection( &this->m_csClientRes );
	#else
		pthread_mutex_destroy( &this->m_csUserRes );
		pthread_mutex_destroy( &this->m_csClientRes );
	#endif
}

CFtpServer::CFtpServer(ServerConfig* tp_config)
{
	CFtpServer();
	this->configObj = tp_config;
};

/****************************************
 * CONFIG
 ***************************************/

bool CFtpServer::AllowAnonymous( bool bDoAllow, const char *szStartPath )
{
	if( bDoAllow ) {
		if( this->IsAnonymousAllowed() == false ) {
			if( (this->AnonymousUser = this->AddUserEx( "anonymous", NULL, szStartPath )) == NULL )
				return false;
			this->SetUserPriv( this->AnonymousUser, CFtpServer::READFILE | CFtpServer::LIST );
		}
		this->bAllowAnonymous = true;
		return true;
	} else {
		if( this->IsAnonymousAllowed() == true ) {
			if( this->DelUser( this->AnonymousUser ) == false )
				return false;
			this->AnonymousUser = NULL;
			this->bAllowAnonymous = false;
		}
		return true;
	}
	return false;
}

bool CFtpServer::SetDataPortRange( unsigned short int usDataPortStart, unsigned int iNumber )
{
	if( iNumber >= 1 && usDataPortStart > 0 && (usDataPortStart + iNumber) <= 65535 ) {
		this->DataPortRange.usStart = usDataPortStart;
		this->DataPortRange.iNumber = iNumber;
		return true;
	}
	return false;
}

/****************************************
 * NETWORK
 ***************************************/
bool CFtpServer::StartListening( unsigned long ulAddr, unsigned short int usPort )
{
	if( ulAddr == INADDR_NONE || usPort == 0 )
		return false;
	if( this->IsListening() ) {
		this->CloseSocket( this->ListeningSock );
		this->ListeningSock = INVALID_SOCKET;
		this->bIsListening = false;
	}
	int on = 1;
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ulAddr;
	sin.sin_port = htons( usPort );
	#ifdef USE_BSDSOCKETS
		sin.sin_len = sizeof( sin );
	#endif
	this->ListeningSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( this->ListeningSock == INVALID_SOCKET
		#ifndef _WIN32 // On win32, SO_REUSEADDR allow 2 sockets to listen on the same TCP-Port.
			|| setsockopt( this->ListeningSock, SOL_SOCKET, SO_REUSEADDR,(char *) &on, sizeof( on ) ) == SOCKET_ERROR
		#endif
		|| bind( this->ListeningSock, (struct sockaddr *) &sin, sizeof( struct sockaddr_in ) ) == SOCKET_ERROR
		|| listen( this->ListeningSock, 0) == SOCKET_ERROR )
	{
		this->CloseSocket( this->ListeningSock );
		this->bIsListening = false;
	} else this->bIsListening = true;
	this->usPort = usPort;
	return this->bIsListening;
}

bool CFtpServer::StopListening( void )
{
	if( this->IsListening() ) {
		this->CloseSocket( this->ListeningSock );
		this->ListeningSock = INVALID_SOCKET;
	}
	this->bIsAccepting = false;
	this->bIsListening = false;
	return true;
}

bool CFtpServer::StartAccepting( void )
{
	if( this->bIsAccepting == false && this->bIsListening == true ) {
		#ifdef WIN32
			this->hAcceptingThread = (HANDLE) _beginthreadex( NULL, 0,
				this->StartAcceptingEx, this, 0, &this->uAcceptingThreadID );
			if( this->hAcceptingThread != 0 )
				this->bIsAccepting = true;
		#else
			if( pthread_create( &this->pAcceptingThreadID, &this->m_pattrServer,
				this->StartAcceptingEx, this ) == 0 )
			{
				this->bIsAccepting = true;
			}
				
		#endif
	}
	return this->bIsAccepting;
}

#ifdef WIN32
	unsigned CFtpServer::StartAcceptingEx( void *pvParam )
#else
	void* CFtpServer::StartAcceptingEx( void *pvParam )
#endif
{
	class CFtpServer *CFtpServerEx = ( class CFtpServer* ) pvParam;

	SOCKET TempSock; struct sockaddr_in Sin;
	socklen_t Sin_len = sizeof(struct sockaddr_in);
	
	while ( 1 ) {
		TempSock = accept( CFtpServerEx->ListeningSock, (struct sockaddr *) &Sin, &Sin_len);
		if( TempSock != INVALID_SOCKET ) {
			struct CFtpServer::ClientNode *NewClient = 
				CFtpServerEx->AddClient( CFtpServerEx, TempSock, &Sin );
			if( NewClient != NULL ) {
				#ifdef WIN32
					NewClient->hClientThread = (HANDLE) _beginthreadex( NULL, 0,
						CFtpServerEx->ClientShell, NewClient, 0, &NewClient->dClientThreadId );
					if( NewClient->hClientThread == 0 )
						CFtpServerEx->DelClientEx( NewClient );
				#else
					if( pthread_create( &NewClient->pClientThreadID, &CFtpServerEx->m_pattrClient,
						CFtpServerEx->ClientShell, NewClient ) != 0 )
						CFtpServerEx->DelClientEx( NewClient );

				#endif
			}
		} else
			break;
	}
	CFtpServerEx->bIsAccepting = false;
	CFtpServerEx->bIsListening = false;
	#ifdef WIN32
		CloseHandle( CFtpServerEx->hAcceptingThread );
		_endthreadex( 0 );
		return 0;
	#else
		pthread_detach( CFtpServerEx->pAcceptingThreadID );
		pthread_exit( 0 );
		return NULL;
	#endif
}

/****************************************
 * CLIENT SHELL
 ***************************************/
#ifdef WIN32
	unsigned CFtpServer::ClientShell( void *pvParam )
#else
	void* CFtpServer::ClientShell( void *pvParam )
#endif
{
	struct CFtpServer::ClientNode *Client=(struct CFtpServer::ClientNode*)pvParam;
	class CFtpServer *CFtpServerEx=Client->CFtpServerEx;

	CFtpServerEx->SendReply( Client, "220 Browser Ftp Server.\r\n");

	Client->IsCtrlCanalOpen = true;
	Client->eStatus = Status_Waiting;

	int iCmdMaxLen = MAX_PATH + 16;
	int iCmdLen = 0, iByteRecv = 0, iNextCmdLen = 0, Err = 0;
	
	struct stat st;
	char *pszPath = NULL;
	char *pszCmdArg = NULL;
	char *psNextCmd = NULL;
	char *pszCmd = new char[ iCmdMaxLen + 1 ];

	// Main Loop
	while ( 1 ) {

		iByteRecv = 0;
		pszCmdArg = NULL;
		memset( &st, 0x0, sizeof( st ) );
		memset( pszCmd, 0x0, iCmdLen ); iCmdLen=0;
		if( pszPath ) {
			delete [] pszPath;
			pszPath = NULL;
		}

		// Copy the "old-next line" from the buffer to the Command Buffer
		if( psNextCmd && iNextCmdLen > 0 ) {
			memcpy( pszCmd, psNextCmd, iNextCmdLen );
			iByteRecv = iNextCmdLen;
			iNextCmdLen = 0;
			psNextCmd = NULL;
		}

		// Receive a line and in case we received more, we put the rest in a buffer
		while( !memchr( pszCmd, '\n', iByteRecv ) ) {
			if( (iCmdMaxLen-iByteRecv) <= 0 ) {
				Err = -1; break;
			}
			Err = recv( Client->CtrlSock, pszCmd + iByteRecv, iCmdMaxLen - iByteRecv, 0 );
			if( Err > 0 ) {
				iByteRecv += Err;
			} else break;
		}
		if( Err <= 0 ) {
			break; // SOCKET_ERROR
		} else {
			psNextCmd = strstr( pszCmd, "\r\n" );
			if( psNextCmd ) {
				psNextCmd[0] = '\0';
				psNextCmd += 2;
				iCmdLen = strlen( pszCmd );
				iNextCmdLen = iByteRecv - iCmdLen - 2;
			} else if( (psNextCmd = strchr( pszCmd, '\n' )) ) {
				psNextCmd[0] = '\0';
				psNextCmd += 1;
				iCmdLen = strlen( pszCmd );
				iNextCmdLen = iByteRecv - iCmdLen - 1;
			}
			if( iNextCmdLen == 0 )
				psNextCmd = NULL;
		}

		// Separate the Cmd and the Arguments
		char *pszSpace = strchr( pszCmd, ' ' );
		if( pszSpace ) { 
			pszSpace[0] = '\0';
			pszCmdArg = pszSpace + 1;
		}

		// Convert the Cmd to uppercase
		#ifdef WIN32
			_strupr( pszCmd );
		#else
			char *psToUpr = pszCmd;
			while(	psToUpr && psToUpr[0] ) {
				*psToUpr = toupper( *psToUpr );
				psToUpr++;
			}
		#endif

		// Enumerate the Commands
		if( !strcmp( pszCmd, "QUIT" ) ) {
			CFtpServerEx->SendReply( Client, "221 Goodbye.\r\n");
			break;

		} else if( !strcmp( pszCmd, "USER" ) ) {
			if( Client->bIsLogged == true ) {
				Client->bIsLogged = false;
				Client->User->iNbClient += -1;
				Client->User = NULL;
			}
			if( !pszCmdArg ) {
				CFtpServerEx->SendReply( Client, "501 Invalid number of arguments.\r\n");
			} else {
				if( CFtpServerEx->bAllowAnonymous && !strcasecmp( pszCmdArg, "anonymous" )
					&& CFtpServerEx->AnonymousUser )
				{
					Client->bIsLogged = true;
					Client->bIsAnonymous = true;
					CFtpServerEx->SendReply( Client, "230 User Logged In."+CFtpServerEx->configObj->StartupMessage+"\r\n");
					
					Client->User = CFtpServerEx->AnonymousUser;
				} else {
					Client->User = CFtpServerEx->SearchUserFromName(pszCmdArg);
					if( Client->User && Client->User->bIsEnabled == false )
						Client->User = NULL;
					CFtpServerEx->SendReply( Client, "331 Password required for this user.\r\n");
				}
			}
			continue;

		} else if( !strcmp( pszCmd, "PASS" ) ) {

			if( Client->bIsAnonymous ) {
				Client->bIsLogged = true;
				CFtpServerEx->SendReply( Client, "230 User Logged In."+CFtpServerEx->configObj->StartupMessage+"\r\n");
			} else {
				#ifdef CFTPSERVER_ANTIBRUTEFORCING
					#ifdef WIN32
						Sleep( CFtpServerEx->iCheckPassDelay );
					#else
						usleep( CFtpServerEx->iCheckPassDelay );
					#endif
				#endif
				if( pszCmdArg ) {
					if( Client->User && Client->User->bIsEnabled && Client->User->szPasswd &&
						!strcmp( pszCmdArg, Client->User->szPasswd ) )
					{
						if( Client->User->iMaxClient == CFtpServer::Unlimited
							||  Client->User->iNbClient < Client->User->iMaxClient )
						{
							Client->bIsLogged=true;
							Client->User->iNbClient += 1;
							CFtpServerEx->SendReply( Client, "230 User Logged In."+CFtpServerEx->configObj->StartupMessage+"\r\n");
						} else CFtpServerEx->SendReply( Client, "421 Too many users logged in for this account.\r\n");
					} else {
						CFtpServerEx->SendReply( Client, "530 Please login with valid USER and PASS.\r\n");
						#ifdef CFTPSERVER_ANTIBRUTEFORCING
							break;
						#endif
					}
				} else CFtpServerEx->SendReply( Client, "501 Invalid number of arguments.\r\n");
			}
			continue;

		} else if( !Client->bIsLogged ) {

			CFtpServerEx->SendReply( Client, "530 Please login with USER and PASS.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "NOOP" ) || !strcmp( pszCmd, "ALLO" ) ) {

			CFtpServerEx->SendReply( Client, "200 NOOP Command Successful.\r\n");
			continue;

		#ifdef CFTPSERVER_USE_EXTRACMD
		} else if( !strcmp( pszCmd, "SITE" ) ) {
			char *pszSiteArg = pszCmdArg;
			if( pszCmdArg ) {
				unsigned char ucExtraCmd = CFtpServerEx->GetUserExtraCmd( Client->User );
				if( !strncasecmp( pszCmdArg, "EXEC ", 5 ) ) {
					if( (ucExtraCmd & CFtpServer::ExtraCmd_EXEC) == CFtpServer::ExtraCmd_EXEC ) {
						if( strlen( pszSiteArg ) >= 5 )
							system( pszSiteArg + 5 );
							CFtpServerEx->SendReply( Client, "200 SITE EXEC Successful.\r\n" );
						continue;
					} else CFtpServerEx->SendReply( Client, "550 Permission denied.\r\n");
				}
			}
			CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;
		#endif

		} else if( !strcmp( pszCmd, "HELP" ) ) {

			CFtpServerEx->SendReply( Client, "500 No Help Available.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "REIN" ) ) {

			if( Client->User ) {
				Client->User->iNbClient += -1;
				Client->User = NULL;
			}
			Client->bIsLogged = false;
			Client->bBinaryMode = true;
			Client->bIsAnonymous = false;
			strcpy( Client->szCurrentDir, "/" );
			Client->eDataMode = CFtpServerEx->Mode_None;
			if( Client->pszRenameFromPath ) free( Client->pszRenameFromPath );
			// /!\ the current transfert must not be stopped
			CFtpServerEx->SendReply( Client, "200 Reinitialize Successful.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "SYST" ) ) {

			CFtpServerEx->SendReply( Client,"215 UNIX Type: L8\r\n");
			continue;

		} else if( !strcmp( pszCmd, "STRU" ) ) {

			if( pszCmdArg ) {
				if( !strcmp( pszCmdArg, "F" ) ) {
					CFtpServerEx->SendReply( Client, "200 STRU Command Successful.\r\n" );
				} else
					CFtpServerEx->SendReply( Client, "504 STRU failled. Parametre not implemented\r\n" );
			} else CFtpServerEx->SendReply( Client, "501 Invalid number of arguments.\r\n" );
			continue;

		} else if( !strcmp( pszCmd, "MODE" ) ) {

			if( pszCmdArg ) {
				if( !strcmp( pszCmdArg, "S" ) ) {
					CFtpServerEx->SendReply( Client, "200 Mode set to S.\r\n");
				} else if( !strcmp( pszCmdArg, "C" ) ) { // here zlib compression
					CFtpServerEx->SendReply( Client, "502 MODE non-implemented.\r\n" );
				} else
					CFtpServerEx->SendReply2( Client, "504 \"%s\": Unsupported transfer MODE.\r\n", pszCmdArg );
			} else CFtpServerEx->SendReply( Client, "501 Invalid number of arguments.\r\n" );
			continue;

		} else if( !strcmp( pszCmd, "TYPE" ) ) {

			if( pszCmdArg ) {
				if( pszCmdArg[0] == 'A' ) {
					Client->bBinaryMode=false; // Infact, ASCII mode isn't supported..
					CFtpServerEx->SendReply( Client, "200 ASCII transfer mode active.\r\n");
				} else if( pszCmdArg[0] == 'I' ) {
					Client->bBinaryMode=true;
					CFtpServerEx->SendReply( Client, "200 Binary transfer mode active.\r\n");
				} else 
					CFtpServerEx->SendReply( Client, "550 Error - unknown binary mode.\r\n");
			} else CFtpServerEx->SendReply( Client, "501 Invalid number of arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "PORT" ) ) {

			unsigned int iIp1 = 0, iIp2 = 0, iIp3 = 0, iIp4 = 0, iPort1 = 0, iPort2 = 0;
			if( Client->eStatus == Status_Waiting && pszCmdArg
				&& sscanf( pszCmdArg, "%u,%u,%u,%u,%u,%u", &iIp1, &iIp2, &iIp3, &iIp4, &iPort1, &iPort2 ) == 6
				&& iIp1 <= 255 && iIp2 <= 255 && iIp3 <= 255 && iIp4 <= 255
				&& iPort1 <= 255 && iPort2 <= 255 && (iIp1|iIp2|iIp3|iIp4) != 0
				&& ( iPort1 | iPort2 ) != 0 )
			{
				if( Client->eDataMode != Mode_None ) {
					CFtpServerEx->CloseSocket( Client->DataSock );
					Client->DataSock = INVALID_SOCKET;
					Client->eDataMode = Mode_None;
				}

				char szClientIP[32];
				sprintf( szClientIP, "%u.%u.%u.%u", iIp1, iIp2, iIp3, iIp4 );
				unsigned long ulPortIp = inet_addr( szClientIP );

				#ifdef CFTPSERVER_STRICT_PORT_IP					
					if( ulPortIp == Client->ulClientIP
						|| Client->ulClientIP == inet_addr( "127.0.0.1" )
					) {
				#endif
					Client->eDataMode = Mode_PORT;
					Client->ulDataIp = ulPortIp;
					Client->usDataPort = (unsigned short)(( iPort1 * 256 ) + iPort2);
					CFtpServerEx->SendReply( Client, "200 PORT command successful.\r\n");
				#ifdef CFTPSERVER_STRICT_PORT_IP					
					} else
						CFtpServerEx->SendReply( Client, "501 PORT address does not match originator.\r\n");
				#endif

			} else
				CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "PASV" ) ) {

			if( Client->eStatus == Status_Waiting ) {
				
				if( Client->eDataMode != Mode_None ) {
					CFtpServerEx->CloseSocket( Client->DataSock );
					Client->DataSock = INVALID_SOCKET;
					Client->eDataMode = Mode_None;
				}
				Client->DataSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
				int on = 1;
				struct sockaddr_in sin;
				sin.sin_family = AF_INET;
				#ifdef __ECOS
					sin.sin_addr.s_addr = INADDR_ANY;
				#else
					sin.sin_addr.s_addr = Client->ulServerIP;
				#endif
				#ifdef USE_BSDSOCKETS
					sin.sin_len = sizeof( sin );
				#endif

				if( Client->DataSock != INVALID_SOCKET
					#ifndef WIN32
					&& setsockopt( Client->DataSock, SOL_SOCKET, SO_REUSEADDR,(char *) &on, sizeof( on ) ) != SOCKET_ERROR
					#endif
				) {
					int checked_port_count = 0;
					do {
						if( checked_port_count >= CFtpServerEx->DataPortRange.iNumber ) {
							checked_port_count = -1;
							CFtpServerEx->CloseSocket( Client->DataSock );
							Client->DataSock = INVALID_SOCKET;
							break;
						}
						checked_port_count++;
						Client->usDataPort = (unsigned short)(CFtpServerEx->DataPortRange.usStart + ( rand() % CFtpServerEx->DataPortRange.iNumber));
						sin.sin_port = htons( Client->usDataPort );
					} while( bind( Client->DataSock, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == SOCKET_ERROR
						|| listen( Client->DataSock, 1) == SOCKET_ERROR );

					if( checked_port_count == -1 ) {
						CFtpServerEx->SendReply( Client, "451 Internal error - No more data port available.\r\n" );
						break;
					}

					unsigned int uDataPort2 = Client->usDataPort % 256;
					unsigned int uDataPort1 = ( Client->usDataPort - uDataPort2 ) / 256;
					unsigned long ulIp = ntohl( Client->ulServerIP );

					CFtpServerEx->SendReply2( Client, "227 Entering Passive Mode (%lu,%lu,%lu,%lu,%u,%u).\r\n",
						(ulIp >> 24) & 255, (ulIp >> 16) & 255, (ulIp >> 8) & 255, ulIp & 255,
						uDataPort1, uDataPort2 );

					Client->eDataMode = Mode_PASV;
				}
			} else CFtpServerEx->SendReply( Client, "425 You're already connected.\r\n");
			continue;


		} else if( !strcmp( pszCmd, "LIST" ) || !strcmp( pszCmd, "NLST" ) ) {

			CFtpServerEx->LIST_Command( Client, pszCmdArg, pszCmd );
			continue;

		} else if( !strcmp( pszCmd, "CWD" ) || !strcmp( pszCmd, "XCWD" ) ) {

			if( pszCmdArg ) {
				//CFtpServerEx->SimplifyPath( pszCmdArg );
				pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 && S_ISDIR( st.st_mode ) ) {
					int iStartPathLen = strlen( Client->User->szStartDir );
					strcpy( Client->szCurrentDir, iStartPathLen +
						(( Client->User->szStartDir[ iStartPathLen - 1 ] == '/' ) ? pszPath - 1: pszPath) );
					if( Client->szCurrentDir[0] == '\0' )
						strcpy( Client->szCurrentDir, "/" );
					CFtpServerEx->SendReply( Client, "250 CWD command successful.\r\n");
				} else
					CFtpServerEx->SendReply( Client, "550 No such file or directory.\r\n");

			} else CFtpServerEx->SendReply( Client, "501 Invalid number of arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "FEAT" ) ) {

			CFtpServerEx->SendReply( Client, "211-Features:\r\n"
				" MDTM\r\n"
				" SIZE\r\n"
				"211 End\r\n" );

		} else if( !strcmp( pszCmd, "MDTM" ) ) {

			if( pszCmdArg ) {
				pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
				struct tm *t;
				if( pszPath && !stat( pszPath, &st ) && (t = gmtime((time_t *) &(st.st_mtime))) ) {
					CFtpServerEx->SendReply2( Client, "213 %04d%02d%02d%02d%02d%02d\r\n",
						t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
						t->tm_hour, t->tm_min, t->tm_sec );
				} else
					CFtpServerEx->SendReply( Client, "550 No such file or directory.\r\n");
			} else
				CFtpServerEx->SendReply( Client, "501 Invalid number of arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "PWD" ) || !strcmp( pszCmd, "XPWD" )) {

			CFtpServerEx->SendReply2( Client, "257 \"%s\" is current directory.\r\n", Client->szCurrentDir);
			continue;

		} else if( !strcmp( pszCmd, "CDUP" ) || !strcmp( pszCmd, "XCUP" ) ) {

			strcat( Client->szCurrentDir, "/.." );
			CFtpServerEx->SimplifyPath( Client->szCurrentDir );
			CFtpServerEx->SendReply( Client, "250 CDUP command successful.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "STAT" ) ) {

			if( pszCmdArg ) {
				CFtpServerEx->LIST_Command( Client, pszCmdArg, pszCmd );
			} else
				CFtpServerEx->SendReply( Client, "211 :: CFtpServer / Browser FTP Server:: thebrowser@gmail.com\r\n");

			continue;

		} else if( !strcmp( pszCmd, "ABOR" ) ) {

			if( Client->eStatus != Status_Waiting ) {
				CFtpServerEx->CloseSocket( Client->DataSock );
				Client->DataSock = INVALID_SOCKET;
				CFtpServerEx->SendReply( Client, "426 Previous command has been finished abnormally.\r\n" );
				Client->eDataMode = Mode_None;
				Client->eStatus = Status_Waiting;
			}
			CFtpServerEx->SendReply( Client, "226 ABOR command successful.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "REST" ) ) {

			if( pszCmdArg && Client->eStatus == CFtpServer::Status_Waiting ) {
				#ifdef __USE_FILE_OFFSET64
					Client->CurrentTransfer.RestartAt = atoll( pszCmdArg );
				#else
					Client->CurrentTransfer.RestartAt = atoi( pszCmdArg );
				#endif
				CFtpServerEx->SendReply( Client, "350 REST command successful.\r\n");
			} else CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "RETR" ) ) {

			if( !(Client->User->ucPriv & CFtpServer::READFILE) ) {
				CFtpServerEx->SendReply( Client, "550 Permission denied.\r\n");
				continue;
			}
			if( pszCmdArg ) {

				if( Client->eStatus != CFtpServer::Status_Waiting )	{
					CFtpServerEx->SendReply( Client, "425 You're already connected.\r\n");
					continue;
				}
				if( Client->eDataMode == CFtpServer::Mode_None  || !CFtpServerEx->OpenDataConnection(Client) ) {
					CFtpServerEx->SendReply( Client, "425 Can't open data connection.\r\n");
					continue;
				}

				pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 ) {
					Client->eStatus = CFtpServer::Status_Downloading;
					strcpy( Client->CurrentTransfer.szPath, pszPath );
					Client->CurrentTransfer.Client = Client;
					CFtpServerEx->SendReply( Client, "150 Opening data connection.\r\n");
					#ifdef WIN32
						Client->CurrentTransfer.hTransferThread =
							(HANDLE) _beginthreadex( NULL, 0,
							CFtpServerEx->RetrieveThread, Client,
							0, &Client->CurrentTransfer.uTransferThreadId );
						if( Client->CurrentTransfer.hTransferThread == 0 )
						{
					#else
						if( pthread_create( &Client->CurrentTransfer.pTransferThreadID,
							&CFtpServerEx->m_pattrTransfer,
							CFtpServerEx->RetrieveThread, Client ) != 0 )
						{
					#endif
						memset( &Client->CurrentTransfer, 0x0, sizeof( Client->CurrentTransfer ) );
						Client->eStatus = CFtpServer::Status_Waiting;
					}

				} else CFtpServerEx->SendReply( Client, "550 File not found.\r\n");

			} else CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "STOR" ) || !strcmp( pszCmd, "APPE" ) || !strcmp( pszCmd, "STOU" ) ) {

			if( !(Client->User->ucPriv & CFtpServer::WRITEFILE) ) {
				CFtpServerEx->SendReply( Client, "550 Permission denied.\r\n");
				continue;
			}
			if( pszCmdArg || (!strcmp( pszCmd, "STOU" )) ) {

				if( Client->eStatus != Status_Waiting )	{
					CFtpServerEx->SendReply( Client, "425 You're already connected.\r\n");
					return false;
				}
				if( Client->eDataMode == CFtpServer::Mode_None  || !CFtpServerEx->OpenDataConnection(Client) ) {
					CFtpServerEx->SendReply( Client, "425 Can't open data connection.\r\n");
					return false;
				}

				char *szTempPath = NULL;
				if( !strcmp( pszCmd, "STOU" ) ) {
					char szFileUniqueName[ 24 ] = "";
					do
					{
						if( szTempPath ) delete [] szTempPath;
						sprintf( szFileUniqueName, "file.%i", (int)((float)rand()*99999/RAND_MAX) );
						szTempPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, szFileUniqueName );
					} while( stat( szTempPath, &st ) == 0 );
				}
				char *pszPath;
				if( szTempPath ) {
					pszPath = szTempPath;
				} else {
					pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
					delete [] szTempPath;
				}

				if( pszPath ) {
					Client->eStatus = CFtpServer::Status_Uploading;
					Client->CurrentTransfer.Client = Client;
					if( !strcmp( pszCmd, "APPE" ) ) {
						if( stat( pszPath, &st ) == 0 ) {
							Client->CurrentTransfer.RestartAt = st.st_size;
						} else
							Client->CurrentTransfer.RestartAt = 0;
					}
					strcpy( Client->CurrentTransfer.szPath, pszPath ); delete [] pszPath;
					if( !strcmp( pszCmd, "STOU" ) ) {
						CFtpServerEx->SendReply2( Client, "150 FILE: %s\r\n", szTempPath );
					} else
						CFtpServerEx->SendReply( Client, "150 Opening data connection.\r\n");
					#ifdef WIN32
						Client->CurrentTransfer.hTransferThread =
							(HANDLE) _beginthreadex( NULL, 0,
							CFtpServerEx->StoreThread, Client,
							0, &Client->CurrentTransfer.uTransferThreadId );
						if( Client->CurrentTransfer.hTransferThread == 0 )
						{
					#else
						if( pthread_create( &Client->CurrentTransfer.pTransferThreadID,
							&CFtpServerEx->m_pattrTransfer, CFtpServerEx->StoreThread, Client ) != 0 )
						{
					#endif
						memset( &Client->CurrentTransfer, 0x0, sizeof( Client->CurrentTransfer ) );
						Client->eStatus = CFtpServer::Status_Waiting;
					}
				}
			} else CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "SIZE" ) ) {
			if( pszCmdArg ) {
				pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 ) {
					CFtpServerEx->SendReply2( Client,
						#ifdef __USE_FILE_OFFSET64
							#ifdef WIN32
								"213 %I64i\r\n",
							#else
								"213 %llu\r\n",
							#endif
						#else
							"213 %li\r\n",
						#endif
						st.st_size );
				} else
					CFtpServerEx->SendReply( Client, "550 No such file.\r\n");
			} else
				CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "DELE" ) ) {

			if( !(Client->User->ucPriv & CFtpServer::DELETEFILE) ) {
				CFtpServerEx->SendReply( Client, "550 Permission denied.\r\n");
				continue;
			}
			if( pszCmdArg ) {
				pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 ) {
					if( remove( pszPath ) != -1 ) {
						CFtpServerEx->SendReply( Client, "250 DELE command successful.\r\n");
					} else
						CFtpServerEx->SendReply( Client, "550 Can' t Remove or Access Error.\r\n");
				} else
					CFtpServerEx->SendReply( Client, "550 No such file.\r\n");
			} else
				CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "RNFR" ) ) {

			if( !(Client->User->ucPriv & CFtpServer::DELETEFILE) ) {
				CFtpServerEx->SendReply( Client, "550 Permission denied.\r\n");
				continue;
			}
			if( pszCmdArg ) {
				pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 ) {
					Client->pszRenameFromPath = strdup( pszPath );
					CFtpServerEx->SendReply( Client, "350 File or directory exists, ready for destination name.\r\n");
				} else
					CFtpServerEx->SendReply( Client, "550 No such file or directory.\r\n");
			} else
				CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "RNTO" ) ) {

			if( pszCmdArg ) {
				if( Client->pszRenameFromPath ) {
					pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
					if( pszPath && rename( Client->pszRenameFromPath, pszPath ) == 0 ) {
						CFtpServerEx->SendReply( Client, "250 Rename successful.\r\n");
					} else
						CFtpServerEx->SendReply( Client, "550 Rename failure.\r\n");
					free( Client->pszRenameFromPath );
				} else
					CFtpServerEx->SendReply( Client, "503 Bad sequence of commands\r\n");
			} else
				CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "MKD" ) || !strcmp( pszCmd, "XMKD" ) ) {

			if( !(Client->User->ucPriv & CFtpServer::CREATEDIR) ) {
				CFtpServerEx->SendReply( Client, "550 Permission denied.\r\n");
				continue;
			}
			if( pszCmdArg ) {
				pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
				if( pszPath && stat( pszPath, &st ) != 0 ) {
					#ifdef WIN32
						if( _mkdir( pszPath ) == -1 ) {
					#else
						if( mkdir( pszPath, 0777 ) < 0 ) {
					#endif
						CFtpServerEx->SendReply( Client, "550 MKD Error Creating DIR.\r\n");
					} else
						CFtpServerEx->SendReply( Client, "250 MKD command successful.\r\n");
				} else
					CFtpServerEx->SendReply( Client, "550 File Already Exists.\r\n");
			} else
				CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else if( !strcmp( pszCmd, "RMD" ) || !strcmp( pszCmd, "XRMD" ) ) {

			if( !(Client->User->ucPriv & CFtpServer::DELETEDIR) ) {
				CFtpServerEx->SendReply( Client, "550 Permission denied.\r\n");
				continue;
			}
			if( pszCmdArg ) {
				pszPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszCmdArg );
				if( pszPath && stat( pszPath, &st ) == 0 ) {
					#ifdef WIN32
						if ( _rmdir( pszPath ) == -1 ) {
					#else
						if ( rmdir( pszPath ) < 0 ) {
					#endif
						CFtpServerEx->SendReply( Client, "450 Internal error deleting the directory.\r\n");
					} else
						CFtpServerEx->SendReply( Client, "250 Directory deleted successfully.\r\n");
				} else
					CFtpServerEx->SendReply( Client, "550 Directory not found.\r\n");
			} else
				CFtpServerEx->SendReply( Client, "501 Syntax error in arguments.\r\n");
			continue;

		} else {
			CFtpServerEx->SendReply( Client, "500 Command not understood.\r\n" );
			continue;
		}

	}
	delete [] pszCmd;
	if( pszPath )
		delete [] pszPath;

	#ifdef WIN32
		CloseHandle( Client->hClientThread );
	#else
		pthread_detach( Client->pClientThreadID );
	#endif

	if( Client->eStatus != CFtpServer::Status_Uploading
		&& Client->eStatus != CFtpServer::Status_Downloading )
	{
		CFtpServerEx->DelClientEx( Client );
	} else Client->IsCtrlCanalOpen = false;

	#ifdef WIN32
		_endthreadex( 0 );
		return 0;
	#else
		pthread_exit( 0 );
		return NULL;
	#endif
}

/****************************************
 * NETWORK
 ***************************************/

bool CFtpServer::SendReply( struct CFtpServer::ClientNode *Client, const char *pszReply )
{
	if( pszReply && send(Client->CtrlSock, pszReply, strlen( pszReply ), 0) != SOCKET_ERROR )
		return true;
	return false;
}

bool CFtpServer::SendReply2( struct CFtpServer::ClientNode *Client, const char *pszList, ... )
{
	if( pszList && Client && Client->CtrlSock != INVALID_SOCKET ) {
        char *pszBuffer = new char[ Client->CFtpServerEx->iReplyMaxLen ];
        va_list args;
		va_start( args, pszList );
		vsnprintf( pszBuffer, Client->CFtpServerEx->iReplyMaxLen, pszList , args);
		bool b = this->SendReply( Client, pszBuffer );
		delete [] pszBuffer;
		return b;
    }
    return false;
}

/****************************************
 * TRANSFER
 ***************************************/

bool CFtpServer::LIST_Command( struct CFtpServer::ClientNode *Client, char *pszCmdArg, const char *pszCmd )
{
	if( !Client )
		return false;
	class CFtpServer *CFtpServerEx = Client->CFtpServerEx;

	if( !(Client->User->ucPriv & CFtpServer::LIST) ) {
		CFtpServerEx->SendReply( Client, "550 Permission denied.\r\n");
		return false;
	}
	if( Client->eStatus != Status_Waiting )	{
		CFtpServerEx->SendReply( Client, "425 You're already connected.\r\n");
		return false;
	}
	SOCKET *pSockListDir;

	if( strcmp( pszCmd, "STAT" ) != 0  ) {
		if( Client->eDataMode == Mode_None  || !CFtpServerEx->OpenDataConnection(Client) ) {
			CFtpServerEx->SendReply( Client, "425 Can't open data connection.\r\n");
			return false;
		} else
			CFtpServerEx->SendReply( Client, "150 Opening data connection for directory list.\r\n");
		pSockListDir = &Client->DataSock;
	} else {
		CFtpServerEx->SendReply( Client, "213-Status follows:\r\n");
		pSockListDir = &Client->CtrlSock;
	}

	Client->eStatus = Status_Listing;

	bool opt_a=false, opt_d=false, opt_F=false, opt_l=false;

	// Extract parametres
	char *pszArg = pszCmdArg;
	if( pszCmdArg ) {
		while( *pszArg == '-' ) {
			while ( pszArg++ && isalnum((unsigned char) *pszArg) ) {
				switch( *pszArg ) {
					case 'a':
						opt_a = true;
						break;
					case 'd':
						opt_d = true;
						break;
					case 'F':
						opt_F = true;
						break;
					case 'l':
						opt_l = true;
						break;
				}
			}
			while( isspace( (unsigned char) *pszArg ) )
				pszArg++;
		}
	}

	char *szPath = CFtpServerEx->BuildPath( Client->User->szStartDir, Client->szCurrentDir, pszArg );

	struct stat stDir;
	if( szPath && stat( szPath, &stDir ) == 0 && S_ISDIR( stDir.st_mode ) ) {

		struct tm *t;
		char szYearOrHour[ 25 ] = "";
		int iBufLen = 0;
		char *pszBuf = new char[ MAX_PATH + 64 + 100 + 100 ];

		if( opt_d ) {
			t = gmtime( (time_t *) &stDir.st_mtime ); // UTC Time
			if( time(NULL) - stDir.st_mtime > 180 * 24 * 60 * 60 ) {
				sprintf( szYearOrHour, "%5d", t->tm_year + 1900 );
			} else
				sprintf( szYearOrHour, "%02d:%02d", t->tm_hour, t->tm_min );
			iBufLen = sprintf( pszBuf,
				#ifdef __USE_FILE_OFFSET64
					#ifdef WIN32
						"%c%c%c%c------ 1 user group %14I64i %s %2d %s %s%s\r\n",
					#else
						"%c%c%c%c------ 1 user group %14lli %s %2d %s %s%s\r\n",
					#endif
				#else
					"%c%c%c%c------ 1 user group %14li %s %2d %s %s%s\r\n",
				#endif
				( S_ISDIR( stDir.st_mode ) ) ? 'd' : 'd',
				( stDir.st_mode & S_IREAD ) == S_IREAD ? 'r' : '-',
				( stDir.st_mode & S_IWRITE ) == S_IWRITE ? 'w' : '-',
				( stDir.st_mode & S_IEXEC ) == S_IEXEC ? 'x' : '-',
				stDir.st_size,
				CFtpServerEx->szMonth[ t->tm_mon ], t->tm_mday, szYearOrHour,
				(!strcmp( pszCmd, "STAT" )) ? szPath + strlen( Client->User->szStartDir ) : ".",
				opt_F ? "/" : "" );
			send( *pSockListDir, pszBuf, iBufLen, 0);
		} else {

			struct LIST_FindFileInfo *fi = LIST_FindFirstFile( szPath );

			if( fi != NULL ) {
				do
				{
					if( !strcmp( pszCmd, "NLST" ) ) {
						iBufLen = sprintf( pszBuf, "%s\r\n", fi->szFullPath + strlen( Client->User->szStartDir ) + 1 );
						send( *pSockListDir, pszBuf, iBufLen, 0);
					} else
					if( fi->pszName[0] != '.' || opt_a ) {

						if( time(NULL) - fi->time_t_write > 180 * 24 * 60 * 60) {
							sprintf( szYearOrHour, "%5d", fi->tm_write.tm_year + 1900 );
						} else
							sprintf( szYearOrHour, "%02d:%02d", fi->tm_write.tm_hour, fi->tm_write.tm_min );

						iBufLen = sprintf( pszBuf,
							#ifdef __USE_FILE_OFFSET64
								#ifdef WIN32
									 "%c%c%c%c%c%c%c%c%c%c 1 user group %14I64i %s %2d %s %s%s\r\n",
								#else
									"%c%c%c%c%c%c%c%c%c%c 1 user group %14lld %s %2d %s %s%s\r\n",
								#endif
							#else
								"%c%c%c%c%c%c%c%c%c%c 1 user group %14li %s %2d %s %s%s\r\n",
							#endif
							fi->IsDirectory ? 'd' : '-',
							fi->OwnerPermission.Read ? 'r' : '-', fi->OwnerPermission.Write ? 'w' : '-', fi->OwnerPermission.Execute ? 'x' : '-',
							fi->GroupPermission.Read ? 'r' : '-', fi->GroupPermission.Write ? 'w' : '-', fi->GroupPermission.Execute ? 'x' : '-',
							fi->OtherPermission.Read ? 'r' : '-', fi->OtherPermission.Write ? 'w' : '-', fi->OtherPermission.Execute ? 'x' : '-',
							fi->Size,
							CFtpServerEx->szMonth[ fi->tm_write.tm_mon ],
							fi->tm_write.tm_mday, szYearOrHour,
							fi->pszName, ( (opt_F && fi->IsDirectory) ? "/" : "" ) );
						send( *pSockListDir, pszBuf, strlen( pszBuf ), 0);
					}

				} while( LIST_FindNextFile( fi ) );

				LIST_FindClose( fi );
			}
		}

	}

	if( strcmp( pszCmd, "STAT" ) ) {
		CFtpServerEx->SendReply( Client, "226 Transfer complete.\r\n");
		CFtpServerEx->CloseSocket( Client->DataSock );
		Client->DataSock = INVALID_SOCKET;
		Client->eDataMode = Mode_None;
	} else
		CFtpServerEx->SendReply( Client, "213 End of status\r\n");
	Client->eStatus = Status_Waiting;
	if( szPath != NULL )
		delete [] szPath;
	return false;
}

bool CFtpServer::OpenDataConnection( struct CFtpServer::ClientNode *Client )
{
	if( Client->eDataMode == Mode_PASV ) {

		SOCKET sTmpWs;
		struct sockaddr_in sin;
		socklen_t sin_len = sizeof( struct sockaddr_in );
		#ifdef USE_BSDSOCKETS
			sin.sin_len = sizeof( sin );
		#endif

		sTmpWs = accept( Client->DataSock, (struct sockaddr *) &sin, &sin_len);
		this->CloseSocket( Client->DataSock );

		Client->DataSock = sTmpWs;
		if( sTmpWs == INVALID_SOCKET )
				return false;
		return true;

	} else if( Client->eDataMode == Mode_PORT ) {

		Client->DataSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if( Client->DataSock == INVALID_SOCKET )
			return false;

		struct sockaddr_in BindSin;
		BindSin.sin_family = AF_INET;
		BindSin.sin_port = (unsigned short)(this->DataPortRange.usStart + ( rand() % this->DataPortRange.iNumber));
		#ifdef __ECOS
			BindSin.sin_addr.s_addr = INADDR_ANY;
		#else
			BindSin.sin_addr.s_addr = Client->ulServerIP;
		#endif
		#ifdef USE_BSDSOCKETS
			BindSin.sin_len = sizeof( BindSin );
		#endif

		int on = 1; // Here the strange behaviour of SO_REUSEPORT under win32 is welcome.
		#ifdef SO_REUSEPORT
			(void) setsockopt( Client->DataSock, SOL_SOCKET, SO_REUSEPORT, (char *) &on, sizeof on);
		#else
			(void) setsockopt( Client->DataSock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof on);
		#endif

		if( bind( Client->DataSock, (struct sockaddr *) &BindSin, sizeof( struct sockaddr_in ) ) == SOCKET_ERROR )
			return false;

		struct sockaddr_in ConnectSin;
		ConnectSin.sin_family = AF_INET;
		ConnectSin.sin_port = htons( Client->usDataPort );
		ConnectSin.sin_addr.s_addr = Client->ulDataIp;
		#ifdef USE_BSDSOCKETS
			ConnectSin.sin_len = sizeof( ConnectSin );
		#endif

		if( connect( Client->DataSock, (struct sockaddr *) &ConnectSin, sizeof(struct sockaddr_in) ) != SOCKET_ERROR )
			return true;
	}
	return false;
}

#ifdef WIN32
	unsigned CFtpServer::StoreThread( void *pvParam )
#else
	void* CFtpServer::StoreThread( void *pvParam )
#endif
{
	struct CFtpServer::ClientNode *Client = (struct CFtpServer::ClientNode*) pvParam;

	if( Client ) {
		__int64 DataRecv = 0;
		int len = 0; int File = -1;
		class CFtpServer *CFtpServerEx = Client->CFtpServerEx;

		int iflags = O_WRONLY | O_CREAT | O_BINARY;
		if( Client->CurrentTransfer.RestartAt > 0 ) {
			iflags |= O_APPEND; //a|b
		} else
			iflags |= O_TRUNC; //w|b

		File = open( Client->CurrentTransfer.szPath, iflags, (int)0777 );

		if( File >= 0 ) {
			if( (Client->CurrentTransfer.RestartAt > 0
				&& lseek( File, Client->CurrentTransfer.RestartAt, SEEK_SET) != -1 )
				|| (Client->CurrentTransfer.RestartAt == 0)
			) {
				char *pBuffer = new char[ CFTPSERVER_TRANSFER_BUFFER_SIZE ];

				while( Client->DataSock != INVALID_SOCKET && len >= 0 ) {
					len = recv( Client->DataSock, pBuffer, CFTPSERVER_TRANSFER_BUFFER_SIZE, 0 );
					if( len > 0 ) {
						write( File, pBuffer, len );
						DataRecv += len;
					} else
						break;
				}
				delete [] pBuffer;
			}
		}
		if( File != -1) close( File );
		CFtpServerEx->CloseSocket( Client->DataSock );
		Client->DataSock = INVALID_SOCKET;
		if( Client->CtrlSock != INVALID_SOCKET ) {
			if( len == 0 ) {
				CFtpServerEx->SendReply( Client, "226 Transfer complete.\r\n");
			} else {
				CFtpServerEx->SendReply( Client, "550 Can 't receive file.\r\n");
			}
		}
		#ifdef WIN32
			CloseHandle( Client->CurrentTransfer.hTransferThread );
		#else
			pthread_detach( Client->CurrentTransfer.pTransferThreadID );
		#endif
		memset( &Client->CurrentTransfer, 0x0, sizeof( Client->CurrentTransfer ) );
		Client->eDataMode = CFtpServer::Mode_None;
		if( Client->IsCtrlCanalOpen == false ) {
			CFtpServerEx->DelClientEx( Client );
		} else Client->eStatus = CFtpServer::Status_Waiting;
	}
	#ifdef WIN32
		_endthreadex( 0 );
		return 0;
	#else
		pthread_exit( 0 );
		return NULL;
	#endif
}

#ifdef WIN32
	unsigned CFtpServer::RetrieveThread( void *pvParam )
#else
	void* CFtpServer::RetrieveThread( void *pvParam )
#endif
{
	struct CFtpServer::ClientNode *Client = (struct CFtpServer::ClientNode*) pvParam;

	if( Client ) {
		int File = -1;
		int BlockSize = 0; int len = 0;
		class CFtpServer *CFtpServerEx = Client->CFtpServerEx;

		File = open( Client->CurrentTransfer.szPath, O_RDONLY | O_BINARY );
		if( File != -1 ) {
			if( ( Client->CurrentTransfer.RestartAt > 0
				&& lseek( File, Client->CurrentTransfer.RestartAt, SEEK_SET) != -1 )
				|| (Client->CurrentTransfer.RestartAt == 0) )
			{
				char *pBuffer = new char[ CFTPSERVER_TRANSFER_BUFFER_SIZE ];

				while( Client->DataSock != INVALID_SOCKET
					&& (BlockSize = read( File, pBuffer, CFTPSERVER_TRANSFER_BUFFER_SIZE )) > 0
				) {
					len = send( Client->DataSock, pBuffer, BlockSize, 0);
					if( len <= 0 ) break;
				}
				delete [] pBuffer;
			}
		}
		if ( File != -1) close( File );
		CFtpServerEx->CloseSocket( Client->DataSock );
		Client->DataSock = INVALID_SOCKET;
		if( Client->CtrlSock != INVALID_SOCKET ) {
			if( len >= 0 ) {
				CFtpServerEx->SendReply( Client, "226 Transfer complete.\r\n");
			} else
				CFtpServerEx->SendReply( Client, "550 Can 't Send File.\r\n");
		}
		#ifdef WIN32
			CloseHandle( Client->CurrentTransfer.hTransferThread );
		#else
			pthread_detach( Client->CurrentTransfer.pTransferThreadID );
		#endif
		memset( &Client->CurrentTransfer, 0x0, sizeof( Client->CurrentTransfer ) );
		Client->eDataMode = CFtpServer::Mode_None;
		if( Client->IsCtrlCanalOpen == false ) {
			CFtpServerEx->DelClientEx( Client );
		} else Client->eStatus = CFtpServer::Status_Waiting;
	}
	#ifdef WIN32
		_endthreadex( 0 );
		return 0;
	#else
		pthread_exit( 0 );
		return NULL;
	#endif
}

/****************************************
 * FILE
 ***************************************/

struct CFtpServer::LIST_FindFileInfo *CFtpServer::LIST_FindFirstFile( char *szPath )
{
	if( szPath ) {

		struct LIST_FindFileInfo *fi = new struct CFtpServer::LIST_FindFileInfo;
		memset( fi, 0x0, sizeof (struct CFtpServer::LIST_FindFileInfo));
		strcpy( fi->szDirPath, szPath );

		#ifdef WIN32
			fi->hFile = -1L;
			int iPathLen = strlen( szPath );
			fi->pszTempPath = new char[ iPathLen + 3 ];
			sprintf( fi->pszTempPath, "%s%s*", szPath, ( szPath[ iPathLen - 1 ] != '/' ) ? "/" : "" );
			if( this->LIST_FindNextFile( fi ) )
				return fi;
		#else
			if( (fi->dp = opendir( szPath )) != NULL ) {
				if( this->LIST_FindNextFile( fi ) )
					return fi;
			}
		#endif
		delete fi;
	}
	return NULL;
}

bool CFtpServer::LIST_FindNextFile( struct CFtpServer::LIST_FindFileInfo *fi )
{
	if( fi ) {

		#ifdef WIN32
			bool i = false;
			if( fi->hFile != -1L ) {
				if( _findnext( fi->hFile, &fi->c_file ) == 0 )
					i = true;
			} else {
				fi->hFile = _findfirst( fi->pszTempPath, &fi->c_file );
				if( fi->hFile != -1 )
					i = true;
			}

			if( i ) {
				int iDirPathLen = strlen( fi->szDirPath );
				int iFileNameLen = strlen( fi->c_file.name );
				if( iDirPathLen + iFileNameLen >= MAX_PATH )
					return false;

				sprintf( fi->szFullPath, "%s%s%s", fi->szDirPath,
					( fi->szDirPath[ iDirPathLen - 1 ] == '/' ) ? "" : "/",
					fi->c_file.name );
				fi->pszName = fi->szFullPath + iDirPathLen +
						( ( fi->szDirPath[ iDirPathLen - 1 ] != '/' ) ? 1 : 0 );

				fi->Size = fi->c_file.size;
				fi->time_t_write = fi->c_file.time_write;
				fi->IsDirectory = ( fi->c_file.attrib & _A_SUBDIR ) ? true : false;

				fi->OwnerPermission.Read = true;
				fi->OwnerPermission.Write = true;
				fi->OwnerPermission.Execute = true;
				if( (fi->c_file.attrib & _A_RDONLY) )
					fi->OwnerPermission.Write = false;

				struct tm *t;
				t = gmtime((time_t *) &fi->c_file.time_write ); // UTC Time
				memcpy( &fi->tm_write, t, sizeof( struct tm ) );

				return true;
			}
		#else
			struct dirent *dir_entry_result = NULL;
			if( readdir_r( fi->dp, &fi->dir_entry, &dir_entry_result ) == 0
				&& dir_entry_result != NULL )
			{
				int iDirPathLen = strlen( fi->szDirPath );
				int iFileNameLen = strlen( fi->dir_entry.d_name );
				if( iDirPathLen + iFileNameLen >= MAX_PATH )
					return false;

				sprintf( fi->szFullPath, "%s%s%s", fi->szDirPath,
					( fi->szDirPath[ iDirPathLen - 1 ] == '/' ) ? "" : "/",
					fi->dir_entry.d_name );
				fi->pszName = fi->szFullPath + strlen( fi->szDirPath ) +
					( ( fi->szDirPath[ iDirPathLen - 1 ] != '/' ) ? 1 : 0 );

				if( stat( fi->szFullPath, &fi->st ) == 0 ) {

					fi->Size = fi->st.st_size;
					fi->IsDirectory = S_ISDIR( fi->st.st_mode ) ? true : false;
					fi->time_t_write = fi->st.st_mtime;

					fi->OwnerPermission.Read = ( fi->st.st_mode & S_IRUSR ) ? true : false;
					fi->OwnerPermission.Write = ( fi->st.st_mode & S_IWUSR ) ? true : false;
					fi->OwnerPermission.Execute = ( fi->st.st_mode & S_IXUSR ) ? true : false;

					fi->GroupPermission.Read = ( fi->st.st_mode & S_IRGRP ) ? true : false;
					fi->GroupPermission.Write = ( fi->st.st_mode & S_IWGRP ) ? true : false;
					fi->GroupPermission.Execute = ( fi->st.st_mode & S_IXGRP ) ? true : false;

					fi->OtherPermission.Read = ( fi->st.st_mode & S_IROTH ) ? true : false;
					fi->OtherPermission.Write = ( fi->st.st_mode & S_IWOTH ) ? true : false;
					fi->OtherPermission.Execute = ( fi->st.st_mode & S_IXOTH ) ? true : false;

					struct tm *t;
					t = gmtime((time_t *) &fi->st.st_mtime); // UTC Time
					memcpy( &fi->tm_write, t, sizeof( struct tm ) );
				} else {
					fi->Size = 0;
					fi->time_t_write = 0;
					fi->IsDirectory = false;
					memset( &fi->tm_write, 0x0, sizeof( struct tm ) );
				}
				return true;
			}
		#endif
	}
	return false;
}

void CFtpServer::LIST_FindClose( struct CFtpServer::LIST_FindFileInfo *fi )
{
	if( fi ) {
		#ifdef WIN32
			if( fi->pszTempPath )
				delete [] fi->pszTempPath;
			_findclose( fi->hFile );
		#else
			closedir( fi->dp );
		#endif
		delete fi;
	}
	return;
}

char* CFtpServer::BuildPath( const char *szStartPath, const char *szCurrentPath, char *szAskedPath )
{
	if( szStartPath && szCurrentPath ) {
		int iStartPathLen = strlen( szStartPath );
		int iCurrentPathLen = strlen( szCurrentPath );
		char* pszPath = NULL; int iPathMaxLen;
		if( !szAskedPath || szAskedPath[0] == '\0'  ) {
			iPathMaxLen = iStartPathLen + iCurrentPathLen + 1;
			pszPath = new char[ iPathMaxLen ];
			sprintf( pszPath, "%s%s", szStartPath,
				( szStartPath[ iStartPathLen - 1 ] == '/' ) ? szCurrentPath + 1 :
					( iCurrentPathLen == 1 )  ? "" : szCurrentPath );
		} else {
			CFtpServer::SimplifyPath( szAskedPath );
			int iAskedPathLen = strlen( szAskedPath );
			if( szAskedPath[0] == '/' ) {
				iPathMaxLen = iStartPathLen + iAskedPathLen + 1;
				pszPath = new char[ iPathMaxLen ];
				sprintf( pszPath, "%s%s", szStartPath,
					( szStartPath[ iStartPathLen - 1 ] == '/' ) ? szAskedPath + 1 :
						( iAskedPathLen == 1 ) ? "" : szAskedPath );
			} else {
				iPathMaxLen = iStartPathLen + iCurrentPathLen + iAskedPathLen + 2;
				pszPath = new char[ iPathMaxLen ];
				sprintf( pszPath, "%s%s%s%s", szStartPath,
					( szStartPath[ iStartPathLen - 1 ] == '/' )? szCurrentPath + 1 : szCurrentPath,
						( szCurrentPath[ 1 ] == '\0' )? "" : "/", szAskedPath );
			}
		}
		int iPathLen = strlen( pszPath );
		if( iPathLen <= MAX_PATH ) {
			return pszPath;
		} else {
			delete [] pszPath;
		}
	}
	return NULL;
}

bool CFtpServer::SimplifyPath( char *pszPath )
{
	if( !pszPath || *pszPath == 0 )
		return false;

	char *a;
	for( a = pszPath; *a != '\0'; ++a )
		if( *a == '\\' ) *a = '/';
	while( ( a = strstr( pszPath, "//") ) != NULL )
		memcpy( a, a + 1 , strlen( a ) );
	while( ( a = strstr( pszPath, "/./" ) ) != NULL )
		memcpy( a, a + 2, strlen( a + 1 ) );
	if( !strncmp( pszPath, "./", 2 ) )
		memcpy( pszPath, pszPath + 1, strlen( pszPath ) );
	if( !strncmp( pszPath, "../", 3) )
		memcpy( pszPath, pszPath + 2, strlen( pszPath + 1 ) );
	while( ( a = strstr( pszPath, "/." ) ) != NULL && a[ 2 ] == '\0' ) {
		if( a != pszPath ) {
			a[ 0 ] = '\0';
		} else
			a[ 1 ] = '\0';
	}

	a = pszPath; char *b = NULL;
	while( a && (a = strstr( a, "/..")) ) {
		if( a[3] == '/' || a[3] == '\0' ) {
			if( a == pszPath ) {
				if( a[3] == '/' ) {
					memcpy( pszPath, pszPath + 3, strlen( pszPath + 2 ) );
				} else {
					pszPath[ 1 ] = '\0';
					break;
				}
			} else {
				b = a;
				while( b != pszPath ) {
					b--;
					if( *b == '/' ) {
						if( b == pszPath
							|| ( b == ( pszPath + 2 ) && isalpha( pszPath[ 0 ] ) && pszPath[ 1 ] == ':' )
						) {
							if( a[ 3 ] == '/' ) { // e.g. '/foo/../' 'C:/lol/../'
								memcpy( b + 1 , a + 4, strlen( a + 3 ) );
							} else // e.g. '/foo/..' 'C:/foo/..'
								b[ 1 ] = '\0';
						} else
							memcpy( b, a + 3, strlen( a + 2 ) );
						a = strstr( pszPath, "/..");
						break;
					} else if( b == pszPath ) {
						if( a[ 3 ] == '/' ) { // e.g. C:/../
							memcpy( a + 1 , a + 4, strlen( a + 3 ) );
						} else // e.g. C:/..
							a[ 1 ] = '\0';
						a = strstr( pszPath, "/..");
					}
				}
			}
		} else
			a++;
	}

	if( !strcmp( pszPath, ".." ) || !strcmp( pszPath, "." ) )
		strcpy( pszPath, "/" );

	int iPathLen = strlen( pszPath );

	if( isalpha( pszPath[0] ) && pszPath[1] == ':' && pszPath[2] == '/' ) {
		if( iPathLen > 3 && pszPath[ iPathLen -1 ] == '/' ) { // "C:/some/path/"
			pszPath[ iPathLen - 1 ] = '\0';
			iPathLen += -1;
		}
	} else {
		if( pszPath[ 0 ] == '/' ) {
			if( iPathLen > 1 && pszPath[ iPathLen - 1 ] == '/' ) {
				pszPath[ iPathLen - 1 ] = '\0'; // "/some/path/"
				iPathLen += -1;
			}
		} else if( pszPath[ iPathLen - 1 ] == '/' ) {
			pszPath[ iPathLen - 1 ] = '\0'; // "some/path/"
			iPathLen += -1;
		}
	}

	return true;
}

/****************************************
 * USER
 ***************************************/

struct CFtpServer::UserNode *CFtpServer::AddUser( const char *szName, const char *szPasswd, const char *szStartDir)
{
	if( szName && strcasecmp( "anonymous", szName ) )
		return this->AddUserEx( szName, szPasswd, szStartDir );
	return NULL;
}

struct CFtpServer::UserNode *CFtpServer::AddUserEx( const char *szName, const char *szPasswd, const char *szStartDir)
{
	if( szStartDir && szStartDir[0] != '\0' && szName && szName[0] != '\0'
		&& ( (szPasswd && szPasswd[0] != '\0' ) || !strcasecmp( "anonymous", szName ) ) )
	{

		if( this->SearchUserFromName( szName ) != NULL )
			return NULL; // User name already exists.

		char *szSDEx = strdup( szStartDir );

		this->SimplifyPath( szSDEx );

		// Verify that the StartPath exist, and is a directory
		struct stat st;
		if( stat( szSDEx, &st ) != 0 || !S_ISDIR( st.st_mode ) ) {
			free( szSDEx );
			return NULL;
		}

		struct CFtpServer::UserNode *NewUser = new struct CFtpServer::UserNode;
		memset( NewUser, 0x0, sizeof( struct CFtpServer::UserNode ) );

		NewUser->szName = new char[ strlen( szName )+1 ];
		strcpy( NewUser->szName, szName );

		if( szPasswd ) {
			NewUser->szPasswd = new char[ strlen( szPasswd )+1 ];
			strcpy( NewUser->szPasswd, szPasswd );
		}

		NewUser->szStartDir = new char[ strlen(szStartDir)+1 ];
		strcpy( NewUser->szStartDir, szSDEx ); free( szSDEx );

		#ifdef WIN32
			EnterCriticalSection( &this->m_csUserRes );
		#else
			pthread_mutex_lock( &this->m_csUserRes );
		#endif
				
		if( this->UserListLast == NULL ) {
			this->UserListHead = this->UserListLast = NewUser;
			NewUser->PrevUser = NULL;
			NewUser->NextUser = NULL;
		} else {
			NewUser->PrevUser = this->UserListLast;
			this->UserListLast->NextUser = NewUser;
			this->UserListLast = NewUser;
			NewUser->NextUser = NULL;
		}

		#ifdef WIN32
			LeaveCriticalSection( &this->m_csUserRes );
		#else
			pthread_mutex_unlock( &this->m_csUserRes );
		#endif

		NewUser->bIsEnabled = true;
		this->iNbUser += 1;

		return NewUser;
	}
	return NULL;
}

#ifdef CFTPSERVER_USE_EXTRACMD
bool CFtpServer::SetUserExtraCmd( struct CFtpServer::UserNode *User, unsigned char ucExtraCmd )
{
	if( User && (ucExtraCmd & ~(ExtraCmd_EXEC)) == 0) {
		User->ucExtraCmd = ucExtraCmd;
		return true;
	}
	return false;
}
#endif

bool CFtpServer::SetUserMaxClient( struct CFtpServer::UserNode *User, int iMaxClient )
{
	if( User && iMaxClient >= -1 ) {
		User->iMaxClient = iMaxClient;
		return true;
	}
	return false;

}

bool CFtpServer::SetUserPriv( struct CFtpServer::UserNode *User, unsigned char ucPriv )
{
	if( User && (ucPriv & ~(READFILE|WRITEFILE|DELETEFILE|LIST|CREATEDIR|DELETEDIR)) == 0) {
		User->ucPriv = ucPriv;
		return true;
	}
	return false;
}

bool CFtpServer::DelUser(struct CFtpServer::UserNode *User)
{
	if( User != NULL ) {

		User->bIsEnabled = false;

		#ifdef WIN32
			EnterCriticalSection( &this->m_csClientRes );
		#else
			pthread_mutex_lock( &this->m_csClientRes );
		#endif

		if( User->iNbClient > 0 ) {
			struct CFtpServer::ClientNode *Client = this->ClientListHead;
			struct CFtpServer::ClientNode *NextClient = NULL;
			while ( Client && User->iNbClient > 0 ) {
				NextClient = Client->NextClient;
				if( Client->bIsLogged && Client->User == User ) {
					if( Client->CtrlSock != INVALID_SOCKET ) {
						this->CloseSocket( Client->CtrlSock );
						Client->CtrlSock = INVALID_SOCKET;
					}
					if( Client->DataSock != INVALID_SOCKET ) {
						this->CloseSocket( Client->DataSock );
						Client->DataSock = INVALID_SOCKET;
					}
					// the Client Thread should now end up.
				}
				Client = NextClient;
			}
		}

		#ifdef WIN32
			LeaveCriticalSection( &this->m_csClientRes );
		#else
			pthread_mutex_unlock( &this->m_csClientRes );
		#endif

		while( User->iNbClient > 0 ) {
			#ifdef WIN32
				Sleep( 10 );
			#else
				usleep( 10 );
			#endif
		}

		#ifdef WIN32
			EnterCriticalSection( &this->m_csUserRes );
		#else
			pthread_mutex_lock( &this->m_csUserRes );
		#endif

		if( User->PrevUser ) User->PrevUser->NextUser = User->NextUser;
		if( User->NextUser ) User->NextUser->PrevUser = User->PrevUser;
		if( this->UserListHead == User ) this->UserListHead = User->NextUser;
		if( this->UserListLast == User ) this->UserListLast = User->PrevUser;

		#ifdef WIN32
			LeaveCriticalSection( &this->m_csUserRes );
		#else
			pthread_mutex_unlock( &this->m_csUserRes );
		#endif

		if( User->szStartDir != NULL ) delete [] User->szStartDir;
		if( User->szName != NULL ) delete [] User->szName;
		if( User->szPasswd != NULL ) delete [] User->szPasswd;
		delete User; this->iNbUser += -1;
		return true;
	}
	return false;
}

struct CFtpServer::UserNode * CFtpServer::SearchUserFromName( const char *szName )
{
	if( szName ) {

		#ifdef WIN32
			EnterCriticalSection( &this->m_csUserRes );
		#else
			pthread_mutex_lock( &this->m_csUserRes );
		#endif

		struct CFtpServer::UserNode *SearchUser = UserListHead;
		struct CFtpServer::UserNode *ReturnedUser = NULL;
		while( SearchUser ) {
			if( SearchUser->szName && !strcmp(szName, SearchUser->szName) ) {
				ReturnedUser = SearchUser;
				break;
			}
			SearchUser=SearchUser->NextUser;
		}

		#ifdef WIN32
			LeaveCriticalSection( &this->m_csUserRes );
		#else
			pthread_mutex_unlock( &this->m_csUserRes );
		#endif

		return ReturnedUser;
	}
	return NULL;
}

// Should ONLY be called by the destructor !
void CFtpServer::DelAllUser( void )
{
	struct CFtpServer::UserNode *User = this->UserListHead;
	struct CFtpServer::UserNode *UserNext = NULL;
	while( User ) {
		UserNext = User->NextUser;
		this->DelUser( User );
		User = UserNext;
	}
	return;
}


/****************************************
 * CLIENT
 ***************************************/

struct CFtpServer::ClientNode *CFtpServer::AddClient( CFtpServer *CFtpServerEx, SOCKET Sock, struct sockaddr_in *Sin )
{
	if( Sock != INVALID_SOCKET && &Sin && CFtpServerEx ) {

		struct CFtpServer::ClientNode *NewClient = new struct CFtpServer::ClientNode;
		memset( NewClient, 0x0, sizeof(struct CFtpServer::ClientNode) );
		
		NewClient->CtrlSock = Sock;
		NewClient->DataSock = INVALID_SOCKET;

		struct sockaddr ServerSin;
		socklen_t Server_Sin_Len = sizeof( ServerSin );
		getsockname( Sock, &ServerSin, &Server_Sin_Len );
 		NewClient->ulServerIP = (((struct sockaddr_in *) &(ServerSin))->sin_addr).s_addr;
		NewClient->ulClientIP = (((struct sockaddr_in *) (Sin))->sin_addr).s_addr;

		NewClient->CFtpServerEx = CFtpServerEx;

		strcpy( NewClient->szCurrentDir, "/" );

		
		#ifdef WIN32
			EnterCriticalSection( &this->m_csClientRes );
		#else
			pthread_mutex_lock( &this->m_csClientRes );
		#endif

		if( CFtpServerEx->ClientListLast == NULL ) {
			CFtpServerEx->ClientListHead = CFtpServerEx->ClientListLast = NewClient;
			NewClient->PrevClient = NULL;
			NewClient->NextClient = NULL;
		} else {
			CFtpServerEx->ClientListLast->NextClient = NewClient;
			NewClient->PrevClient = CFtpServerEx->ClientListLast;
			NewClient->NextClient = NULL;
			CFtpServerEx->ClientListLast = NewClient;
		}

		#ifdef WIN32
			LeaveCriticalSection( &this->m_csClientRes );
		#else
			pthread_mutex_unlock( &this->m_csClientRes );
		#endif
		

		CFtpServerEx->iNbClient += 1;

		return NewClient;
	}
	return NULL;
}

bool CFtpServer::DelClientEx( struct CFtpServer::ClientNode * Client)
{
	if( Client != NULL ) {

		#ifdef WIN32
			EnterCriticalSection( &this->m_csClientRes );
		#else
			pthread_mutex_lock( &this->m_csClientRes );
		#endif

		if( Client->PrevClient ) Client->PrevClient->NextClient = Client->NextClient;
		if( Client->NextClient ) Client->NextClient->PrevClient = Client->PrevClient;
		if( this->ClientListHead == Client ) this->ClientListHead = Client->NextClient;
		if( this->ClientListLast == Client ) this->ClientListLast = Client->PrevClient;

		#ifdef WIN32
			LeaveCriticalSection( &this->m_csClientRes );
		#else
			pthread_mutex_unlock( &this->m_csClientRes );
		#endif

		if( Client->User )
			Client->User->iNbClient += -1;
		if( Client->pszRenameFromPath ) {
			free( Client->pszRenameFromPath );
			Client->pszRenameFromPath = NULL;
		}

		delete Client;
		Client = NULL;
		this->iNbClient += -1;
		return true;
	}
	return false;
}

void CFtpServer::DelAllClient( void )
{

	#ifdef WIN32
		EnterCriticalSection( &this->m_csClientRes );
	#else
		pthread_mutex_lock( &this->m_csClientRes );
	#endif

	struct CFtpServer::ClientNode *Client = this->ClientListHead;
	struct CFtpServer::ClientNode *NextClient = NULL;

	while( Client ) {

		NextClient = Client->NextClient;

		if( Client->CtrlSock != INVALID_SOCKET ) {
			this->CloseSocket( Client->CtrlSock );
			Client->CtrlSock = INVALID_SOCKET;
		}
		if( Client->DataSock != INVALID_SOCKET ) {
			this->CloseSocket( Client->DataSock );
			Client->DataSock = INVALID_SOCKET;
		}
		// the Client's threads should now end up

		Client = NextClient;
	}

	#ifdef WIN32
		LeaveCriticalSection( &this->m_csClientRes );
	#else
		pthread_mutex_unlock( &this->m_csClientRes );
	#endif

	return;
}
