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

// Compilation Options ! :
// 98% of the time, shouldn't be modified

// Uncomment the next line if you want to allow Clients to run extra commands
//#define CFTPSERVER_USE_EXTRACMD

// Uncomment the next line if you want to use bruteforcing protection
//#define CFTPSERVER_ANTIBRUTEFORCING

// Uncomment the next line if you want CFtpServer to check if the IP supplied with
// the PORT command and the Client IP are the same. Strict IP check may fail when
// the server is behind a router.
//#define CFTPSERVER_STRICT_PORT_IP

// Here is the switch to enable large file support under Win. ( gcc and MS.VC++ )
#ifdef WIN32
	#define __USE_FILE_OFFSET64
#endif

#define CFTPSERVER_TRANSFER_BUFFER_SIZE	(8 * 1024)



#include <ctime>
#include <cctype>
#include <cstdio>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include <sys/stat.h>
#include "Enums.h"

#ifndef WIN32
	#ifdef __ECOS
		#define USE_BSDSOCKETS
		#define SOCKET int
	#else
		typedef int SOCKET;
	#endif
	typedef long long __int64;
	#define MAX_PATH PATH_MAX
	#include <limits.h>
	#include <unistd.h>
	#include <dirent.h>
	#include <pthread.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/ip.h>
	#include <arpa/inet.h>
#else
	#pragma comment(lib,"WSOCK32.LIB")
	#define WIN32_LEAN_AND_MEAN
	#include <io.h>
	#include <direct.h>
	#include <winsock.h>
	#include <process.h>
#endif

#ifndef INADDR_NONE
	#define INADDR_NONE	((unsigned long int) 0xffffffff)
#endif
#ifndef INADDR_ANY
	#define INADDR_ANY	((unsigned long int) 0x00000000)
#endif

/***************************************
 * THE CLASS: CFtpServer
 ***************************************/
class CFtpServer
{

	/***************************************
	 * PUBLIC
	 ***************************************/
	public:

		/* Constructor */
		CFtpServer(void);
		/* Destructor */
		~CFtpServer(void);

		CFtpServer(ServerConfig* tpconfig);

		/****************************************
		 * USER
		 ****************************************/

		/* The Structure which will be allocated for each User. */
		struct UserNode
		{
			int iMaxClient;
			unsigned char ucPriv;
			#ifdef CFTPSERVER_USE_EXTRACMD
				unsigned char ucExtraCmd;
			#endif
			char *szName;
			char *szPasswd;
			bool bIsEnabled;
			char *szStartDir;
			int iNbClient;
			struct UserNode *PrevUser;
			struct UserNode *NextUser;
		};

		/* Enumerate the different Privilegies a User can get. */
		enum
		{
			READFILE	= 0x1,
			WRITEFILE	= 0x2,
			DELETEFILE	= 0x4,
			LIST		= 0x8,
			CREATEDIR	= 0x10,
			DELETEDIR	= 0x20,
		};

		/* Create a new User. Don't create a user named "anonymous", call AllowAnonymous();.
			Arguments:
				-the User Name.
				-the User Password.
				-the User Start directory.
			Returns:
				-on success: the adress of the New-User's CFtpServer::UserNode structure.
				-on error: NULL.
		*/
		struct CFtpServer::UserNode *AddUser( const char *szName, const char *szPasswd, const char* szStartDir );

		/*create config object*/

		 ServerConfig *configObj;



		/* Set the Privilegies of a User.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
				-the user's privilegies of the CFtpServer Enum of privilegies,
					separated by the | operator.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool SetUserPriv( struct CFtpServer::UserNode *User, unsigned char ucPriv );

		/* Delete a User, and by the way all the Clients connected to this User.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool DelUser( struct CFtpServer::UserNode * User );

		/* Get the Head of the User List.
			Arguments:
			Returns:
				-the Head of the User List.
		*/
		struct CFtpServer::UserNode *GetUserListHead( void ) {
			return this->UserListHead;
		}

