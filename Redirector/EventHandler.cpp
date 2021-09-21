#include "EventHandler.h"

#include "TCPHandler.h"

extern BOOL filterTCP;
extern BOOL filterUDP;
extern vector<wstring> bypassList;
extern vector<wstring> handleList;

extern USHORT tcpListen;

wstring ConvertIP(PSOCKADDR addr)
{
	WCHAR buffer[MAX_PATH] = L"";
	DWORD bufferLength = MAX_PATH;

	if (addr->sa_family == AF_INET)
	{
		WSAAddressToStringW(addr, sizeof(SOCKADDR_IN), NULL, buffer, &bufferLength);
	}
	else
	{
		WSAAddressToStringW(addr, sizeof(SOCKADDR_IN6), NULL, buffer, &bufferLength);
	}

	return buffer;
}

wstring GetProcessName(DWORD id)
{
	if (id == 0)
	{
		return L"Idle";
	}

	if (id == 4)
	{
		return L"System";
	}

	wchar_t name[MAX_PATH];
	if (!nf_getProcessNameFromKernel(id, name, MAX_PATH))
	{
		if (!nf_getProcessNameW(id, name, MAX_PATH))
		{
			return L"Unknown";
		}
	}

	wchar_t data[MAX_PATH];
	if (GetLongPathNameW(name, data, MAX_PATH))
	{
		return data;
	}

	return name;
}

BOOL checkBypassName(DWORD id)
{
	auto name = GetProcessName(id);

	for (size_t i = 0; i < bypassList.size(); i++)
	{
		if (regex_search(name, wregex(bypassList[i])))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL checkHandleName(DWORD id)
{
	auto name = GetProcessName(id);

	for (size_t i = 0; i < handleList.size(); i++)
	{
		if (regex_search(name, wregex(handleList[i])))
		{
			return TRUE;
		}
	}

	return FALSE;
}

bool eh_init()
{
	return TCPHandler::Init();
}

void eh_free()
{
	TCPHandler::Free();
}

void threadStart()
{

}

void threadEnd()
{

}

void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO info)
{
	if (!filterTCP)
	{
		nf_tcpDisableFiltering(id);

		wcout << "[Redirector][EventHandler][tcpConnectRequest][" << id << "][" << info->processId << "][!filterTCP] " << GetProcessName(info->processId) << endl;
		return;
	}

	if (checkBypassName(info->processId))
	{
		nf_tcpDisableFiltering(id);

		wcout << "[Redirector][EventHandler][tcpConnectRequest][" << id << "][" << info->processId << "][checkBypassName] " << GetProcessName(info->processId) << endl;
		return;
	}

	if (!checkHandleName(info->processId))
	{
		nf_tcpDisableFiltering(id);

		wcout << "[Redirector][EventHandler][tcpConnectRequest][" << id << "][" << info->processId << "][!checkHandleName] " << GetProcessName(info->processId) << endl;
		return;
	}

	if (info->ip_family != AF_INET && info->ip_family != AF_INET6)
	{
		nf_tcpDisableFiltering(id);

		wcout << "[Redirector][EventHandler][tcpConnectRequest][" << id << "][" << info->processId << "][!IPv4 && !IPv6] " << GetProcessName(info->processId) << endl;
		return;
	}

	SOCKADDR_IN6 client;
	memcpy(&client, info->localAddress, sizeof(SOCKADDR_IN6));

	SOCKADDR_IN6 remote;
	memcpy(&remote, info->remoteAddress, sizeof(SOCKADDR_IN6));

	if (info->ip_family == AF_INET)
	{
		auto addr = (PSOCKADDR_IN)info->remoteAddress;
		addr->sin_family = AF_INET;
		addr->sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);
		addr->sin_port = tcpListen;
	}

	if (info->ip_family == AF_INET6)
	{
		auto addr = (PSOCKADDR_IN6)info->remoteAddress;
		IN6ADDR_SETLOOPBACK(addr);
		addr->sin6_port = tcpListen;
	}

	TCPHandler::CreateHandler(client, remote);

	wcout << "[Redirector][EventHandler][tcpConnectRequest][" << id << "][" << info->processId << "] " << ConvertIP((PSOCKADDR)&client) << " -> " << ConvertIP((PSOCKADDR)&remote) << endl;
}

void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO info)
{
	wcout << "[Redirector][EventHandler][tcpConnected][" << id << "][" << info->processId << "][" << ConvertIP((PSOCKADDR)info->remoteAddress) << "] " << GetProcessName(info->processId) << endl;
}

void tcpCanSend(ENDPOINT_ID id)
{
	UNREFERENCED_PARAMETER(id);
}

void tcpSend(ENDPOINT_ID id, const char* buffer, int length)
{
	nf_tcpPostSend(id, buffer, length);
}

void tcpCanReceive(ENDPOINT_ID id)
{
	UNREFERENCED_PARAMETER(id);
}

void tcpReceive(ENDPOINT_ID id, const char* buffer, int length)
{
	nf_tcpPostReceive(id, buffer, length);
}

void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO info)
{
	SOCKADDR_IN6 client;
	memcpy(&client, info->localAddress, sizeof(SOCKADDR_IN6));

	TCPHandler::DeleteHandler(client);

	printf("[Redirector][EventHandler][tcpClosed][%llu][%lu]\n", id, info->processId);
}

void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO info)
{
	if (!filterUDP)
	{
		nf_udpDisableFiltering(id);

		wcout << "[Redirector][EventHandler][udpCreated][" << id << "][" << info->processId << "][!filterUDP] " << GetProcessName(info->processId) << endl;
		return;
	}

	if (checkBypassName(info->processId))
	{
		nf_udpDisableFiltering(id);

		wcout << "[Redirector][EventHandler][udpCreated][" << id << "][" << info->processId << "][checkBypassName] " << GetProcessName(info->processId) << endl;
		return;
	}

	if (!checkHandleName(info->processId))
	{
		nf_udpDisableFiltering(id);

		wcout << "[Redirector][EventHandler][udpCreated][" << id << "][" << info->processId << "][!checkHandleName] " << GetProcessName(info->processId) << endl;
		return;
	}

	nf_udpDisableFiltering(id);
}

void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST info)
{
	UNREFERENCED_PARAMETER(id);
	UNREFERENCED_PARAMETER(info);
}

void udpCanSend(ENDPOINT_ID id)
{
	UNREFERENCED_PARAMETER(id);
}

void udpSend(ENDPOINT_ID id, const unsigned char* target, const char* buffer, int length, PNF_UDP_OPTIONS options)
{
	nf_udpPostSend(id, target, buffer, length, options);
}

void udpCanReceive(ENDPOINT_ID id)
{
	UNREFERENCED_PARAMETER(id);
}

void udpReceive(ENDPOINT_ID id, const unsigned char* target, const char* buffer, int length, PNF_UDP_OPTIONS options)
{
	nf_udpPostReceive(id, target, buffer, length, options);
}

void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO info)
{
	UNREFERENCED_PARAMETER(id);
	UNREFERENCED_PARAMETER(info);
}
