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

	void runThreads() {
		std::vector<std::vector<Thread>> processorGroups;
		processorGroups.resize(GetMaximumProcessorGroupCount());
		WORD curProcGroup{ 0 };

		for (auto &procGroup : processorGroups) {
			procGroup.resize(GetMaximumProcessorCount(curProcGroup));
			BYTE curProc{ 0 };
			for (auto &proc : procGroup) {
				proc.create(DequeueThread, iocp, STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED);
				PROCESSOR_NUMBER procNum{ curProcGroup, curProc, 0 };
				proc.setThreadIdealProcessorEx(&procNum);
				proc.resumeThread();
				curProc++;
			}
			curProcGroup++;
		}

		puts("threads running");
		//WaitForMultipleObjects(SafeInt<DWORD>(allThreads.size()), allThreads.data(), TRUE, INFINITE);
		//for (auto thread : allThreads) {
//			CloseHandle(thread);
	//	}
	}

};