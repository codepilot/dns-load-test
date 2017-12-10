#pragma once

#include <intrin.h>

namespace ThreadVector {

	DWORD WINAPI DequeueThread(LPVOID lpThreadParameter) {
		for (;;) {
			__int64 remainingTicks = finishTick - SafeInt<__int64>(GetTickCount64());
			if (remainingTicks <= 0) { break; }
			std::array<OVERLAPPED_ENTRY, 16> entriesArray;
			DWORD numEntriesRemoved{ 0 };
			const auto status = GetQueuedCompletionStatusEx(iocp, entriesArray.data(), SafeInt<ULONG>(entriesArray.size()), &numEntriesRemoved, SafeInt<DWORD>(remainingTicks), TRUE);
			if (!status) {
				if (WAIT_TIMEOUT == GetLastError()) { break; }
				ErrorFormatMessage::exGetLastError();
				std::cout << "iocp status: " << status << std::endl;
				continue;
			}

			for (size_t eidx{ 0 }; eidx < numEntriesRemoved; eidx++) {
				auto &entry = entriesArray[eidx];
				auto cq = reinterpret_cast<rio::CompletionQueue<1>*>(entry.lpCompletionKey);
				for (;;) {
					auto results = cq->dequeue();
					if (!results.size()) { break; }

					__int64 numSendCompleted = 0;
					__int64 numRecvCompleted = 0;

					for (auto result : results) {
						const auto request = reinterpret_cast<rio::ExRequest *>(result.RequestContext);
						if (result.Status != 0 || result.BytesTransferred == 0) {
							printf("%s status: %d, bytes: %d, socket: %p, request: %p\n",
								request->isSend ? "Send" : "Recv",
								result.Status,
								result.BytesTransferred,
								reinterpret_cast<LPVOID>(result.SocketContext),
								reinterpret_cast<LPVOID>(result.RequestContext));
						}

						if (request->isSend) {
							numSendCompleted++;
							const auto sendRequest{ reinterpret_cast<rio::SendExRequest *>(request) };
							sendRequest->send();
							
						}
						else {
							const auto receiveRequest{ reinterpret_cast<rio::ReceiveExRequest *>(request) };
							std::vector<uint8_t> recvData{
								reinterpret_cast<uint8_t *>(globalReceiveBuffer.buf) + receiveRequest->pData.Offset,
								reinterpret_cast<uint8_t *>(globalReceiveBuffer.buf) + receiveRequest->pData.Offset + min(result.BytesTransferred, receiveRequest->pData.Length) };

							if (!DomainNameSystem::doesReplyMatchVector(recvData)) {
								for (const auto &data : recvData) {
									printf("%02x", data);
								}
								puts("");
							}
							else {
								numRecvCompleted++;
							}
							receiveRequest->completed();
							receiveRequest->receive();
						}
					}
					InterlockedAdd64(&GlobalSendCompletionCount, numSendCompleted);
					InterlockedAdd64(&GlobalReceiveCompletionCount, numRecvCompleted);
				}
				cq->notify();
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
		auto setThreadAffinityMask(DWORD_PTR dwThreadAffinityMask) {
			return SetThreadAffinityMask(handle, dwThreadAffinityMask);
		}
		DWORD resumeThread() { return ResumeThread(handle); }
		DWORD suspendThread() { return SuspendThread(handle); }
	};
	typedef VOID (__stdcall *TimeoutFunc)();

	void runThreads(DWORD milliseconds = INFINITE, TimeoutFunc timeoutFunc=nullptr) {
		std::vector<HANDLE> allThreads;
		std::vector<std::vector<Thread>> processorGroups;
		processorGroups.resize((GetMaximumProcessorGroupCount()));
		WORD curProcGroup{ 0 };

		for (auto &procGroup : processorGroups) {
			procGroup.resize(min(8, GetMaximumProcessorCount(curProcGroup)));
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

		puts("[");
		auto nextTimeout = milliseconds + GetTickCount64();
		for (;;) {
			const auto curTickCount = GetTickCount64();
			const auto waitTime = SafeInt<DWORD>( (curTickCount >= nextTimeout) ? 0 : nextTimeout - curTickCount);
			if (!waitTime) {
				nextTimeout += milliseconds;
				if (timeoutFunc) { timeoutFunc(); }
				continue;
			}
			const auto waitStatus{ WaitForMultipleObjectsEx(SafeInt<DWORD>(allThreads.size()), allThreads.data(), TRUE, waitTime, TRUE) };
			if (waitStatus >= WAIT_OBJECT_0 && waitStatus < (WAIT_OBJECT_0 + allThreads.size())) {
				//state of all specified objects is signaled
				//puts("All threads quit");
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