		/* Get the Last User of the List.
			Arguments:
			Returns:
				-the Last User of the List.
		*/
		struct CFtpServer::UserNode *GetUserListLast( void ) {
			return this->UserListLast;
		}

		/* Get the Next User of the List.
			Arguments:
				-a pointer to a CFtpServer::UserNode structure.
			Returns:
				-the Next User of the list.
		*/
		struct CFtpServer::UserNode *GetNextUser( const struct CFtpServer::UserNode *User ) {
			if( User )
				return User->NextUser;
			return NULL;
		}

		/* Get the Previous User of the List.
			Arguments:
				-a pointer to a CFtpServer::UserNode structure.
			Returns:
				-the Previous User of the List.
		*/
		struct CFtpServer::UserNode *GetPreviousUser( const struct CFtpServer::UserNode *User ) {
			if( User )
				return User->PrevUser;
			return NULL;
		}

		enum {
			Error = -2,
			None = -1,
			Unlimited = 0
		};
		/* Set the maximum number of Clients which can be connected to the User at a time.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
				-the number of clients who will be able to be connected to the user at a time.
					CFtpServer::UserMaxClient.Unlimited: Unlimited.
					CFtpServer::UserMaxClient.None: None.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool SetUserMaxClient( struct CFtpServer::UserNode *User, int iMaxClient );

		/* Get the maximum number of Clients which can be connected to the User at a time.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: the number of clients which can be connected to the User at a time.
				-on error: -2.
		*/
		unsigned int GetUserMaxClient( const struct CFtpServer::UserNode *User ) {
			if( User )
				return User->iMaxClient;
			return 0;
		}

		/* Delete all the Users, and by the way all the Server's Clients connected to a User. */
		void DelAllUser( void );

