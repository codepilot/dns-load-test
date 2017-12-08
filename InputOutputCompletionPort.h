#pragma once

namespace InputOutputCompletionPort {
	class IOCP {
	public:
		UserHandle::Handle handle;
		IOCP(DWORD NumberOfConcurrentThreads = 0) : handle{ CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, NumberOfConcurrentThreads) } { }
		operator HANDLE() { return handle; }
	};
};