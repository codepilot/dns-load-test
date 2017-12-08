// dns-load-test.cpp : Defines the entry point for the console application.
//

#define MainCPP

#include "Platform.h"
#include "ClsSockets.h"
#include "ErrorFormatMessage.h"
#include "UserHandle.h"
#include "InputOutputCompletionPort.h"
#include "DomainNameSystem.h"
#include "Statistics.h"
#include "rio.h"

namespace {
	const auto SendsPerSocket = 8;
	const auto NumSendSockets = 16;
	const size_t GlobalRioBufferSize = 4096ull * 1ull;
	const uint32_t ServerAddress = 0x0400020a;
	const uint32_t ServerPort = 53;
	const rio::IPv4_Address ClientAddress{ 10, 0, 0, 3 };
	HANDLE MainThread;

	InputOutputCompletionPort::IOCP iocp;

#include "ThreadVector.h"
	rio::CompletionQueue sendCQ;
	rio::CompletionQueue recvCQ;
	rio::Buffer buf;

	Statistics::StandardDeviation sendStats;

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

		for (auto &sendRequest : ser) {
			sendRequest.AccumFunc = [](ULONG_PTR Parameter) {
				sendStats.addSample(Parameter);
			};//AccumSendTicks;
			sendRequest.AccumSize = 1024;
			sendRequest.AccumThread = MainThread;
		}


		rio::SendExRequest::sendAll(ser);
		sock.RIONotify(sendCQ.completion);

		ThreadVector::runThreads(1000, []() {
			std::cout << sendStats.statistics() << std::endl;
		});
		printf("test");
	}
};

int main()
{
	MainThread = UserHandle::Handle::duplicate(GetCurrentThread());
#if 1
	Sockets::Win10SocketLib win10SocketLib;
	startTest();
#else
	Statistics::StandardDeviation stddev;
	std::array<double_t, 8> samples{2, 4, 4, 4, 5, 5, 7, 9};
	for (const auto &sample : samples) {
		stddev.addSample(sample);
	}
	std::cout << stddev.statistics() << std::endl;
#endif
	puts("Press any key to quit");
	return getchar();

}
