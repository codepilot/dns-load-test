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
			if (completion) {
				RIOCloseCompletionQueue(completion);
			}
		}
		std::vector<RIORESULT> dequeue() {
			std::vector<RIORESULT> ret;
			ret.resize(1024);
			ret.resize(RIODequeueCompletion(completion, ret.data(), SafeInt<ULONG>(ret.capacity())));
			return ret;
		}
	};
}