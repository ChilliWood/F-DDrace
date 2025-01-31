/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/console.h>

#include "netban.h"
#include "network.h"


bool CNetConsole::Open(NETADDR BindAddr, CNetBan *pNetBan, NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser)
{
	// zero out the whole structure
	mem_zero(this, sizeof(*this));
	m_Socket.type = NETTYPE_INVALID;
	m_Socket.ipv4sock = -1;
	m_Socket.ipv6sock = -1;
	m_pNetBan = pNetBan;

	// open socket
	m_Socket = net_tcp_create(BindAddr);
	if(!m_Socket.type)
		return false;
	if(net_tcp_listen(m_Socket, NET_MAX_CONSOLE_CLIENTS))
		return false;
	net_set_non_blocking(m_Socket);

	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
		m_aSlots[i].m_Connection.Reset();

	m_pfnNewClient = pfnNewClient;
	m_pfnDelClient = pfnDelClient;
	m_UserPtr = pUser;

	return true;
}

void CNetConsole::Close()
{
	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
		Drop(i, "Closing console");

	net_tcp_close(m_Socket);
}

void CNetConsole::Drop(int ClientID, const char *pReason)
{
	if(ClientID < 0 || ClientID >= NET_MAX_CONSOLE_CLIENTS || m_aSlots[ClientID].m_Connection.State() == NET_CONNSTATE_OFFLINE)
		return;

	if(m_pfnDelClient)
		m_pfnDelClient(ClientID, pReason, m_UserPtr);

	m_aSlots[ClientID].m_Connection.Disconnect(pReason);
}

int CNetConsole::AcceptClient(NETSOCKET Socket, const NETADDR *pAddr)
{
	char aError[256] = { 0 };
	int FreeSlot = -1;

	// look for free slot or multiple client
	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
	{
		if(FreeSlot == -1 && m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
			FreeSlot = i;
		if(m_aSlots[i].m_Connection.State() != NET_CONNSTATE_OFFLINE)
		{
			if(net_addr_comp(pAddr, m_aSlots[i].m_Connection.PeerAddress(), true) == 0)
			{
				str_copy(aError, "only one client per IP allowed", sizeof(aError));
				break;
			}
		}
	}

	// accept client
	if(!aError[0] && FreeSlot != -1)
	{
		m_aSlots[FreeSlot].m_Connection.Init(Socket, pAddr);
		if(m_pfnNewClient)
			m_pfnNewClient(FreeSlot, false, m_UserPtr);
		return 0;
	}

	// reject client
	if(!aError[0])
		str_copy(aError, "no free slot available", sizeof(aError));

	net_tcp_send(Socket, aError, str_length(aError));
	net_tcp_close(Socket);

	return -1;
}

int CNetConsole::Update()
{
	NETSOCKET Socket;
	NETADDR Addr;

	if(net_tcp_accept(m_Socket, &Socket, &Addr) > 0)
	{
		// check if we just should drop the packet
		char aBuf[128];
		int LastInfoQuery;
		if(NetBan() && NetBan()->IsBanned(&Addr, aBuf, sizeof(aBuf), &LastInfoQuery))
		{
			// banned, reply with a message (5 second cooldown) and drop
			int Time = time_timestamp();
			if(LastInfoQuery + 5 < Time)
			{
				net_tcp_send(Socket, aBuf, str_length(aBuf));
			}
			net_tcp_close(Socket);
		}
		else
			AcceptClient(Socket, &Addr);
	}

	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
	{
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ONLINE)
			m_aSlots[i].m_Connection.Update();
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR)
			Drop(i, m_aSlots[i].m_Connection.ErrorString());
	}

	return 0;
}

int CNetConsole::Recv(char *pLine, int MaxLength, int *pClientID)
{
	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
	{
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ONLINE && m_aSlots[i].m_Connection.Recv(pLine, MaxLength))
		{
			if(pClientID)
				*pClientID = i;
			return 1;
		}
	}
	return 0;
}

int CNetConsole::Send(int ClientID, const char *pLine)
{
	if(m_aSlots[ClientID].m_Connection.State() == NET_CONNSTATE_ONLINE)
		return m_aSlots[ClientID].m_Connection.Send(pLine);
	else
		return -1;
}

void CNetConsole::SetLingerState(int State)
{
	net_tcp_set_linger(m_Socket, State);
}
