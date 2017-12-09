// dns-load-test.cpp : Defines the entry point for the console application.
//

#define MainCPP

#include "Platform.h"

__declspec(align(64)) __int64 volatile GlobalSendCount{ 0 };
__declspec(align(64)) __int64 volatile GlobalReceiveCount{ 0 };

__declspec(align(64)) __int64 volatile GlobalSendCompletionCount{ 0 };
__declspec(align(64)) __int64 volatile GlobalReceiveCompletionCount{ 0 };

__declspec(align(64)) __int64 volatile GlobalSendAckCount{ 0 };
__declspec(align(64)) __int64 volatile GlobalReceiveAckCount{ 0 };

__declspec(align(64)) __int64 volatile GlobalIncompletionCount{ 0 };



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
	size_t GlobalSendBufferSize = 4096ui64 * 1024ui64; //default
	size_t SendPerAddress = 1; //default
	size_t RunTimeSecond = 1;//default
	size_t NumSendCQs = 16;
	size_t NumReceiveCQs = 16;
	__int64 startTick;
	__int64 finishTick;

	rio::IPv4_AddressRangePort ClientAddress;
	rio::IPv4_AddressRangePort ServerAddress;

	InputOutputCompletionPort::IOCP iocp;

	std::vector<rio::CompletionQueue<8192>> sendCQs;
	std::vector<rio::CompletionQueue<8192>> recvCQs;
	rio::Buffer globalSendBuffer;
	rio::Buffer globalReceiveBuffer;
