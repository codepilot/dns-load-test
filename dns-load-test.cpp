// dns-load-test.cpp : Defines the entry point for the console application.
//

#include "Platform.h"


#include "ClsSockets.h"

Sockets::GuidMsTcpIp Sockets::GenericWin10Socket::GUID_WSAID;

#include "ErrorFormatMessage.h"

#include "UserHandle.h"
#include "InputOutputCompletionPort.h"
#include "DomainNameSystem.h"


namespace {

#include "rio.h"

	const auto SendsPerSocket = 8;
	const auto NumSendSockets = 16;
	const size_t GlobalRioBufferSize = 4096ull * 1ull;
	const uint32_t ServerAddress = 0x0400020a;
	const uint32_t ServerPort = 53;
	const rio::IPv4_Address ClientAddress{ 10, 0, 0, 3 };





	InputOutputCompletionPort::IOCP iocp;
#include "ThreadVector.h"
	rio::CompletionQueue sendCQ;
	rio::CompletionQueue recvCQ;
	rio::Buffer buf;


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

		RIO_BUF udpData{ buf.append(&DomainNameSystem::udpMsgData, sizeof(DomainNameSystem::udpMsgData)) };
		RIO_BUF udpRemote{ buf.appendAddress(ServerAddress, ServerPort) };

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
