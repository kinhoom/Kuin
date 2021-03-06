// LibNet.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include <WS2tcpip.h>
#include <WinInet.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Wininet.lib")

#define TCP_DATA_SIZE (1024 * 1024)
#define HTTP_DATA_SIZE (1024)

typedef struct SServerConnectList
{
	struct SServerConnectList* Next;
	SOCKET Socket;
} SServerConnectList;

typedef struct STcpServer
{
	SClass Class;
	SOCKET* Socket;
	Bool ThreadExit;
	SServerConnectList* ConnectTop;
	SServerConnectList* ConnectBottom;
	CRITICAL_SECTION* Mutex;
	HANDLE ThreadHandle;
} STcpServer;

typedef struct STcp
{
	SClass Class;
	SOCKET* Socket;
	Bool ThreadExit;
	Bool DataFull;
	U8* Data;
	int DataTop;
	int DataBottom;
	CRITICAL_SECTION* Mutex;
	HANDLE ThreadHandle;
} STcp;

typedef struct SHttpList
{
	struct SHttpList* Next;
	size_t Size;
	U8 Data[HTTP_DATA_SIZE];
} SHttpList;

typedef struct SHttp
{
	SClass Class;
	HINTERNET HandleOpen;
	HINTERNET HandleConnect;
	HINTERNET HandleRequest;
	Bool ThreadExit;
	Bool Success;
	size_t TotalSize;
	SHttpList* DataTop;
	SHttpList* DataBottom;
	CRITICAL_SECTION* Mutex;
	HANDLE ThreadHandle;
} SHttp;

static WSADATA* WsaData = NULL;

static DWORD WINAPI ServerThread(LPVOID param);
static DWORD WINAPI ConnectThread(LPVOID param);
static DWORD WINAPI HttpThread(LPVOID param);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name)
{
	if (Heap != NULL)
		return;
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	AppName = app_name == NULL ? L"Untitled" : (Char*)(app_name + 0x10);
	Instance = (HINSTANCE)GetModuleHandle(NULL);

	WsaData = (WSADATA*)AllocMem(sizeof(WSADATA));
	if (WSAStartup(MAKEWORD(2, 2), WsaData) != 0)
	{
		FreeMem(WsaData);
		WsaData = NULL;
	}
}

EXPORT void _fin(void)
{
	if (WsaData != NULL)
	{
		WSACleanup();
		FreeMem(WsaData);
		WsaData = NULL;
	}
}

EXPORT SClass* _makeTcpServer(SClass* me_, S64 port)
{
	THROWDBG(port < 0 || 49152 < port, 0xe9170006);
	STcpServer* me2 = (STcpServer*)me_;
	SOCKET socket2 = socket(AF_INET, SOCK_STREAM, 0);
	if (socket2 == INVALID_SOCKET)
		return NULL;
	{
		SOCKADDR_IN addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons((u_short)(U64)port);
		addr.sin_addr.S_un.S_addr = INADDR_ANY;
		BOOL yes = 1;
		setsockopt(socket2, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(BOOL));
		if (bind(socket2, (SOCKADDR*)&addr, sizeof(addr)) != 0)
			return NULL;
		if (listen(socket2, SOMAXCONN) != 0)
			return NULL;
	}

	me2->Socket = (SOCKET*)AllocMem(sizeof(SOCKET));
	*me2->Socket = socket2;
	me2->Mutex = (CRITICAL_SECTION*)AllocMem(sizeof(CRITICAL_SECTION));

	me2->ThreadExit = False;
	me2->ConnectTop = NULL;
	me2->ConnectBottom = NULL;
	InitializeCriticalSection(me2->Mutex);
	u_long val = 1;
	ioctlsocket(*me2->Socket, FIONBIO, &val);
	me2->ThreadHandle = CreateThread(NULL, 0, ServerThread, me2, 0, NULL);
	return me_;
}

EXPORT void _tcpServerDtor(SClass* me_)
{
	_tcpServerFin(me_);
}

