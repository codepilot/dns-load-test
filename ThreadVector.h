#pragma once

#include <intrin.h>

namespace ThreadVector {

	DWORD WINAPI DequeueThread(LPVOID lpThreadParameter) {
		for (;;) {
			std::array<OVERLAPPED_ENTRY, 16> entriesArray;
			DWORD numEntriesRemoved{ 0 };
			const auto status = GetQueuedCompletionStatusEx(iocp, entriesArray.data(), SafeInt<ULONG>(entriesArray.size()), &numEntriesRemoved, INFINITE, TRUE);
			__int64 numCompleted = 0;
			for (size_t eidx{ 0 }; eidx < numEntriesRemoved; eidx++) {
				auto &entry = entriesArray[eidx];
				auto results = reinterpret_cast<rio::CompletionQueue*>(entry.lpCompletionKey)->dequeue();
				for (auto result : results) {
					//reinterpret_cast<rio::SendExRequest *>(result.RequestContext)->completed();
					numCompleted++;
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
			InterlockedAdd64(&GlobalCompletionCount, numCompleted);

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
		processorGroups.resize(min(3, GetMaximumProcessorGroupCount()));
		WORD curProcGroup{ 0 };

		for (auto &procGroup : processorGroups) {
			procGroup.resize(min(3, GetMaximumProcessorCount(curProcGroup)));
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
		auto nextTimeout = milliseconds + GetTickCount64();
		for (;;) {
			const auto curTickCount = GetTickCount64();
			const auto waitTime = SafeInt<DWORD>( (curTickCount >= nextTimeout) ? 0 : nextTimeout - curTickCount);
			const auto waitStatus{ WaitForMultipleObjectsEx(SafeInt<DWORD>(allThreads.size()), allThreads.data(), TRUE, waitTime, TRUE) };
			if (waitStatus >= WAIT_OBJECT_0 && waitStatus < (WAIT_OBJECT_0 + allThreads.size())) {
				//state of all specified objects is signaled
				puts("All threads quit");
				return;
			}
			else if (waitStatus >= WAIT_ABANDONED_0 && waitStatus < (WAIT_ABANDONED_0 + allThreads.size())) {
				//state of all specified objects is signaled and at least one of the objects is an abandoned mutex object
				puts("Mutex abandoned ???");
				DebugBreak();
				return;
			}
			else if (WAIT_IO_COMPLETION == waitStatus) {
				//The wait was ended by one or more user-mode asynchronous procedure calls (APC) queued to the thread.
				//puts("APC completed");
				continue;
			}
			else if (WAIT_TIMEOUT == waitStatus) {
				//The time-out interval elapsed, the conditions specified by the bWaitAll parameter were not satisfied, and no completion routines are queued.
				//print out stats
				nextTimeout += milliseconds;
				if (timeoutFunc) { timeoutFunc(); }
				continue;
			}
			else if (WAIT_FAILED == waitStatus) {
				puts("WAIT_FAILED");
				ErrorFormatMessage::exGetLastError();
				break;
			}
			else {
				printf("waitStatus unknown %d\n", waitStatus);
				ErrorFormatMessage::exGetLastError();
				break;
			}
		}
	}

};