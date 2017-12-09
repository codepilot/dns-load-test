#pragma once

namespace rio {
	class Buffer {
	public:
		RIO_BUFFERID bufid;
		LPFN_RIODEREGISTERBUFFER RIODeregisterBuffer;
		LPFN_RIOREGISTERBUFFER RIORegisterBuffer;
		LPVOID buf;
		size_t bufferSize;
		size_t freeOffset{ 0 };

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
			bufferSize = mbi[0].RegionSize;
			bufid = RIORegisterBuffer(reinterpret_cast<PCHAR>(mbi[0].BaseAddress), SafeInt<DWORD>(mbi[0].RegionSize));
		}

		~Buffer() {
			if (bufid) {
				RIODeregisterBuffer(bufid);
			}
			if (buf) {
				VirtualFree(buf, 0, MEM_RELEASE);
			}
		}

		size_t linearAllocate(size_t byteCount) {
			if (freeOffset + byteCount > bufferSize) {
				OutputDebugString(L"freeOffset + byteCount > bufferSize");
				DebugBreak();
			}
			size_t ret{ freeOffset };
			freeOffset += byteCount;
			return ret;
		}

		RIO_BUF append(LPCVOID src, size_t byteCount) {
			size_t retOffset = linearAllocate(byteCount);
			if (src) {
				CopyMemory(reinterpret_cast<uint8_t *>(buf) + retOffset, src, byteCount);
			}
			return { bufid, SafeInt<ULONG>(retOffset), SafeInt<ULONG>(byteCount) };
		}

		RIO_BUF appendAddress(uint32_t IPV4, uint16_t Port) {
			SOCKADDR_INET Socket; SecureZeroMemory(&Socket, sizeof(Socket));
			Socket.Ipv4.sin_family = Socket.si_family = AF_INET;
			Socket.Ipv4.sin_port = htons(Port);
			Socket.Ipv4.sin_addr.S_un.S_addr = htonl(IPV4);
			return append(&Socket, sizeof(Socket));
		}
	};
}