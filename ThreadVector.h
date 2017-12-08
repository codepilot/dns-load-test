#pragma once

namespace ThreadVector {

	DWORD WINAPI DequeueThread(LPVOID lpThreadParameter) {
		//InputOutputCompletionPort::IOCP *iocp{ reinterpret_cast<decltype(iocp)>(lpThreadParameter) };
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

	class Thread {
	public:
		DWORD id{ 0 };
		UserHandle::Handle handle{ nullptr };
		Thread() : id{ 0 }, handle{ nullptr } { }
		Thread(const Thread &src) : id{ src.id }, handle{ handle } { } //copy
		Thread(Thread&& src) noexcept: id{ std::move(src.id) }, handle{ std::move(handle) } { // move constructor
			src.clear();
		}

		Thread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags = STACK_SIZE_PARAM_IS_A_RESERVATION) {
			
		}
		void clear() {
			SecureZeroMemory(this, sizeof(*this));
		}
		void create(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags = STACK_SIZE_PARAM_IS_A_RESERVATION) {
			handle.handle = CreateThread(nullptr, 0, lpStartAddress, lpParameter, dwCreationFlags, &id);
		}
		~Thread() {
			WaitForSingleObject(handle, INFINITE);
		}
		operator HANDLE() { return handle; }
		BOOL setThreadIdealProcessorEx(_In_ PPROCESSOR_NUMBER lpIdealProcessor) {
			return SetThreadIdealProcessorEx(handle, lpIdealProcessor, nullptr);
		}
		DWORD resumeThread() { return ResumeThread(handle); }
		DWORD suspendThread() { return SuspendThread(handle); }
	};
	typedef VOID (__stdcall *TimeoutFunc)();

