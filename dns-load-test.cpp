// dns-load-test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


Sockets::GuidMsTcpIp Sockets::GenericWin10Socket::GUID_WSAID;

namespace rio {
	class CompletionQueue {
	public:
		RIO_CQ completion;
		OVERLAPPED overlapped;
		LPFN_RIOCLOSECOMPLETIONQUEUE RIOCloseCompletionQueue;
		LPFN_RIOCREATECOMPLETIONQUEUE RIOCreateCompletionQueue;
		LPFN_RIORESIZECOMPLETIONQUEUE RIOResizeCompletionQueue;
		LPFN_RIODEQUEUECOMPLETION RIODequeueCompletion;

		void init(Sockets::GenericWin10Socket &sock, HANDLE iocp) {
			RIOCreateCompletionQueue = sock.RIOCreateCompletionQueue;
			RIOCloseCompletionQueue = sock.RIOCloseCompletionQueue;
			RIODequeueCompletion = sock.RIODequeueCompletion;
			RIOResizeCompletionQueue = sock.RIOResizeCompletionQueue;

			RIO_NOTIFICATION_COMPLETION rioComp;
			SecureZeroMemory(&rioComp, sizeof(rioComp));
			SecureZeroMemory(&overlapped, sizeof(overlapped));
			rioComp.Type = RIO_IOCP_COMPLETION;
			rioComp.Iocp.IocpHandle = iocp;
			rioComp.Iocp.Overlapped = &overlapped;
			rioComp.Iocp.CompletionKey = this;
			completion = RIOCreateCompletionQueue(1024, &rioComp);
		}
		~CompletionQueue() {
			RIOCloseCompletionQueue(completion);
		}
		std::vector<RIORESULT> dequeue() {
			std::vector<RIORESULT> ret;
			ret.resize(1024);
			ret.resize(RIODequeueCompletion(completion, ret.data(), ret.capacity()));
			return ret;
		}
	};

	class RequestQueue {
	public:
		LPFN_RIOCREATEREQUESTQUEUE RIOCreateRequestQueue;
		LPFN_RIORESIZEREQUESTQUEUE RIOResizeRequestQueue;
		RIO_RQ requests;
		void init(Sockets::GenericWin10Socket &sock, rio::CompletionQueue &sendCQ, rio::CompletionQueue &recvCQ, ULONG  MaxOutstandingReceive, ULONG  MaxOutstandingSend, PVOID  SocketContext = nullptr) {
			RIOCreateRequestQueue = sock.RIOCreateRequestQueue;
			RIOResizeRequestQueue = sock.RIOResizeRequestQueue;
			requests = RIOCreateRequestQueue(
				sock.getSocket(),
				MaxOutstandingReceive,
				1,
				MaxOutstandingSend,
				1,
				recvCQ.completion,
				sendCQ.completion,
				SocketContext
			);
			if (!requests) {
				const auto lastError = WSAGetLastError();
				printf("lastError: %d\n", lastError);
			}
		}
	};

	class Buffer {
	public:
		RIO_BUFFERID bufid;
		LPFN_RIODEREGISTERBUFFER RIODeregisterBuffer;
		LPFN_RIOREGISTERBUFFER RIORegisterBuffer;
		LPVOID buf;
		void init(Sockets::GenericWin10Socket &sock, DWORD DataLength) {
			RIORegisterBuffer = sock.RIORegisterBuffer;
			RIODeregisterBuffer = sock.RIODeregisterBuffer;
			buf = VirtualAlloc(nullptr, DataLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			bufid = RIORegisterBuffer(reinterpret_cast<PCHAR>(buf), DataLength);
		}
		~Buffer() {
			RIODeregisterBuffer(bufid);
			VirtualFree(buf, 0, MEM_RELEASE);
		}
	};

	class SendExRequest {
	public:
		LPFN_RIOSENDEX RIOSendEx;
		RIO_RQ requests;
		PRIO_BUF pData;
		PRIO_BUF pRemoteAddress;
		SendExRequest(LPFN_RIOSENDEX _RIOSendEx, RIO_RQ _requests, PRIO_BUF _pData, PRIO_BUF _pRemoteAddress) {
			RIOSendEx = _RIOSendEx;
			requests = _requests;
			pData = _pData;
			pRemoteAddress = _pRemoteAddress;
		}
		static void sendAll(std::vector<SendExRequest> &ser) {
			for (auto &n : ser) {
				n.send();
			}
		}
		void send() {
			auto status = RIOSendEx(
				requests,
				pData,
				pData ? 1 : 0,
				nullptr,
				pRemoteAddress,
				nullptr,
				nullptr,
				0,
				this);
			if (!status || status == -1) {
				const auto lastError = WSAGetLastError();
				printf("RIOSendEx lastError %u\n", lastError);
			}
		}
	};