#include "ThreadVector.h"

	Statistics::StandardDeviation sendStats;

	void startTest() {
		Sockets::GenericWin10Socket sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO);
		static Sockets::GuidMsTcpIp guidMsTcpIP;
		Sockets::MsSockFuncPtrs funcPtrs(sock.getSocket(), guidMsTcpIP);

		sendCQs.resize(NumSendCQs);
		recvCQs.resize(NumReceiveCQs);

		for (auto &sendCQ : sendCQs) { sendCQ.init(sock, iocp); }
		for (auto &recvCQ : recvCQs) { recvCQ.init(sock, iocp); }
		globalSendBuffer.init(sock, SafeInt<DWORD>(GlobalSendBufferSize));

		std::vector<rio::RioSock> sendSockets;
		sendSockets.reserve(ClientAddress.rangeAddresses());
		globalReceiveBuffer.init(sock, SafeInt<DWORD>(sendSockets.capacity() * SendPerAddress * (512 + 64)));

		for (decltype(sendSockets.capacity()) i{ 0 }; i < sendSockets.capacity(); i++) {
			sendSockets.emplace_back(std::move(rio::RioSock()));
		}

		RIO_BUF udpData{ globalSendBuffer.append(&DomainNameSystem::udpMsgData, sizeof(DomainNameSystem::udpMsgData)) };
		RIO_BUF udpRemote{ globalSendBuffer.appendAddress(ServerAddress, ServerAddress.port) };

		uint32_t rangeOffset = 0;
		for (auto &sendSock : sendSockets) {
			sendSock.init(ClientAddress.rangeOffset(rangeOffset++), sendCQs[rangeOffset % sendCQs.size()].completion, recvCQs[rangeOffset % recvCQs.size()].completion, 16, 16, &sendSock);
		}

		std::vector<rio::SendExRequest> ser;
		std::vector<rio::ReceiveExRequest> rer;
		ser.reserve(sendSockets.size() * SendPerAddress);
		rer.reserve(sendSockets.size() * SendPerAddress);

		for (size_t rep = 0; rep < SendPerAddress; rep++) {
			for (auto &sendSock : sendSockets) {
				sendSock.queueSendEx(ser, udpData, udpRemote);

				//sendSock.queueReceiveEx(rer, dataBuf, addrBuf);
				sendSock.queueReceiveEx(rer, globalReceiveBuffer.append(nullptr, 512), globalReceiveBuffer.append(nullptr, 32), globalReceiveBuffer.append(nullptr, 32));
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


		//rio::SendExRequest::sendAll(ser);
		rio::SendExRequest::deferSendAll(ser);
		rio::ReceiveExRequest::deferReceiveAll(rer);
		for (auto &sendSock : sendSockets) {
			sendSock.commitSend();
			sendSock.commitReceive();
		}
		for (auto &sendCQ : sendCQs) {
			sendCQ.notify();
		}

		for (auto &recvCQ : recvCQs) {
			recvCQ.notify();
		}

		startTick = SafeInt<__int64>(GetTickCount64());
		finishTick = startTick + 1000 * RunTimeSecond;

		ThreadVector::runThreads(1000, []() {
			//std::cout << sendStats.statistics() << std::endl;
			__int64 capturedSendCount = GlobalSendCompletionCount;
			__int64 capturedReceiveCount = GlobalReceiveCompletionCount;
			__int64 inFlight{ (GlobalReceiveCount + GlobalSendCount) - GlobalIncompletionCount - GlobalSendCompletionCount - GlobalReceiveCompletionCount };
#if 0
			std::cout
				<< "pending(" << GlobalSendCount << ", " << GlobalReceiveCount << ") "
				<< "err(" << GlobalIncompletionCount << ") "
				<< "completed(" << GlobalSendCompletionCount << ", " << GlobalReceiveCompletionCount << ") "
				<< "inflight(" << inFlight << ") "
				<< "tx*: " << (capturedSendCount - GlobalSendAckCount) << "pps, " << ((capturedSendCount - GlobalSendAckCount) * sizeof(DomainNameSystem::udpMsgData)) / 125000 << "Mbps "
				<< "rx*: " << (capturedReceiveCount - GlobalReceiveAckCount) << "pps, " << ((capturedReceiveCount - GlobalReceiveAckCount) * sizeof(DomainNameSystem::udpMsgData)) / 125000 << "Mbps"
				<< std::endl;
#else
			std::cout << "{"
				<< "\"pending\": [" << GlobalSendCount << ", " << GlobalReceiveCount << "], "
				<< "\"err\":" << GlobalIncompletionCount << ", "
				<< "\"completed\": [" << GlobalSendCompletionCount << ", " << GlobalReceiveCompletionCount << "], "
				<< "\"inflight\": " << inFlight << ", "
				<< "\"tx_pps\": " << (capturedSendCount - GlobalSendAckCount) << ", \"tx_Mbps\": " << ((capturedSendCount - GlobalSendAckCount) * sizeof(DomainNameSystem::udpMsgData)) / 125000 << ", "
				<< "\"rx_pps\": " << (capturedReceiveCount - GlobalReceiveAckCount) << ", \"rx_Mbps\": " << ((capturedReceiveCount - GlobalReceiveAckCount) * sizeof(DomainNameSystem::udpMsgData)) / 125000 << "},"
				<< std::endl;

#endif

			GlobalSendAckCount = capturedSendCount;
			GlobalReceiveAckCount = capturedReceiveCount;
			//InterlockedAdd64(&GlobalCompletionCount, -capturedCount);
		});
		printf("null]");
	}
};

void parseCommandLine() {

	ServerAddress = cmd.argMap[L"server"];
	ClientAddress = cmd.argMap[L"client"];

	GlobalSendBufferSize = cmd.argMap.count(L"GlobalSendBufferSize") ?
		std::stoull(cmd.argMap[L"GlobalSendBufferSize"]) :
		GlobalSendBufferSize;

	SendPerAddress = cmd.argMap.count(L"SendPerAddress") ?
		std::stoull(cmd.argMap[L"SendPerAddress"]) :
		SendPerAddress;

	RunTimeSecond = cmd.argMap.count(L"RunTimeSecond") ?
		std::stoull(cmd.argMap[L"RunTimeSecond"]) :
		RunTimeSecond;

	NumSendCQs = cmd.argMap.count(L"NumSendCQs") ?
		std::stoull(cmd.argMap[L"NumSendCQs"]) :
		NumSendCQs;

	NumReceiveCQs = cmd.argMap.count(L"NumReceiveCQs") ?
		std::stoull(cmd.argMap[L"NumReceiveCQs"]) :
		NumReceiveCQs;

}

int main()
{
	parseCommandLine();

	Sockets::Win10SocketLib win10SocketLib;
	startTest();

	return 0;
}
