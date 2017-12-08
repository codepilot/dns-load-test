// dns-load-test.cpp : Defines the entry point for the console application.
//

#define MainCPP

#include "Platform.h"

__int64 volatile GlobalCompletionCount{ 0 };

#include "CommandLine.h"
CommandLine cmd;

#include "ClsSockets.h"
#include "ErrorFormatMessage.h"
#include "UserHandle.h"
#include "InputOutputCompletionPort.h"
#include "DomainNameSystem.h"
#include "Statistics.h"
#include "rio.h"

namespace {
	size_t SendsPerSocket = 8ui64; //default
	size_t GlobalRioBufferSize = 4096ui64 * 1024ui64; //default
	rio::IPv4_AddressRangePort ClientAddress;
	rio::IPv4_AddressRangePort ServerAddress;

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
		buf.init(sock, SafeInt<DWORD>(GlobalRioBufferSize));

		std::vector<rio::RioSock> sendSockets;
		sendSockets.reserve(ClientAddress.rangeAddresses());
		for (decltype(sendSockets.capacity()) i{ 0 }; i < sendSockets.capacity(); i++) {
			sendSockets.emplace_back(std::move(rio::RioSock()));
		}

		RIO_BUF udpData{ buf.append(&DomainNameSystem::udpMsgData, sizeof(DomainNameSystem::udpMsgData)) };
		RIO_BUF udpRemote{ buf.appendAddress(ServerAddress, ServerAddress.port) };

		uint32_t rangeOffset = 0;
		for (auto &sendSock : sendSockets) {
			sendSock.init(ClientAddress.rangeOffset(rangeOffset++), sendCQ, recvCQ, 16, 16, &sendSock);
		}

		std::vector<rio::SendExRequest> ser;
		for (auto &sendSock : sendSockets) {
			for (decltype(SendsPerSocket) i{ 0 }; i < SendsPerSocket; i++) {
				sendSock.queueSendEx(ser, &udpData, &udpRemote);
			}
		}

		HANDLE MainThread = UserHandle::Handle::duplicate(GetCurrentThread());

		for (auto &sendRequest : ser) {
			sendRequest.AccumFunc = [](ULONG_PTR Parameter) {
				sendStats.addSample(static_cast<double_t>( Parameter));
			};//AccumSendTicks;
			sendRequest.AccumSize = 128;
			sendRequest.AccumThread = MainThread;
		}


		rio::SendExRequest::sendAll(ser);
		sock.RIONotify(sendCQ.completion);

		ThreadVector::runThreads(1000, []() {
			//std::cout << sendStats.statistics() << std::endl;
			__int64 capturedCount = GlobalCompletionCount;
			std::cout << capturedCount << "pps " << (capturedCount * sizeof(DomainNameSystem::udpMsgData)) / 125000 << "Mbps" << std::endl;
			InterlockedAdd64(&GlobalCompletionCount, -capturedCount);
		});
		printf("test");
	}
};

void parseCommandLine() {

	ServerAddress = cmd.argMap[L"server"];
	ClientAddress = cmd.argMap[L"client"];

	SendsPerSocket = cmd.argMap.count(L"SendsPerSocket") ?
		std::stoull(cmd.argMap[L"SendsPerSocket"]) :
		SendsPerSocket;

	GlobalRioBufferSize = cmd.argMap.count(L"GlobalRioBufferSize") ?
		std::stoull(cmd.argMap[L"GlobalRioBufferSize"]) :
		GlobalRioBufferSize;

}

int main()
{
	parseCommandLine();

	Sockets::Win10SocketLib win10SocketLib;
	startTest();

	puts("Press any key to quit");
	return getchar();

}
