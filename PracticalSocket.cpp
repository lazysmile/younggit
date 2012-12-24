//&>/dev/null;x="${0%.*}";[ ! "$x" -ot "$0" ]||(rm -f "$x";g++ -o "$x" "$0")&&"$x" $*;exit
// =====================================================================================
// 
//       Filename:  PracticalSocket.cpp
// 
//    Description:  C++ sockets on Unix and windows,source file
// 
//        Version:  1.0
//        Created:  10/31/2011 09:57:45 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  Philip.Young (Cxy), yangcongxi@cdeledu.com
//        Company:  CDEL
// 
// =====================================================================================

#include "PracticalSocket.h"

#ifdef WIN32
	#include<winsock.h>
	typedef int socklen_t;
	typedef char raw_type;
#else
	#include<sys/types.h>
	#include<sys/socket.h>
	#include<netdb.h>
	#include<arpa/inet.h>
	#include<unistd.h>
	#include<netinet/in.h>
	typedef void raw_type;
#endif


#include<errno.h>
using namespace std;

#ifdef WIN32
static bool initialized = false;
#endif

SocketException::SocketException(const string &message, bool inclSysMsg) throw(): userMessage(message)
{
	if (inclSysMsg)
	{
		userMessage.append(": ");
		userMessage.append(strerror(errno));
	}
}

SocketException::~SocketException() throw() {}

const char *SocketException::what() const throw()
{
	return userMessage.c_str();
}

static void fillAddr(const string &address, unsigned short port, sockaddr_in &addr)
{
	memset(&addr,0, sizeof(addr));
	addr.sin_family = AF_INET;
	hostent *host;
	if ((host = gethostbyname(address.c_str())) == NULL)
	{
		throw SocketException("Failed to resolve name (gethostbyname())");
	}
	addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
	addr.sin_port = htons(port);
}

Socket::Socket(int type ,int protocol) throw(SocketException)
{
	#ifdef WIN32
		if (!initialized)
		{
			WORD wVersionRequested;
			WSADATA wsaData;
			wVersionRequested = MAKEWORD(2,0);
			if (WSAStartup(wVersionRequested,&wsaData) != 0)
			{
				throw SocketException("UNABLE to load winsock dll");
			}
			initialized = true;
		}
	#endif

	if ((sockDesc = socket(PF_INET, type, protocol)) < 0)
	{
		throw SocketException("socket creation failed(socket())",true);
	}
}

Socket::Socket(int sockdesc)
{
	this->sockDesc = sockdesc;
}

Socket::~Socket()
{
	#ifdef WIN32
		::closesocket(sockDesc);
	#else
		::close(sockDesc);
	#endif
		sockDesc = -1;
}

string Socket::getLocalAddress() throw(SocketException)
{
	sockaddr_in addr;

	unsigned int addr_len = sizeof(addr);

	if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0)
	{
		throw SocketException("fetch of local address failed (getsockname())",true);
	}
	return inet_ntoa(addr.sin_addr);
}

unsigned short Socket::getLocalPort() throw(SocketException)
{
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);
	if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0)
	{
		throw SocketException("fetch of local port failed (getsockname())",true);
	}
	return ntohs(addr.sin_port);
}


