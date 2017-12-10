// dns-load-test.cpp : Defines the entry point for the console application.
//

#define MainCPP

#include "Platform.h"

size_t MaxSendCount = 100000ui64;

__declspec(align(64)) __int64 volatile GlobalSendCount{ 0 };
__declspec(align(64)) __int64 volatile GlobalReceiveCount{ 0 };

__declspec(align(64)) __int64 volatile GlobalSendCompletionCount{ 0 };
__declspec(align(64)) __int64 volatile GlobalReceiveCompletionCount{ 0 };

__declspec(align(64)) __int64 volatile GlobalSendAckCount{ 0 };
__declspec(align(64)) __int64 volatile GlobalReceiveAckCount{ 0 };

__declspec(align(64)) __int64 volatile GlobalIncompletionCount{ 0 };
__declspec(align(64)) __int64 volatile DnsRequestCounter{ 0 };

LPVOID globalSendBufferBuf{ nullptr };

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
	size_t RecvPerAddress = 1; //default
	size_t RunTimeSecond = 1;//default
	size_t NumSendCQs = 16;
	size_t NumReceiveCQs = 16;
	size_t MaxOutstandingReceive = 16;
	size_t MaxOutstandingSend = 16;
	size_t SendCQSize = 8192;
	size_t RecvCQSize = 8192;
	DWORD64 lastDisplayTick = 0;
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

		for (auto &sendCQ : sendCQs) { sendCQ.init(sock, iocp); sendCQ.resize(SafeInt<DWORD>(SendCQSize)); }
		for (auto &recvCQ : recvCQs) { recvCQ.init(sock, iocp); recvCQ.resize(SafeInt<DWORD>(RecvCQSize)); }
		globalSendBuffer.init(sock, SafeInt<DWORD>(GlobalSendBufferSize));
		globalSendBufferBuf = globalSendBuffer.buf;

		std::vector<rio::RioSock> sendSockets;
		sendSockets.reserve(ClientAddress.rangeAddresses());
		globalReceiveBuffer.init(sock, SafeInt<DWORD>(sendSockets.capacity() * RecvPerAddress * (512 + 64)));

		for (decltype(sendSockets.capacity()) i{ 0 }; i < sendSockets.capacity(); i++) {
			sendSockets.emplace_back(std::move(rio::RioSock()));
		}

		RIO_BUF udpRemote{ globalSendBuffer.appendAddress(ServerAddress, ServerAddress.port) };


		uint32_t rangeOffset = 0;
		for (auto &sendSock : sendSockets) {
			sendSock.init(ClientAddress.rangeOffset(rangeOffset++), sendCQs[rangeOffset % sendCQs.size()].completion, recvCQs[rangeOffset % recvCQs.size()].completion, SafeInt<ULONG>(MaxOutstandingReceive), SafeInt<ULONG>(MaxOutstandingSend), &sendSock);
		}

		std::vector<rio::SendExRequest> ser;
		std::vector<rio::ReceiveExRequest> rer;
		ser.reserve(sendSockets.size() * SendPerAddress);
		rer.reserve(sendSockets.size() * RecvPerAddress);

		for (size_t rep = 0; rep < RecvPerAddress; rep++) {
			for (auto &sendSock : sendSockets) {
				sendSock.queueReceiveEx(rer, globalReceiveBuffer.append(nullptr, 512), globalReceiveBuffer.append(nullptr, 32), globalReceiveBuffer.append(nullptr, 32));
			}
		}

		for (size_t rep = 0; rep < SendPerAddress; rep++) {
			for (auto &sendSock : sendSockets) {
				sendSock.queueSendEx(ser, globalSendBuffer.append(&DomainNameSystem::udpMsgData, sizeof(DomainNameSystem::udpMsgData)), udpRemote);
			}
		}

		HANDLE MainThread = UserHandle::Handle::duplicate(GetCurrentThread());

		for (auto &sendRequest : ser) {
			sendRequest.AccumFunc = [](ULONG_PTR Parameter) {
				sendStats.addSample(static_cast<double_t>( Parameter));
			};//AccumSendTicks;
			sendRequest.AccumSize = 65536;
			sendRequest.AccumThread = MainThread;
		}

		rio::ReceiveExRequest::receiveAll(rer);
		rio::SendExRequest::sendAll(ser);

		for (auto &recvCQ : recvCQs) {
			recvCQ.notify();
		}

		for (auto &sendCQ : sendCQs) {
			sendCQ.notify();
		}

		startTick = SafeInt<__int64>(GetTickCount64());
		finishTick = startTick + 1000 * RunTimeSecond;
		lastDisplayTick = startTick;
		ThreadVector::runThreads(1000, []() {
			//std::cout << sendStats.statistics() << std::endl;
			__int64 capturedSendCount = GlobalSendCompletionCount;
			__int64 capturedReceiveCount = GlobalReceiveCompletionCount;
			__int64 inFlight{ (GlobalReceiveCount + GlobalSendCount) - GlobalIncompletionCount - GlobalSendCompletionCount - GlobalReceiveCompletionCount };
			__int64 curTick = SafeInt<__int64>(GetTickCount64());
			double_t intervalTick = static_cast<double_t>( curTick - lastDisplayTick );
			lastDisplayTick = curTick;

			std::cout << "{"
				<< "\"pending\": [" << GlobalSendCount << ", " << GlobalReceiveCount << "], "
				<< "\"err\":" << GlobalIncompletionCount << ", "
				<< "\"completed\": [" << GlobalSendCompletionCount << ", " << GlobalReceiveCompletionCount << "], "
				<< "\"inflight\": " << inFlight << ", "
				<< "\"tx_pps\": " << (capturedSendCount - GlobalSendAckCount) << ", \"tx_Mbps\": " << ((capturedSendCount - GlobalSendAckCount) * sizeof(DomainNameSystem::udpMsgData)) / (intervalTick * 125.000) << ", "
				<< "\"rx_pps\": " << (capturedReceiveCount - GlobalReceiveAckCount) << ", \"rx_Mbps\": " << ((capturedReceiveCount - GlobalReceiveAckCount) * sizeof(DomainNameSystem::udpMsgData)) / (intervalTick * 125.000) << "},"
				<< std::endl << std::flush;

			GlobalSendAckCount = capturedSendCount;
			GlobalReceiveAckCount = capturedReceiveCount;
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

	RunTimeSecond = cmd.argMap.count(L"RunTimeSecond") ?
		std::stoull(cmd.argMap[L"RunTimeSecond"]) :
		RunTimeSecond;

	NumSendCQs = cmd.argMap.count(L"NumSendCQs") ?
		std::stoull(cmd.argMap[L"NumSendCQs"]) :
		NumSendCQs;

	NumReceiveCQs = cmd.argMap.count(L"NumReceiveCQs") ?
		std::stoull(cmd.argMap[L"NumReceiveCQs"]) :
		NumReceiveCQs;

	SendPerAddress = cmd.argMap.count(L"SendPerAddress") ?
		std::stoull(cmd.argMap[L"SendPerAddress"]) :
		SendPerAddress;

	RecvPerAddress = SendPerAddress * 2;

	MaxSendCount = cmd.argMap.count(L"MaxSendCount") ?
		std::stoull(cmd.argMap[L"MaxSendCount"]) :
		MaxSendCount;

	MaxOutstandingSend = SendPerAddress;
	MaxOutstandingReceive = RecvPerAddress;
	SendCQSize = static_cast<size_t>(ceil(static_cast<double_t>(ClientAddress.rangeAddresses()) / static_cast<double_t>(NumSendCQs))) * SendPerAddress;
	RecvCQSize = static_cast<size_t>(ceil(static_cast<double_t>(ClientAddress.rangeAddresses()) / static_cast<double_t>(NumReceiveCQs))) * RecvPerAddress;
	GlobalSendBufferSize = 64 + ClientAddress.rangeAddresses() * SendPerAddress * sizeof(DomainNameSystem::udpMsgData);
}

int main()
{
	parseCommandLine();
	SetPriorityClass(GetCurrentThread(), REALTIME_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	Sockets::Win10SocketLib win10SocketLib;
	startTest();

	return 0;
}
