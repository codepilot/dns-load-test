// dns-load-test.cpp : Defines the entry point for the console application.
//

#define MainCPP

#include "Platform.h"

std::vector<std::wstring> CommandLineVector;
std::unordered_set<std::wstring> CommandLineSet;
std::unordered_map<std::wstring, std::wstring> CommandLineMap;


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
		for (decltype(sendSockets.capacity()) i{ 0 }; i < sendSockets.capacity(); i++) {
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

		HANDLE MainThread = UserHandle::Handle::duplicate(GetCurrentThread());

		for (auto &sendRequest : ser) {
			sendRequest.AccumFunc = [](ULONG_PTR Parameter) {
				sendStats.addSample(Parameter);
			};//AccumSendTicks;
			sendRequest.AccumSize = 128;
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

std::vector<std::wstring> GetCommandLineArgs() {
	int nArgs;
	const auto szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	std::vector<std::wstring> cmdLine(nArgs);
	for (decltype(cmdLine.size()) i{ 0 }; i < cmdLine.size(); i++) {
		cmdLine[i] = szArglist[i];
	}
	LocalFree(szArglist);
	return cmdLine;
}

auto ArgsToSet() {
	std::unordered_set<std::wstring> ret;
	int nArgs{ 0 };
	const auto szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	std::vector<std::wstring> cmdLine(nArgs);
	for (decltype(cmdLine.size()) i{ 0 }; i < cmdLine.size(); i++) {
		std::wstring arg{ szArglist[i] };
		ret.emplace(arg);
	}
	return ret;
}

auto ArgsToMap() {
	std::unordered_map<std::wstring, std::wstring> ret;

	int nArgs{ 0 };
	const auto szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	std::vector<std::wstring> cmdLine(nArgs);
	for (decltype(cmdLine.size()) i{ 1 }; i < cmdLine.size(); i++) {
		std::wstring arg{ szArglist[i] };
		const auto found{ arg.find_first_of(L"=") };
		if (std::wstring::npos == found) {
			continue;
		}
		ret.emplace(arg.substr(0, found), arg.substr(found + 1));
	}
	LocalFree(szArglist);

	return ret;
}

int main()
{
	CommandLineVector = GetCommandLineArgs();
	CommandLineSet = ArgsToSet();
	CommandLineMap = ArgsToMap();
	Sockets::Win10SocketLib win10SocketLib;
	startTest();

	puts("Press any key to quit");
	return getchar();

}