EXPORT void _tcpServerFin(SClass* me_)
{
	STcpServer* me2 = (STcpServer*)me_;
	if (me2->Mutex == NULL)
		return;
	EnterCriticalSection(me2->Mutex);
	me2->ThreadExit = True;
	LeaveCriticalSection(me2->Mutex);
	WaitForSingleObject(me2->ThreadHandle, INFINITE);
	CloseHandle(me2->ThreadHandle);
	me2->ThreadHandle = NULL;
	DeleteCriticalSection(me2->Mutex);
	FreeMem(me2->Mutex);
	me2->Mutex = NULL;

	shutdown(*me2->Socket, SD_BOTH);
	closesocket(*me2->Socket);
	FreeMem(me2->Socket);
	me2->Socket = NULL;

	while (me2->ConnectTop != NULL)
	{
		SServerConnectList* connect = me2->ConnectTop;
		me2->ConnectTop = me2->ConnectTop->Next;
		FreeMem(connect);
	}
	me2->ConnectTop = NULL;
	me2->ConnectBottom = NULL;
}

EXPORT SClass* _tcpServerGet(SClass* me_, SClass* me2)
{
	STcpServer* me3 = (STcpServer*)me_;
	STcp* me4 = (STcp*)me2;
	if (me3->ThreadExit)
		return NULL;

	EnterCriticalSection(me3->Mutex);
	if (me3->ConnectTop == NULL)
	{
		LeaveCriticalSection(me3->Mutex);
		return NULL;
	}
	SOCKET socket2 = me3->ConnectTop->Socket;
	SServerConnectList* connect = me3->ConnectTop;
	me3->ConnectTop = me3->ConnectTop->Next;
	if (me3->ConnectTop == NULL)
		me3->ConnectBottom = NULL;
	FreeMem(connect);
	LeaveCriticalSection(me3->Mutex);

	me4->Socket = (SOCKET*)AllocMem(sizeof(SOCKET));
	*me4->Socket = socket2;
	me4->Mutex = (CRITICAL_SECTION*)AllocMem(sizeof(CRITICAL_SECTION));

	me4->ThreadExit = False;
	me4->DataFull = False;
	me4->Data = (U8*)AllocMem(TCP_DATA_SIZE);
	me4->DataTop = 0;
	me4->DataBottom = 0;
	InitializeCriticalSection(me4->Mutex);
	u_long val = 1;
	ioctlsocket(*me4->Socket, FIONBIO, &val);
	me4->ThreadHandle = CreateThread(NULL, 0, ConnectThread, me4, 0, NULL);
	return me2;
}

EXPORT SClass* _makeTcpClient(SClass* me_, const U8* host, S64 port)
{
	THROWDBG(host == NULL, 0xc0000005);
	THROWDBG(port < 0 || 65535 < port, 0xe9170006);
	STcp* me2 = (STcp*)me_;

	char host_name[KUIN_MAX_PATH + 1];
	{
		const Char* host2 = (const Char*)(host + 0x10);
		S64 len = *(S64*)(host + 0x08);
		if (len > KUIN_MAX_PATH)
			THROWDBG(True, 0xe9170006);
		S64 i;
		for (i = 0; i < len; i++)
		{
			if (host2[i] > 0xff)
			{
				THROWDBG(True, 0xe9170006);
				return NULL;
			}
			host_name[i] = (char)host2[i];
		}
		host_name[len] = L'\0';
	}
	char port_name[33];
	sprintf(port_name, "%I64d", port);

	SOCKET socket2 = INVALID_SOCKET;
	{
		ADDRINFO hints;
		ADDRINFO* res;
		memset(&hints, 0, sizeof(hints));
		if (getaddrinfo(host_name, port_name, &hints, &res) != 0)
			return NULL;
		ADDRINFO* res2 = res;
		while (res2 != NULL)
		{
			socket2 = socket(res2->ai_family, res2->ai_socktype, res2->ai_protocol);
			if (socket2 != INVALID_SOCKET)
			{
				if (connect(socket2, res2->ai_addr, (int)res2->ai_addrlen) == 0)
					break;
				else
				{
					socket2 = INVALID_SOCKET;
					closesocket(socket2);
				}
			}
			res2 = res2->ai_next;
		}
		freeaddrinfo(res);
		if (socket2 == INVALID_SOCKET)
			return NULL;
	}

	me2->Socket = (SOCKET*)AllocMem(sizeof(SOCKET));
	*me2->Socket = socket2;
	me2->Mutex = (CRITICAL_SECTION*)AllocMem(sizeof(CRITICAL_SECTION));

	me2->ThreadExit = False;
	me2->DataFull = False;
	me2->Data = (U8*)AllocMem(TCP_DATA_SIZE);
	me2->DataTop = 0;
	me2->DataBottom = 0;
	InitializeCriticalSection(me2->Mutex);
	u_long val = 1;
	ioctlsocket(*me2->Socket, FIONBIO, &val);

	me2->ThreadHandle = CreateThread(NULL, 0, ConnectThread, me2, 0, NULL);
	return me_;
}