void Socket::setLocalPort(unsigned short localPort) throw(SocketException)
{
	sockaddr_in localAddr;
	memset(&localAddr,0,sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(localPort);

	if (bind(sockDesc, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0)
	{
		throw SocketException("set of local port failed (bind())",true);
	}
}

void Socket::setLocalAddressAndPort(const string &localAddress,unsigned short localPort) throw(SocketException)
{
	sockaddr_in localAddr;
	fillAddr(localAddress, localPort, localAddr);

	if (bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0)
	{
		throw SocketException("SET of local address and port failed(bind())",true);
	}
}

void Socket::cleanUp() throw(SocketException)
{
	#ifdef WIN32
		if (WSACleanup() != 0)
		{
			throw SocketException("WSACleanup() failed");
		}
	#endif
}

unsigned short Socket::resolveService(const string &service, const string &protocol)
{
	struct servent *serv;
	if ((serv = getservbyname(service.c_str(),protocol.c_str())) == NULL)
		return atoi(service.c_str());
	else
		return ntohs(serv->s_port);
}

CommunicatingSocket::CommunicatingSocket(int type, int protocol) throw(SocketException): Socket(type, protocol)
{
}

CommunicatingSocket::CommunicatingSocket(int newConnSD) : Socket(newConnSD)
{

}

void CommunicatingSocket::connect(const string &foreignAddress,unsigned short foreignPort) throw(SocketException)
{
	sockaddr_in destAddr;
	fillAddr(foreignAddress, foreignPort, destAddr);

	if(::connect(sockDesc, (sockaddr *) &destAddr, sizeof(destAddr))<0)
	{
		throw SocketException("connect failed (connect())",true);
	}
}

void CommunicatingSocket::send(const void *buffer, int bufferLen) throw(SocketException)
{
	if (::send(sockDesc, (raw_type *)buffer,bufferLen,0) < 0)
	{
		throw SocketException("send failed (send)",true);
	}
}

size_t CommunicatingSocket::recv(void *buffer,int bufferLen) throw(SocketException)
{
	size_t rtn;
	if ((rtn = ::recv(sockDesc,(raw_type *) buffer, bufferLen, 0)) < 0)
	{
		throw SocketException("receive failed",true);
	}
	return rtn;
}

string CommunicatingSocket::getForeignAddress() throw(SocketException)
{
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);

	if (getpeername(sockDesc,(sockaddr *) &addr, (socklen_t *) &addr_len) < 0)
	{
		throw SocketException("fetch of foreign address failed",true);

	}
	return inet_ntoa(addr.sin_addr);
}

unsigned short CommunicatingSocket::getForeignPort() throw(SocketException)
{
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);

	if (getpeername(sockDesc, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0)
	{
		throw SocketException("fetch of foreign port failed",true);
	}
	return ntohs(addr.sin_port);
}

TCPSocket::TCPSocket() throw(SocketException) : CommunicatingSocket(SOCK_STREAM,IPPROTO_TCP){}

TCPSocket::TCPSocket(const string &foreignAddress,unsigned short foreignPort) throw(SocketException) : CommunicatingSocket(SOCK_STREAM,IPPROTO_TCP)
{
	connect(foreignAddress,foreignPort);
}

TCPSocket::TCPSocket(int newConnSD) : CommunicatingSocket(newConnSD){}

TCPServerSocket::TCPServerSocket(unsigned short localPort, int queueLen) throw(SocketException) : Socket(SOCK_STREAM, IPPROTO_TCP)
{
	setLocalPort(localPort);
	setListen(queueLen);
}

TCPServerSocket::TCPServerSocket(const string &localAddress,unsigned short localPort, int queueLen) throw(SocketException) : Socket(SOCK_STREAM,IPPROTO_TCP)
{
	setLocalAddressAndPort(localAddress,localPort);
	setListen(queueLen);
}

TCPSocket *TCPServerSocket::accept() throw(SocketException)
{
	int newConnSD;
	if ((newConnSD = ::accept(sockDesc,NULL,0)) < 0)
	{
		throw SocketException("accept failed",true);
	}
	return new TCPSocket(newConnSD);
}

void TCPServerSocket::setListen(int queueLen) throw(SocketException)
{
	if (listen(sockDesc,queueLen)<0)
	{
		throw SocketException("set listening socket failed",true);
	}
}

UDPSocket::UDPSocket() throw(SocketException) : CommunicatingSocket(SOCK_DGRAM,IPPROTO_UDP)
{
	setBroadcast();
}

UDPSocket::UDPSocket(unsigned short localPort) throw(SocketException) : CommunicatingSocket(SOCK_DGRAM,IPPROTO_UDP)
{
	setLocalPort(localPort);
	setBroadcast();
}

UDPSocket::UDPSocket(const string &localAddress,unsigned short localPort) throw(SocketException) : CommunicatingSocket(SOCK_DGRAM,IPPROTO_UDP)
{
	setLocalAddressAndPort(localAddress,localPort);
	setBroadcast();
}

void UDPSocket::setBroadcast()
{
	int broadcastPermission = 1;
	setsockopt(sockDesc, SOL_SOCKET,SO_BROADCAST, (raw_type *) &broadcastPermission,sizeof(broadcastPermission));
}

void UDPSocket::disconnect() throw(SocketException)
{
	sockaddr_in nullAddr;
	memset(&nullAddr,0,sizeof(nullAddr));
	nullAddr.sin_family = AF_UNSPEC;

	if (::connect(sockDesc, (sockaddr *) &nullAddr, sizeof(nullAddr)) < 0)
	{
		#ifdef WIN32
			if (errno != WSAEAFNOSUPPORT) 
			{
		#else
			if (errno != EAFNOSUPPORT) 
			{
		#endif
				throw SocketException("disconnect failed",true);
			}
	}
}

void UDPSocket::sendTo(const void *buffer, int bufferLen, const string &foreignAddress, unsigned short foreignPort) throw(SocketException)
{
	sockaddr_in destAddr;
	fillAddr(foreignAddress,foreignPort,destAddr);

	if (sendto(sockDesc,(raw_type *) buffer, bufferLen, 0, (sockaddr *) &destAddr, sizeof(destAddr)) != bufferLen)
	{
		throw SocketException("Send failed",true);
	}
}


int UDPSocket::recvFrom(void *buffer, int bufferLen, string &sourceAddress, unsigned short &sourcePort) throw(SocketException)
{
	sockaddr_in clntAddr;
	socklen_t addrLen = sizeof(clntAddr);
	int rtn;
	if ((rtn = recvfrom(sockDesc,(raw_type *) buffer,bufferLen,0,(sockaddr *) &clntAddr, (socklen_t *) &addrLen)) < 0)
	{
		throw SocketException("receive failed",true);
	}

	sourceAddress = inet_ntoa(clntAddr.sin_addr);
	sourcePort = ntohs(clntAddr.sin_port);
	return rtn;
}

void UDPSocket::setMulticastTTL(unsigned char multicastTTL) throw(SocketException)
{
	if (setsockopt(sockDesc, IPPROTO_IP, IP_MULTICAST_TTL,(raw_type *) &multicastTTL, sizeof(multicastTTL)) < 0)
	{
		throw SocketException("Multicast TTL set failed",true);
	}
}

void UDPSocket::joinGroup(const string &multicastTTL) throw(SocketException)
{
	struct ip_mreq multicastRequest;
	multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastTTL.c_str());
	multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sockDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP,(raw_type *) &multicastRequest,sizeof(multicastRequest)) < 0)
	{
		throw SocketException("multicast group join failed",true);
	}
}

void UDPSocket::leaveGroup(const string &multicastGroup) throw(SocketException)
{
	struct ip_mreq multicastRequest;
	multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
	multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sockDesc, IPPROTO_IP,IP_DROP_MEMBERSHIP, (raw_type *) &multicastRequest, sizeof(multicastRequest)) < 0)
	{
		throw SocketException("multicast group leaving failed",true);
	}
}