	class RioSock {
	public:
		Sockets::GenericWin10Socket sock;
		rio::RequestQueue rq;

		RioSock():sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO) {

		}
		void init(rio::CompletionQueue &sendCQ, rio::CompletionQueue &recvCQ, ULONG  MaxOutstandingReceive, ULONG  MaxOutstandingSend, PVOID  SocketContext = nullptr) {
			rq.init(sock, sendCQ, recvCQ, MaxOutstandingReceive, MaxOutstandingSend, this);

		}
		void queueSendEx(std::vector<SendExRequest> &ser, PRIO_BUF pData, PRIO_BUF pRemoteAddress) {
			ser.push_back({ sock.RIOSendEx , rq.requests , pData, pRemoteAddress });
		}
		void sendEx(PRIO_BUF pData, PRIO_BUF pRemoteAddress) {
			auto status = sock.RIOSendEx(
				rq.requests,
				pData,
				pData?1:0,
				nullptr,
				pRemoteAddress,
				nullptr,
				nullptr,
				0,
				nullptr);
			if (!status || status == -1) {
				const auto lastError = WSAGetLastError();
				printf("RIOSendEx lastError %u\n", lastError);
			}
		}
	};
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
		const auto status = GetQueuedCompletionStatusEx(iocp, entries.data(), entries.capacity(), &numEntriesRemoved, INFINITE, TRUE);
		entries.resize(numEntriesRemoved);
		for (auto &entry : entries) {
			auto results = reinterpret_cast<rio::CompletionQueue*>(entry.lpCompletionKey)->dequeue();
			for (auto result : results) {
				if (result.Status != 0 || result.BytesTransferred == 0) {
					printf("result status: %d, bytes: %d, socket: %p, request: %p\n",
						result.Status, result.BytesTransferred, result.SocketContext, result.RequestContext);
				}
				reinterpret_cast<rio::SendExRequest *>(result.RequestContext)->send();
				reinterpret_cast<rio::RioSock *>(result.SocketContext)->sock.RIONotify(reinterpret_cast<rio::CompletionQueue*>(entry.lpCompletionKey)->completion);
			}
		}
	}
	return 0;
}

void runThreads() {
	std::vector<HANDLE> threads;
	for (int i = 0; i < 10; i++) {
		threads.push_back(CreateThread(nullptr, 0, DequeueThread, 0, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr));
	}
	puts("threads running");
	WaitForMultipleObjects(threads.size(), threads.data(), TRUE, INFINITE);
	for (auto thread : threads) {
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
	uint8_t udpMsgData[36]{ 0x02, 0xdc, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x03, 0x6e, 0x73, 0x34, 0x0b, 0x61, 0x72,
		0x63, 0x68, 0x65, 0x6f, 0x66, 0x75, 0x74, 0x75, 0x72,
		0x02, 0x75, 0x73, 0x00, 0x00, 0x1c, 0x00, 0x01 };
	RIO_BUF udpData{ buf.bufid , 0, sizeof(udpMsgData) };
	SOCKADDR_INET remoteSocket{ AF_INET , htons(53), {10, 2, 0, 4} };
	RIO_BUF udpRemote{ buf.bufid , 1024, sizeof(remoteSocket) };
	RtlCopyMemory(reinterpret_cast<uint8_t *>(buf.buf) + 1024, &remoteSocket, sizeof(remoteSocket));
	RtlCopyMemory(reinterpret_cast<uint8_t *>(buf.buf) + 0, udpMsgData, sizeof(udpMsgData));
	WSABUF udpMsg{ sizeof(udpMsgData), reinterpret_cast<char *>(buf.buf) };
	OVERLAPPED ovTest{ 0 };
	sockaddr_in LocalAddr{ AF_INET , htons(0),{ 10, 0, 0, 3 } };
	for (auto &sendSock : sendSockets) {
		sendSock.init(sendCQ, recvCQ, 16, 16, &sendSock);
		auto rc = bind(sendSock.sock.getSocket(), (struct sockaddr *) &LocalAddr, sizeof(LocalAddr));
		if (rc != 0) {
			printf("bind: %d\n", rc);
		}
	}
	std::vector<rio::SendExRequest> ser;
	const auto SendsPerSocket = 8;
	ser.reserve(sendSockets.size() * SendsPerSocket);
	for (auto &sendSock : sendSockets) {
		for (int i = 0; i < SendsPerSocket; i++) {
			//sendSock.sendEx(&udpData, &udpRemote);
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



int main()
{
	Sockets::Win10SocketLib win10SocketLib;
	startTest();

	return 0;
}