EXPORT void _tcpDtor(SClass* me_)
{
	_tcpFin(me_);
}

EXPORT void _tcpFin(SClass* me_)
{
	STcp* me2 = (STcp*)me_;
	if (me2->Mutex == NULL)
		return;
	EnterCriticalSection(me2->Mutex);
	me2->ThreadExit = True;
	LeaveCriticalSection(me2->Mutex);
	WaitForSingleObject(me2->ThreadHandle, INFINITE);
	CloseHandle(me2->ThreadHandle);
	me2->ThreadHandle = NULL;
	DeleteCriticalSection(me2->Mutex);
	FreeMem(me2->Mutex);
	me2->Mutex = NULL;

	shutdown(*me2->Socket, SD_BOTH);
	closesocket(*me2->Socket);
	FreeMem(me2->Socket);
	me2->Socket = NULL;

	FreeMem(me2->Data);
	me2->DataFull = False;
	me2->DataTop = 0;
	me2->DataBottom = 0;
}

EXPORT void _tcpSend(SClass* me_, const U8* data)
{
	THROWDBG(data == NULL, 0xc0000005);
	STcp* me2 = (STcp*)me_;
	if (me2->ThreadExit)
		return;
	int total = (int)*(S64*)(data + 0x08);
	const U8* ptr = data + 0x10;
	while (total > 0)
	{
		int len = send(*me2->Socket, (const char*)ptr, total, 0);
		if (len == SOCKET_ERROR)
			return;
		ptr += len;
		total -= len;
	}
}

EXPORT void* _tcpReceive(SClass* me_, S64 size)
{
	THROWDBG(size < 0 || TCP_DATA_SIZE < size, 0xe9170006);
	STcp* me2 = (STcp*)me_;
	if (me2->ThreadExit)
		return NULL;

	U8* result;
	if (size == 0)
	{
		result = (U8*)AllocMem(0x10);
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = 0;
	}
	else
	{
		EnterCriticalSection(me2->Mutex);

		if (me2->DataTop == me2->DataBottom && !me2->DataFull)
		{
			LeaveCriticalSection(me2->Mutex);
			return NULL;
		}

		int len;
		if (me2->DataTop < me2->DataBottom)
			len = me2->DataBottom - me2->DataTop;
		else
			len = TCP_DATA_SIZE - me2->DataTop + me2->DataBottom;
		if (len < (int)size)
		{
			LeaveCriticalSection(me2->Mutex);
			return NULL;
		}

		result = (U8*)AllocMem(0x10 + (size_t)size);
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (size_t)size;
		U8* ptr = result + 0x10;
		len = (int)size;
		while (len > 0)
		{
			*ptr = me2->Data[me2->DataTop];
			me2->DataTop++;
			if (me2->DataTop == TCP_DATA_SIZE)
				me2->DataTop = 0;
			ptr++;
			len--;
		}
		me2->DataFull = False;

		LeaveCriticalSection(me2->Mutex);
	}
	return result;
}

EXPORT Bool _tcpConnecting(SClass* me_)
{
	STcp* me2 = (STcp*)me_;
	return !me2->ThreadExit;
}

