#pragma once

namespace rio {
	template<DWORD QueueSize> class CompletionQueue {
	public:
		RIO_CQ completion;
		OVERLAPPED overlapped;
		LPFN_RIOCLOSECOMPLETIONQUEUE RIOCloseCompletionQueue;
		LPFN_RIOCREATECOMPLETIONQUEUE RIOCreateCompletionQueue;
		LPFN_RIORESIZECOMPLETIONQUEUE RIOResizeCompletionQueue;
		LPFN_RIODEQUEUECOMPLETION RIODequeueCompletion;
		LPFN_RIONOTIFY RIONotify;
		CRITICAL_SECTION GlobalCriticalSection;

		void init(Sockets::GenericWin10Socket &sock, HANDLE iocp) {
			InitializeCriticalSectionAndSpinCount(&GlobalCriticalSection, 0x00000400);

			RIOCreateCompletionQueue = sock.RIOCreateCompletionQueue;
			RIOCloseCompletionQueue = sock.RIOCloseCompletionQueue;
			RIODequeueCompletion = sock.RIODequeueCompletion;
			RIOResizeCompletionQueue = sock.RIOResizeCompletionQueue;
			RIONotify = sock.RIONotify;

			RIO_NOTIFICATION_COMPLETION rioComp;
			SecureZeroMemory(&rioComp, sizeof(rioComp));
			SecureZeroMemory(&overlapped, sizeof(overlapped));
			rioComp.Type = RIO_IOCP_COMPLETION;
			rioComp.Iocp.IocpHandle = iocp;
			rioComp.Iocp.Overlapped = &overlapped;
			rioComp.Iocp.CompletionKey = this;
			completion = RIOCreateCompletionQueue(QueueSize, &rioComp);
		}
		~CompletionQueue() {
			if (completion) {
				RIOCloseCompletionQueue(completion);
			}
		}
		//void enter() {
		//	EnterCriticalSection(&GlobalCriticalSection);
		//}
		//void leave() {
		//	LeaveCriticalSection(&GlobalCriticalSection);
		//}
		void notify() {
			EnterCriticalSection(&GlobalCriticalSection);
			RIONotify(completion);
			LeaveCriticalSection(&GlobalCriticalSection);
		}
		std::vector<RIORESULT> dequeue() {
			//std::vector<RIORESULT> retVector;
			std::array<RIORESULT, 16> ret;

			EnterCriticalSection(&GlobalCriticalSection);
			//for(;;) {
				const auto count = RIODequeueCompletion(completion, ret.data(), SafeInt<ULONG>(ret.size()));
				//if (!count) { break; }
				//std::copy(ret.begin(), ret.begin() + count, std::back_inserter(retVector));
			//}
			RIONotify(completion);
			LeaveCriticalSection(&GlobalCriticalSection);
			return std::vector<RIORESULT>{ret.begin(), ret.begin() + count};
		}
	};
}