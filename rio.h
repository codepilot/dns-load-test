#pragma once

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
			ret.resize(RIODequeueCompletion(completion, ret.data(), SafeInt<ULONG>(ret.capacity())));
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
		LARGE_INTEGER PerfFreq{ 0 };
		LARGE_INTEGER PerfCountStart{ 0 };
		LARGE_INTEGER PerfCountCompleted{ 0 };
		double_t freqToMicroseconds{ 0 };
		std::vector<double_t> completionTimes;

		SendExRequest(LPFN_RIOSENDEX _RIOSendEx, RIO_RQ _requests, PRIO_BUF _pData, PRIO_BUF _pRemoteAddress) {
			RIOSendEx = _RIOSendEx;
			requests = _requests;
			pData = _pData;
			pRemoteAddress = _pRemoteAddress;
			QueryPerformanceFrequency(&PerfFreq);
			freqToMicroseconds = 1e6 / PerfFreq.QuadPart;
		}

		static void sendAll(std::vector<SendExRequest> &ser) {
			for (auto &n : ser) {
				n.send();
			}
		}

		static double_t sum(std::vector<double_t> dv) {
			double_t sum{ 0 };
			for (auto &n : dv) {
				sum += n;
			}
			return sum;
		}

		static double_t avg(std::vector<double_t> dv) {
			return sum(dv) / static_cast<double_t>(dv.size());
		}

		void completed() {
			QueryPerformanceCounter(&PerfCountCompleted);
			const auto timeTaken{ PerfCountCompleted.QuadPart - PerfCountStart.QuadPart };
			completionTimes.push_back(static_cast<double_t>(timeTaken) * freqToMicroseconds);
			if (completionTimes.size() >= 1000) {
				const auto avgMicroSeconds = avg(completionTimes);
				printf("timeTaken(%I64u): %fus, %fMbps\n", completionTimes.size(), avgMicroSeconds, static_cast<double_t>(pData->Length) * 8.0 / avgMicroSeconds);
				completionTimes.resize(0);
			}

		}

		void send() {
			QueryPerformanceCounter(&PerfCountStart);
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

		RioSock() :sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO) {

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
				pData ? 1 : 0,
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