EXPORT SClass* _makeHttp(SClass* me_, const U8* url, Bool post, const U8* agent)
{
	THROWDBG(url == NULL, 0xc0000005);
	SHttp* me2 = (SHttp*)me_;
	URL_COMPONENTS url_components;
	Char host_name[2049];
	Char url_path[2049];
	memset(&url_components, 0, sizeof(url_components));
	url_components.dwStructSize = sizeof(url_components);
	url_components.lpszHostName = host_name;
	url_components.lpszUrlPath = url_path;
	url_components.dwHostNameLength = 2049;
	url_components.dwUrlPathLength = 2049;
	if (!InternetCrackUrl((const Char*)(url + 0x10), (DWORD)((S64*)(url + 0x08))[1], 0, &url_components))
		return NULL;
	DWORD flags;
	if (url_components.nScheme == INTERNET_SCHEME_HTTP)
		flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT;
	else if (url_components.nScheme == INTERNET_SCHEME_HTTPS)
		flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	else
		return NULL;
	// TODO: Parameters
	if (post)
	{
	}
	Bool success = False;
	for (; ; )
	{
		me2->HandleOpen = InternetOpen(agent == NULL ? L"" : (const Char*)(agent + 0x10), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (me2->HandleOpen == NULL)
			break;
		me2->HandleConnect = InternetConnect(me2->HandleOpen, url_components.lpszHostName, url_components.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		if (me2->HandleConnect == NULL)
			break;
		me2->HandleRequest = HttpOpenRequest(me2->HandleConnect, post ? L"POST" : L"GET", url_components.lpszUrlPath, NULL, NULL, NULL, flags, 0);
		if (me2->HandleRequest == NULL)
			break;
		const Char* header = post ? L"Content-Type: application/x-www-form-urlencoded" : L"";
		if (!HttpSendRequest(me2->HandleRequest, header, (DWORD)wcslen(header), NULL, 0)) // TODO: Parameters
			break;
		{
			DWORD status_code;
			DWORD len = (DWORD)sizeof(DWORD);
			if (!HttpQueryInfo(me2->HandleRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &len, 0))
				break;
			if (status_code < 200 || 299 < status_code)
				break;
		}
		success = True;
		break;
	}
	if (!success)
	{
		if (me2->HandleRequest != NULL)
			InternetCloseHandle(me2->HandleRequest);
		if (me2->HandleConnect != NULL)
			InternetCloseHandle(me2->HandleConnect);
		if (me2->HandleOpen != NULL)
			InternetCloseHandle(me2->HandleOpen);
		return NULL;
	}
	me2->Mutex = (CRITICAL_SECTION*)AllocMem(sizeof(CRITICAL_SECTION));

	me2->ThreadExit = False;
	me2->Success = False;
	me2->TotalSize = 0;
	SHttpList* http_list = (SHttpList*)AllocMem(sizeof(SHttpList));
	http_list->Next = NULL;
	http_list->Size = 0;
	me2->DataTop = http_list;
	me2->DataBottom = http_list;
	InitializeCriticalSection(me2->Mutex);
	me2->ThreadHandle = CreateThread(NULL, 0, HttpThread, me2, 0, NULL);
	return me_;
}

EXPORT void _httpDtor(SClass* me_)
{
	_httpFin(me_);
}

EXPORT void _httpFin(SClass* me_)
{
	SHttp* me2 = (SHttp*)me_;
	if (me2->Mutex == NULL)
		return;
	EnterCriticalSection(me2->Mutex);
	me2->ThreadExit = True;
	LeaveCriticalSection(me2->Mutex);
	WaitForSingleObject(me2->ThreadHandle, INFINITE);
	CloseHandle(me2->ThreadHandle);
	me2->ThreadHandle = NULL;
	DeleteCriticalSection(me2->Mutex);
	FreeMem(me2->Mutex);
	me2->Mutex = NULL;

	if (me2->HandleRequest != NULL)
		InternetCloseHandle(me2->HandleRequest);
	if (me2->HandleConnect != NULL)
		InternetCloseHandle(me2->HandleConnect);
	if (me2->HandleOpen != NULL)
		InternetCloseHandle(me2->HandleOpen);

	while (me2->DataTop != NULL)
	{
		SHttpList* ptr = me2->DataTop;
		me2->DataTop = me2->DataTop->Next;
		FreeMem(ptr);
	}
	me2->Success = False;
	me2->TotalSize = 0;
	me2->DataTop = NULL;
	me2->DataBottom = NULL;
}

EXPORT void* _httpGet(SClass* me_)
{
	SHttp* me2 = (SHttp*)me_;
	if (!me2->Success)
		return NULL;

	EnterCriticalSection(me2->Mutex);
	me2->Success = False;
	U8* result;
	if (me2->TotalSize == 0)
	{
		result = (U8*)AllocMem(0x10);
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = 0;
	}
	else
	{
		char* str = (char*)AllocMem((size_t)me2->TotalSize + 1);
		char* ptr = str;
		SHttpList* ptr2 = me2->DataTop;
		while (ptr2 != NULL)
		{
			memcpy(ptr, ptr2->Data, ptr2->Size);
			ptr += ptr2->Size;
			ptr2 = ptr2->Next;
		}
		ASSERT(ptr == str + me2->TotalSize);
		str[me2->TotalSize] = L'\0';
		size_t len = (size_t)MultiByteToWideChar(CP_UTF8, 0, str, (int)me2->TotalSize, NULL, 0);
		result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)len;
		if (MultiByteToWideChar(CP_UTF8, 0, str, (int)me2->TotalSize, (Char*)(result + 0x10), (int)len) != (int)len)
		{
			FreeMem(result);
			result = (U8*)AllocMem(0x10);
			((S64*)result)[0] = DefaultRefCntFunc;
			((S64*)result)[1] = 0;
		}
		else
			((Char*)(result + 0x10))[len] = L'\0';
		FreeMem(str);
	}
	LeaveCriticalSection(me2->Mutex);
	return result;
}

static DWORD WINAPI ServerThread(LPVOID param)
{
	STcpServer* param2 = (STcpServer*)param;
	for (; ; )
	{
		Sleep(1);

		EnterCriticalSection(param2->Mutex);
		if (param2->ThreadExit)
		{
			LeaveCriticalSection(param2->Mutex);
				break;
		}

		SOCKADDR_IN addr;
		int len = (int)sizeof(addr);
		SOCKET sock = accept(*param2->Socket, (SOCKADDR*)&addr, &len);
		if (sock == INVALID_SOCKET)
		{
			LeaveCriticalSection(param2->Mutex);
			continue;
		}
		SServerConnectList* connect = (SServerConnectList*)AllocMem(sizeof(SServerConnectList));
		connect->Next = NULL;
		connect->Socket = sock;

		if (param2->ConnectTop == NULL)
		{
			param2->ConnectTop = connect;
			param2->ConnectBottom = connect;
		}
		else
		{
			param2->ConnectBottom->Next = connect;
			param2->ConnectBottom = connect;
		}
		LeaveCriticalSection(param2->Mutex);
	}
	return TRUE;
}

static DWORD WINAPI ConnectThread(LPVOID param)
{
	STcp* param2 = (STcp*)param;
	for (; ; )
	{
		Sleep(1);

		EnterCriticalSection(param2->Mutex);
		if (param2->ThreadExit)
		{
			param2->DataFull = False;
			param2->DataTop = 0;
			param2->DataBottom = 0;
			LeaveCriticalSection(param2->Mutex);
			break;
		}
		if (param2->DataFull)
		{
			LeaveCriticalSection(param2->Mutex);
			continue;
		}

		int size;
		if (param2->DataTop <= param2->DataBottom)
			size = TCP_DATA_SIZE - param2->DataBottom;
		else
			size = param2->DataTop - param2->DataBottom;
		int received = recv(*param2->Socket, (char*)(param2->Data + param2->DataBottom), size, 0);
		if (received > 0)
		{
			param2->DataBottom += received;
			if (param2->DataBottom == TCP_DATA_SIZE)
				param2->DataBottom = 0;
			if (param2->DataBottom == param2->DataTop)
				param2->DataFull = True;
		}

		// Disconnected
		if (received == 0)
		{
			param2->ThreadExit = True;
			param2->DataFull = False;
			param2->DataTop = 0;
			param2->DataBottom = 0;
			LeaveCriticalSection(param2->Mutex);
			break;
		}

		LeaveCriticalSection(param2->Mutex);
	}
	return TRUE;
}

static DWORD WINAPI HttpThread(LPVOID param)
{
	SHttp* param2 = (SHttp*)param;
	for (; ; )
	{
		Sleep(1);

		EnterCriticalSection(param2->Mutex);
		if (param2->ThreadExit)
		{
			param2->Success = False;
			param2->TotalSize = 0;
			LeaveCriticalSection(param2->Mutex);
			break;
		}

		DWORD read_size = 0;
		if (!InternetReadFile(param2->HandleRequest, param2->DataBottom->Data, HTTP_DATA_SIZE, &read_size))
		{
			param2->ThreadExit = True;
			param2->Success = False;
			param2->TotalSize = 0;
			LeaveCriticalSection(param2->Mutex);
			break;
		}

		if (read_size == 0)
		{
			param2->ThreadExit = True;
			param2->Success = True;
			LeaveCriticalSection(param2->Mutex);
			break;
		}
		else
		{
			param2->DataBottom->Size = (size_t)read_size;
			param2->TotalSize += (size_t)read_size;
			SHttpList* http_list = (SHttpList*)AllocMem(sizeof(SHttpList));
			http_list->Next = NULL;
			http_list->Size = 0;
			param2->DataBottom->Next = http_list;
			param2->DataBottom = http_list;
		}

		LeaveCriticalSection(param2->Mutex);
	}
	return TRUE;
}
