// dns-load-test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

Sockets::GuidMsTcpIp Sockets::GenericWin10Socket::GUID_WSAID;

#include "ErrorFormatMessage.h"

namespace {

#include "rio.h"

	const auto SendsPerSocket = 8;
	const auto NumSendSockets = 16;
	const size_t GlobalRioBufferSize = 4096ull * 1ull;
	const uint32_t ServerAddress = 0x0400020a;
	const rio::IPv4_Address ClientAddress{ 10, 0, 0, 3 };


	const uint8_t udpMsgData[36]{

		0x02, 0xdc, //Transaction ID: 0x02DC
		0x01, 0x00, //Flags: 0x0100 Standard Query
		0x00, 0x01, //Questions: 1
		0x00, 0x00, //Answers: 0
		0x00, 0x00, //Authority: 0
		0x00, 0x00, //Additionals: 0
		
		//Query: ns4.archeofutur.us: type AAAA, class IN
			0x03, //3 bytes
				0x6e, 0x73, 0x34, // "ns4"
			0x0b, // 11 bytes
				0x61, 0x72,	0x63, 0x68, 0x65, 0x6f, 0x66, 0x75, 0x74, 0x75, 0x72, // "archeofutur"
			0x02, // 2 bytes
				0x75, 0x73, // "us"
			0x00, // 0 bytes, end of domain name
		
			0x00, 0x1c, //type 28 AAAA
			0x00, 0x01 //Class 1 Internet
	};


#include "UserHandle.h"
#include "InputOutputCompletionPort.h"

	InputOutputCompletionPort::IOCP iocp;
	rio::CompletionQueue sendCQ;
	rio::CompletionQueue recvCQ;
	rio::Buffer buf;

#include "ThreadVector.h"
#include "InputOutputCompletionPort.h"

	void startTest() {
		Sockets::GenericWin10Socket sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO);
		static Sockets::GuidMsTcpIp guidMsTcpIP;
		Sockets::MsSockFuncPtrs funcPtrs(sock.getSocket(), guidMsTcpIP);
		sendCQ.init(sock, iocp);
		recvCQ.init(sock, iocp);
		buf.init(sock, GlobalRioBufferSize);

		std::vector<rio::RioSock> sendSockets;
		sendSockets.reserve(NumSendSockets);
		for (int i = 0; i < sendSockets.capacity(); i++) {
			sendSockets.emplace_back(std::move(rio::RioSock()));
		}

		RIO_BUF udpData{ buf.bufid , 0, sizeof(udpMsgData) };
		SOCKADDR_INET remoteSocket; SecureZeroMemory(&remoteSocket, sizeof(remoteSocket));
		remoteSocket.si_family = AF_INET;
		remoteSocket.Ipv4.sin_family = AF_INET;
		remoteSocket.Ipv4.sin_port = htons(53);
		remoteSocket.Ipv4.sin_addr.S_un.S_addr = ServerAddress;

		RIO_BUF udpRemote{ buf.bufid , 1024, sizeof(remoteSocket) };
		RtlCopyMemory(reinterpret_cast<uint8_t *>(buf.buf) + 1024, &remoteSocket, sizeof(remoteSocket));
		RtlCopyMemory(reinterpret_cast<uint8_t *>(buf.buf) + 0, udpMsgData, sizeof(udpMsgData));

		for (auto &sendSock : sendSockets) {
			sendSock.init(ClientAddress, sendCQ, recvCQ, 16, 16, &sendSock);
		}

		std::vector<rio::SendExRequest> ser;
		for (auto &sendSock : sendSockets) {
			for (int i = 0; i < SendsPerSocket; i++) {
				sendSock.queueSendEx(ser, &udpData, &udpRemote);
			}
		}

		rio::SendExRequest::sendAll(ser);
		sock.RIONotify(sendCQ.completion);

		ThreadVector::runThreads();
		printf("test");
		//sock.RIOSendEx(cq.completion,)
	}



};

int main()
{
	Sockets::Win10SocketLib win10SocketLib;
	startTest();
	return 0;
}
