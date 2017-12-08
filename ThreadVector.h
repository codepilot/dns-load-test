#pragma once

namespace ThreadVector {

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

};