	void runThreads(DWORD milliseconds = INFINITE, TimeoutFunc timeoutFunc=nullptr) {
		std::vector<HANDLE> allThreads;
		std::vector<std::vector<Thread>> processorGroups;
		processorGroups.resize(min(1, GetMaximumProcessorGroupCount()));
		WORD curProcGroup{ 0 };

		for (auto &procGroup : processorGroups) {
			procGroup.resize(min(1, GetMaximumProcessorCount(curProcGroup)));
			BYTE curProc{ 0 };
			for (auto &proc : procGroup) {
				proc.create(DequeueThread, iocp, STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED);
				allThreads.push_back(proc);
				PROCESSOR_NUMBER procNum{ curProcGroup, curProc, 0 };
				proc.setThreadIdealProcessorEx(&procNum);
				proc.resumeThread();
				curProc++;
			}
			curProcGroup++;
		}

		puts("threads running");
		for (;;) {
			const auto waitStatus{ WaitForMultipleObjectsEx(SafeInt<DWORD>(allThreads.size()), allThreads.data(), TRUE, milliseconds, TRUE) };
			switch (waitStatus) {
			case WAIT_OBJECT_0 + 0:
			case WAIT_OBJECT_0 + 1:
			case WAIT_OBJECT_0 + 2:
			case WAIT_OBJECT_0 + 3:
			case WAIT_OBJECT_0 + 4:
			case WAIT_OBJECT_0 + 5:
			case WAIT_OBJECT_0 + 6:
			case WAIT_OBJECT_0 + 7:
			case WAIT_OBJECT_0 + 8:
			case WAIT_OBJECT_0 + 9:
			case WAIT_OBJECT_0 + 10:
			case WAIT_OBJECT_0 + 11:
			case WAIT_OBJECT_0 + 12:
			case WAIT_OBJECT_0 + 13:
			case WAIT_OBJECT_0 + 14:
			case WAIT_OBJECT_0 + 15:
			case WAIT_OBJECT_0 + 16:
			case WAIT_OBJECT_0 + 17:
			case WAIT_OBJECT_0 + 18:
			case WAIT_OBJECT_0 + 19:
			case WAIT_OBJECT_0 + 20:
			case WAIT_OBJECT_0 + 21:
			case WAIT_OBJECT_0 + 22:
			case WAIT_OBJECT_0 + 23:
			case WAIT_OBJECT_0 + 24:
			case WAIT_OBJECT_0 + 25:
			case WAIT_OBJECT_0 + 26:
			case WAIT_OBJECT_0 + 27:
			case WAIT_OBJECT_0 + 28:
			case WAIT_OBJECT_0 + 29:
			case WAIT_OBJECT_0 + 30:
			case WAIT_OBJECT_0 + 31:
			case WAIT_OBJECT_0 + 32:
			case WAIT_OBJECT_0 + 33:
			case WAIT_OBJECT_0 + 34:
			case WAIT_OBJECT_0 + 35:
			case WAIT_OBJECT_0 + 36:
			case WAIT_OBJECT_0 + 37:
			case WAIT_OBJECT_0 + 38:
			case WAIT_OBJECT_0 + 39:
			case WAIT_OBJECT_0 + 40:
			case WAIT_OBJECT_0 + 41:
			case WAIT_OBJECT_0 + 42:
			case WAIT_OBJECT_0 + 43:
			case WAIT_OBJECT_0 + 44:
			case WAIT_OBJECT_0 + 45:
			case WAIT_OBJECT_0 + 46:
			case WAIT_OBJECT_0 + 47:
			case WAIT_OBJECT_0 + 48:
			case WAIT_OBJECT_0 + 49:
			case WAIT_OBJECT_0 + 50:
			case WAIT_OBJECT_0 + 51:
			case WAIT_OBJECT_0 + 52:
			case WAIT_OBJECT_0 + 53:
			case WAIT_OBJECT_0 + 54:
			case WAIT_OBJECT_0 + 55:
			case WAIT_OBJECT_0 + 56:
			case WAIT_OBJECT_0 + 57:
			case WAIT_OBJECT_0 + 58:
			case WAIT_OBJECT_0 + 59:
			case WAIT_OBJECT_0 + 60:
			case WAIT_OBJECT_0 + 61:
			case WAIT_OBJECT_0 + 62:
			case WAIT_OBJECT_0 + 63:
				//state of all specified objects is signaled
				puts("All threads quit");
				return;

			case WAIT_ABANDONED_0 + 0:
			case WAIT_ABANDONED_0 + 1:
			case WAIT_ABANDONED_0 + 2:
			case WAIT_ABANDONED_0 + 3:
			case WAIT_ABANDONED_0 + 4:
			case WAIT_ABANDONED_0 + 5:
			case WAIT_ABANDONED_0 + 6:
			case WAIT_ABANDONED_0 + 7:
			case WAIT_ABANDONED_0 + 8:
			case WAIT_ABANDONED_0 + 9:
			case WAIT_ABANDONED_0 + 10:
			case WAIT_ABANDONED_0 + 11:
			case WAIT_ABANDONED_0 + 12:
			case WAIT_ABANDONED_0 + 13:
			case WAIT_ABANDONED_0 + 14:
			case WAIT_ABANDONED_0 + 15:
			case WAIT_ABANDONED_0 + 16:
			case WAIT_ABANDONED_0 + 17:
			case WAIT_ABANDONED_0 + 18:
			case WAIT_ABANDONED_0 + 19:
			case WAIT_ABANDONED_0 + 20:
			case WAIT_ABANDONED_0 + 21:
			case WAIT_ABANDONED_0 + 22:
			case WAIT_ABANDONED_0 + 23:
			case WAIT_ABANDONED_0 + 24:
			case WAIT_ABANDONED_0 + 25:
			case WAIT_ABANDONED_0 + 26:
			case WAIT_ABANDONED_0 + 27:
			case WAIT_ABANDONED_0 + 28:
			case WAIT_ABANDONED_0 + 29:
			case WAIT_ABANDONED_0 + 30:
			case WAIT_ABANDONED_0 + 31:
			case WAIT_ABANDONED_0 + 32:
			case WAIT_ABANDONED_0 + 33:
			case WAIT_ABANDONED_0 + 34:
			case WAIT_ABANDONED_0 + 35:
			case WAIT_ABANDONED_0 + 36:
			case WAIT_ABANDONED_0 + 37:
			case WAIT_ABANDONED_0 + 38:
			case WAIT_ABANDONED_0 + 39:
			case WAIT_ABANDONED_0 + 40:
			case WAIT_ABANDONED_0 + 41:
			case WAIT_ABANDONED_0 + 42:
			case WAIT_ABANDONED_0 + 43:
			case WAIT_ABANDONED_0 + 44:
			case WAIT_ABANDONED_0 + 45:
			case WAIT_ABANDONED_0 + 46:
			case WAIT_ABANDONED_0 + 47:
			case WAIT_ABANDONED_0 + 48:
			case WAIT_ABANDONED_0 + 49:
			case WAIT_ABANDONED_0 + 50:
			case WAIT_ABANDONED_0 + 51:
			case WAIT_ABANDONED_0 + 52:
			case WAIT_ABANDONED_0 + 53:
			case WAIT_ABANDONED_0 + 54:
			case WAIT_ABANDONED_0 + 55:
			case WAIT_ABANDONED_0 + 56:
			case WAIT_ABANDONED_0 + 57:
			case WAIT_ABANDONED_0 + 58:
			case WAIT_ABANDONED_0 + 59:
			case WAIT_ABANDONED_0 + 60:
			case WAIT_ABANDONED_0 + 61:
			case WAIT_ABANDONED_0 + 62:
			case WAIT_ABANDONED_0 + 63:
				//state of all specified objects is signaled and at least one of the objects is an abandoned mutex object
				puts("Mutex abandoned ???");
				DebugBreak();
				return;

			case WAIT_IO_COMPLETION:
				//The wait was ended by one or more user-mode asynchronous procedure calls (APC) queued to the thread.
				//puts("APC completed");
				continue;

			case WAIT_TIMEOUT:
				//The time-out interval elapsed, the conditions specified by the bWaitAll parameter were not satisfied, and no completion routines are queued.
				//print out stats
				if (timeoutFunc) { timeoutFunc(); }
				continue;

			case WAIT_FAILED:
			default:
				puts("WAIT_FAILED");
				ErrorFormatMessage::exGetLastError();
				break;
			}
		}
	}

};