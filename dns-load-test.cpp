// dns-load-test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

Sockets::GuidMsTcpIp Sockets::GenericWin10Socket::GUID_WSAID;

namespace {

#include "rio.h"

	const auto SendsPerSocket = 8;
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


	
	HANDLE iocp;
	rio::CompletionQueue sendCQ;
	rio::CompletionQueue recvCQ;
	rio::Buffer buf;

	DWORD WINAPI DequeueThread(LPVOID lpThreadParameter) {
		for (;;) {
			std::vector<OVERLAPPED_ENTRY> entries;
			entries.resize(16);
			DWORD numEntriesRemoved{ 0 };
			const auto status = GetQueuedCompletionStatusEx(iocp, entries.data(), SafeInt<ULONG>(entries.capacity()), &numEntriesRemoved, INFINITE, TRUE);
			entries.resize(numEntriesRemoved);
			for (auto &entry : entries) {
				auto results = reinterpret_cast<rio::CompletionQueue*>(entry.lpCompletionKey)->dequeue();
				for (auto result : results) {
					reinterpret_cast<rio::SendExRequest *>(result.RequestContext)->completed();
					if (result.Status != 0 || result.BytesTransferred == 0) {
						printf("result status: %d, bytes: %d, socket: %p, request: %p\n",
							result.Status,
							result.BytesTransferred,
							reinterpret_cast<LPVOID>(result.SocketContext),
							reinterpret_cast<LPVOID>(result.RequestContext));
					}
					reinterpret_cast<rio::SendExRequest *>(result.RequestContext)->send();
					reinterpret_cast<rio::RioSock *>(result.SocketContext)->sock.RIONotify(reinterpret_cast<rio::CompletionQueue*>(entry.lpCompletionKey)->completion);
				}
			}
		}
		return 0;
	}

	void runThreads() {
		std::vector<HANDLE> allThreads;
		std::vector<std::vector<HANDLE>> processorGroups;
		processorGroups.resize(GetMaximumProcessorGroupCount());
		WORD curProcGroup{ 0 };

		for (auto &procGroup : processorGroups) {
			procGroup.resize(GetMaximumProcessorCount(curProcGroup));
			BYTE curProc{ 0 };
			for (auto &proc : procGroup) {
				proc = CreateThread(nullptr, 0, DequeueThread, 0, STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, nullptr);
				PROCESSOR_NUMBER procNum{ curProcGroup, curProc, 0 };
				SetThreadIdealProcessorEx(proc, &procNum, nullptr);
				ResumeThread(proc);
				curProc++;
				allThreads.push_back(proc);
			}
			curProcGroup++;
		}

		puts("threads running");
		WaitForMultipleObjects(SafeInt<DWORD>(allThreads.size()), allThreads.data(), TRUE, INFINITE);
		for (auto thread : allThreads) {
			CloseHandle(thread);
		}
	}

	void startTest() {
		Sockets::GenericWin10Socket sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO);
		static Sockets::GuidMsTcpIp guidMsTcpIP;
		iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
		Sockets::MsSockFuncPtrs funcPtrs(sock.getSocket(), guidMsTcpIP);
		sendCQ.init(sock, iocp);
		recvCQ.init(sock, iocp);
		buf.init(sock, 4096 * 1024);

		std::vector<rio::RioSock> sendSockets;
		sendSockets.reserve(16);
		for (int i = 0; i < sendSockets.capacity(); i++) {
			sendSockets.emplace_back(std::move(rio::RioSock()));
		}

		RIO_BUF udpData{ buf.bufid , 0, sizeof(udpMsgData) };
		SOCKADDR_INET remoteSocket{ AF_INET , htons(53), {10, 2, 0, 4} };
		RIO_BUF udpRemote{ buf.bufid , 1024, sizeof(remoteSocket) };
		RtlCopyMemory(reinterpret_cast<uint8_t *>(buf.buf) + 1024, &remoteSocket, sizeof(remoteSocket));
		RtlCopyMemory(reinterpret_cast<uint8_t *>(buf.buf) + 0, udpMsgData, sizeof(udpMsgData));

		sockaddr_in LocalAddr{ AF_INET , htons(0),{ 10, 0, 0, 3 } };
		for (auto &sendSock : sendSockets) {
			sendSock.init(sendCQ, recvCQ, 16, 16, &sendSock);
			auto rc = bind(sendSock.sock.getSocket(), (struct sockaddr *) &LocalAddr, sizeof(LocalAddr));
			if (rc != 0) {
				printf("bind: %d\n", rc);
			}
		}

		std::vector<rio::SendExRequest> ser;
		ser.reserve(sendSockets.size() * SendsPerSocket);
		for (auto &sendSock : sendSockets) {
			for (int i = 0; i < SendsPerSocket; i++) {
				sendSock.queueSendEx(ser, &udpData, &udpRemote);
			}
		}

		rio::SendExRequest::sendAll(ser);
		sock.RIONotify(sendCQ.completion);

		runThreads();
		printf("test");
		CloseHandle(iocp);
		//sock.RIOSendEx(cq.completion,)
	}



};

int main()
{
	Sockets::Win10SocketLib win10SocketLib;
	startTest();

	return 0;
}
