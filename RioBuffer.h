#pragma once

class Buffer {
public:
	RIO_BUFFERID bufid;
	LPFN_RIODEREGISTERBUFFER RIODeregisterBuffer;
	LPFN_RIOREGISTERBUFFER RIORegisterBuffer;
	LPVOID buf;
	size_t bufferSize;

	void init(Sockets::GenericWin10Socket &sock, DWORD DataLength) {
		RIORegisterBuffer = sock.RIORegisterBuffer;
		RIODeregisterBuffer = sock.RIODeregisterBuffer;
		buf = VirtualAlloc(nullptr, DataLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		std::vector<MEMORY_BASIC_INFORMATION> mbi{ 1 };
		const auto numMbiBytes = VirtualQuery(buf, mbi.data(), mbi.size() * sizeof(MEMORY_BASIC_INFORMATION));
		if (!numMbiBytes) {
			ErrorFormatMessage::exGetLastError();
		}
		mbi.resize(numMbiBytes / sizeof(MEMORY_BASIC_INFORMATION));
		bufid = RIORegisterBuffer(reinterpret_cast<PCHAR>(mbi[0].BaseAddress), SafeInt<DWORD>(mbi[0].RegionSize));
	}
	~Buffer() {
		RIODeregisterBuffer(bufid);
		VirtualFree(buf, 0, MEM_RELEASE);
	}
};