		/* Get a User's privilegies 
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: the user's privilegies concatenated with the bitwise inclusive
					binary operator "|".
				-on error: 0.
		*/
		unsigned char GetUserPriv( const struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->ucPriv;
			} else return 0;
		}

		/* Get a pointer to a User's Name.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: a pointer to the User's Name.
				-on error: NULL.
		*/
		const char* GetUserLogin( const struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->szName;
			} else return NULL;
		}

		/* Get a pointer to a User's Password.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: a pointer to the User's Password.
				-on error: NULL.
		*/
		const char* GetUserPasswd( const struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->szPasswd;
			} else return NULL;
		}

		/* Get a pointer to a User's Start Directory.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: a pointer to the User's Start Directory.
				-on error: NULL.
		*/
		const char* GetUserStartDir( const struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->szStartDir;
			} else return NULL;
		}
		
		#ifdef CFTPSERVER_USE_EXTRACMD

			/* Enum the Extra Commands a User can got. */
			enum eFtpExtraCmd {
				ExtraCmd_EXEC	= 0x1,
			};

			/* Set the supported Extra-Commands of a User.
				Arguments:
					-a pointer to the CFtpServer::UserNode structure of the User.
					-the user's Extra-Commands concatenated with the bitwise inclusive
						binary operator "|".
				Returns:
					-on success: true.
					-on error: false.
			*/
			bool SetUserExtraCmd( struct CFtpServer::UserNode *User, unsigned char dExtraCmd );

			/* Get the supported Extra-Commands of a User.
				Arguments:
					-a pointer to the CFtpServer::UserNode structure of the User.
				Returns:
					-on succes: the user's Extra-Commands concatenated with the bitwise
						inclusive binary operator "|".
					-on error: 0.
			*/
			unsigned char GetUserExtraCmd( const struct CFtpServer::UserNode *User ) {
				if( User ) {
					return User->ucExtraCmd;
				} else return 0;
			}

		#endif

		/***************************************
		 * START / STOP
		 ***************************************/

		/* Ask the Server to Start Listening on the TCP-Port supplied by SetPort().
			Arguments:
				-the Network Adress CFtpServer will listen on.
					Example:	INADDR_ANY for all local interfaces.
								inet_addr( "127.0.0.1" ) for the TCP Loopback interface.
				-the TCP-Port on which CFtpServer will listen.
			Returns:
				-on success: true.
				-on error: false, the supplied Adress or TCP-Port may not be valid.
		*/
		bool StartListening( unsigned long ulAddr, unsigned short int usPort );

		/* Ask the Server to Stop Listening.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool StopListening( void );

		/* Check if the Server is currently Listening.
			Returns:
				-true: if the Server is currently listening.
				-false: if the Server isn't currently listening.
		*/
		bool IsListening( void ) {
			return this->bIsListening;
		}

		/* Ask the Server to Start Accepting Clients.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool StartAccepting( void );

		/* Check if the Server is currently Accpeting Clients.
			Returns:
				-true: if the Server is currently accepting clients.
				-false: if the Server isn't currently accepting clients.
		*/
		bool IsAccepting( void ) {
			return this->bIsAccepting;
		}

		/****************************************
		 * CONFIG
		 ****************************************/

		/* Get the TCP Port on which CFtpServer will listen for incoming clients.
			Arguments:
			Returns:
				-on success: the TCP-Port.
				-on error: 0.
		*/
		unsigned short GetListeningPort( void ) {
			return this->usPort;
		}

		/* Set the TCP Port Range CFtpServer can use to Send and Receive Files or Data.
			Arguments:
				-the First Port of the Range.
				-the Number of Ports, including the First previously given.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool SetDataPortRange( unsigned short int usDataPortStart, unsigned int iNumber );

		/* Get the TCP Port Range CFtpServer can use to Send and Receive Files or Data.
			Arguments:
				-a Pointer to the First Port.
				-a Pointer to the Number of Ports, including the First.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool GetDataPortRange( unsigned short int *usDataPortStart, int *iNumber ) {
			if( usDataPortStart && iNumber ) {
				*usDataPortStart = this->DataPortRange.usStart;
				*iNumber = this->DataPortRange.iNumber;
				return true;
			}
			return false;
		}

		/* Allow or disallow Anonymous users. Its privilegies will be set to
			CFtpServer::READFILE | CFtpServer::LIST.
			Arguments:
				-true if you want CFtpServer to accept anonymous clients, otherwise false.
				-the Anonymous User Start Directory.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool AllowAnonymous( bool bDoAllow, const char *szStartPath );

		/* Check if Anonymous Users are allowed.
			Returns:
				-true: if Anonymous Users are allowed.
				-false: if Anonymous Users aren't allowed.
		*/
		bool IsAnonymousAllowed( void ) {
			return this->bAllowAnonymous;
		}

		#ifdef CFTPSERVER_ANTIBRUTEFORCING

			/* Set the delay the Server will wait when checking for the Client's pass. */
			void SetCheckPassDelay( int iCheckPassDelay ) {
				this->iCheckPassDelay = iCheckPassDelay;
			}

			/* Get the delay the Server will wait when checking for the Client's pass.
				Returns:
					-the delay the Server will wait when checking for the Client's pass.
			*/
			int GetSetCheckPassDelay( void ) {
				return this->iCheckPassDelay;
			}

		#endif

		/****************************************
		 * STATISTICS
		 ****************************************/

		/* Get in real-time the number of Clients connected to the Server.
			Returns:
				-the current number of Clients connected to the Server.
		*/
		int GetNbClient( void ) {
			return this->iNbClient;
		}

		/* Get in real-time the number of existing Users.
			Returns:
				-the current number of existing Users.
		*/
		int GetNbUser( void ) {
			return this->iNbUser;
		}

		/* Get the number of Clients currently connected using the specified User.
			Arguments:
				-a pointer to the CFtpServer::UserNode structure of the User.
			Returns:
				-on success: the count of Clients using currently the User.
				-on error: -1.
		*/
		int GetNbClientUsingUser( struct CFtpServer::UserNode *User ) {
			if( User ) {
				return User->iNbClient;
			} else return -1;
		}

	/***************************************
	 * PRIVATE
	 ***************************************/
	private:

		/****************************************
		 * CLIENT
		 ****************************************/

		/* Enum the differents Modes of Connection for transfering Data. */
		enum CFtpDataMode {
			Mode_None,
			Mode_PASV,
			Mode_PORT
		};

		/* Enum the differents Status a Client can got. */
		enum CFtpClientStatus {
			Status_Waiting = 0,
			Status_Listing,
			Status_Uploading,
			Status_Downloading
		};

		/* The Structure which will be allocated for each Client. */
		struct ClientNode
		{

			/*****************************************
			 * TRANSFER
			 *****************************************/

			SOCKET DataSock;

			unsigned long ulServerIP;
			unsigned long ulClientIP;

			bool bBinaryMode;

			struct
			{
				#ifdef __USE_FILE_OFFSET64
					__int64 RestartAt;
				#else
					int RestartAt;
				#endif
				struct ClientNode *Client;
				char szPath[ MAX_PATH + 1 ];

				#ifdef WIN32
					HANDLE hTransferThread;
					unsigned uTransferThreadId;
				#else
					pthread_t pTransferThreadID;
				#endif
			} CurrentTransfer;

			/*****************************************
			 * USER
			 *****************************************/

			struct UserNode *User;

			/*****************************************
			 * SHELL
			 *****************************************/

			SOCKET CtrlSock;

			bool bIsLogged;
			bool bIsAnonymous;

			CFtpDataMode eDataMode;
			CFtpClientStatus eStatus;

			unsigned long ulDataIp;
			unsigned short usDataPort;
			
			char* pszRenameFromPath;

			char szCurrentDir[ MAX_PATH + 4 ];

			#ifdef WIN32
				HANDLE hClientThread;
				unsigned dClientThreadId;
			#else
				pthread_t pClientThreadID;
			#endif

			/******************************************
			 * LINKED LIST
			 ******************************************/

			struct ClientNode *PrevClient;
			struct ClientNode *NextClient;

			/******************************************
			 * OTHER
			 ******************************************/

			bool IsCtrlCanalOpen;
			class CFtpServer *CFtpServerEx;

		};

		/* Add a new Client.
			Returns:
				-on success: a pointer to the new Client's CFtpServer::ClientNode structure.
				-on error: NULL.
		*/
		struct CFtpServer::ClientNode *AddClient( CFtpServer *CFtpServerEx, SOCKET Sock, struct sockaddr_in *Sin );

		/* Delete a Client.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool DelClientEx( struct CFtpServer::ClientNode *Client );

		/* Delete all Clients. */
		void DelAllClient( void );

		/* Parse the Client's Command.
		/	Returns:
				-nothing important.
		*/
		#ifdef WIN32
			static unsigned __stdcall ClientShell( void *pvParam );
		#else
			static void* ClientShell( void *pvParam );
		#endif

		#ifdef WIN32
			CRITICAL_SECTION m_csClientRes;
		#else
			pthread_mutex_t m_csClientRes;
		#endif

		struct CFtpServer::ClientNode *ClientListHead;
		struct CFtpServer::ClientNode *ClientListLast;

		/*****************************************
		 * Network
		 *****************************************/

		SOCKET ListeningSock;

		struct
		{
			int iNumber;
			unsigned short int usStart;

		} DataPortRange;


		/* Send a reply to a Client.
			Returns:
				-on success: true.
				-on error: false; The Socket may be invalid;
					The Connection may have been interupted.
		*/
		bool SendReply( struct CFtpServer::ClientNode *Client, const char *pszReply );
		bool SendReply2( struct CFtpServer::ClientNode *Client, const char *pszList, ... );
		
		/* Close a Socket. */
		int CloseSocket( SOCKET Sock ) {
			#ifdef WIN32
				return closesocket( Sock );
			#else
				return close( Sock );
			#endif
		}

		/*****************************************
		 * FILE
		 *****************************************/

		char *szMonth[12];

		/* Simplify a Path. */
		bool SimplifyPath( char *pszPath );

		/* Build a Path using the Client Command and the Client's User Start-Path. */
		char* BuildPath( const char* szStartPath, const char* szCurrentPath, char* szAskedPath );

		/* Structure used by the layer of Abstraction in order to list a Directory
			on both Linux and Win32 OS.
		*/
		struct LIST_FindFileInfo
		{
			#ifdef WIN32
				long hFile;
				char* pszTempPath;
				#ifdef __USE_FILE_OFFSET64
					struct _finddatai64_t c_file;
				#else
					struct _finddata_t c_file;
				#endif
			#else
				DIR *dp;
				struct stat st;
				struct dirent dir_entry;
			#endif

			char *pszName;

			struct
			{
				bool Read;
				bool Write;
				bool Execute;
			} OwnerPermission, GroupPermission, OtherPermission;

			bool IsDirectory;

			struct tm tm_write;
			time_t time_t_write;
			char szDirPath[ MAX_PATH + 1 ];
			char szFullPath[ MAX_PATH + 1 ];
			#ifdef __USE_FILE_OFFSET64
				__int64 Size;
			#else
				long Size;
			#endif
		};

		/* Layer of Abstraction which list a Directory on both Linux and Win32 */
		struct LIST_FindFileInfo *LIST_FindFirstFile( char *szPath );
		void LIST_FindClose( struct LIST_FindFileInfo *fi );
		bool LIST_FindNextFile( struct LIST_FindFileInfo *fi );

		/* Process the LIST, NLST Commands.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool LIST_Command( struct CFtpServer::ClientNode *Client, char *pszCmdArg, const char *pszCmd );

		/* Process the STOR, STOU, APPE Commands.
			Returns:
				-on success: true.
				-on error: false.
		*/
		bool STOR_Command( struct CFtpServer::ClientNode *Client, char *pszCmdArg, const char *pszCmd );

		/*****************************************
		 * TRANSFER
		 *****************************************/

		/* Open the Data Canal in order to transmit data.
			Returns:
			-on success: true.
			-on error: false.
		*/
		bool OpenDataConnection(struct CFtpServer::ClientNode *Client);

		/* Open the Data Canal in order to transmit data */
		#ifdef WIN32
			static unsigned __stdcall RetrieveThread( void *pvParam );
			static unsigned __stdcall StoreThread( void *pvParam  );
		#else
			static void* RetrieveThread( void *pvParem);
			static void* StoreThread( void *pvParam );
		#endif

		/*****************************************
		 * USER
		 *****************************************/

		#ifdef WIN32
			CRITICAL_SECTION m_csUserRes;
		#else
			pthread_mutex_t m_csUserRes;
		#endif

		struct UserNode *AnonymousUser;

		struct UserNode *UserListHead;

		struct UserNode *UserListLast;

		struct UserNode *AddUserEx( const char *szName, const char *szPasswd, const char* szStartDir );

		struct UserNode * SearchUserFromName( const char *szName );

		/*****************************************
		 * CONFIG
		 *****************************************/

		int iReplyMaxLen;

		unsigned int iCheckPassDelay;

		unsigned short int usPort;

		bool bAllowAnonymous;

		int iNbClient;
		int iNbUser;

		bool bIsListening;
		bool bIsAccepting;

		#ifdef WIN32
			HANDLE hAcceptingThread;
			unsigned uAcceptingThreadID;
			static unsigned __stdcall CFtpServer::StartAcceptingEx( void *pvParam );
		#else
			pthread_t pAcceptingThreadID;
			static void* StartAcceptingEx( void *pvParam );
			pthread_attr_t m_pattrServer;
			pthread_attr_t m_pattrClient;
			pthread_attr_t m_pattrTransfer;
		#endif


};
