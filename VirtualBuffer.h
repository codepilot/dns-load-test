
#pragma once

class VirtualBuffer {
public:
	LPVOID ptr{nullptr};
	DWORD lastError{ERROR_SUCCESS};
	size_t requestedSize{0};
	BOOL isMapped{FALSE};
	size_t fileSize{0};
	size_t memSize{0};

	VirtualBuffer(LPCTSTR lpFileName, DWORD dwDesiredAccess = GENERIC_READ, DWORD dwShareMode = FILE_SHARE_READ, DWORD dwCreationDisposition = OPEN_EXISTING, __int64 FileSize = 0) {
		auto fileHandle = CreateFile(
			lpFileName,
			dwDesiredAccess,
			dwShareMode,
			nullptr,
			dwCreationDisposition,
			0,
			nullptr);

		if (fileHandle == INVALID_HANDLE_VALUE) { ErrorFormatMessage::exGetLastError(); }
		//SetFilePointerEx(fileHandle, )

		LARGE_INTEGER LargeFileSize;
		LargeFileSize.QuadPart = FileSize;

		DWORD flProtect{ SafeInt<DWORD>(dwDesiredAccess == GENERIC_READ ? PAGE_READONLY : PAGE_READWRITE) };
		auto mappingHandle = CreateFileMappingW(
			fileHandle,
			nullptr,
			flProtect,
			LargeFileSize.HighPart,
			LargeFileSize.LowPart,
			nullptr);

		if (!mappingHandle) { ErrorFormatMessage::exGetLastError(); }
		GetFileSizeEx(fileHandle, reinterpret_cast<PLARGE_INTEGER>(&fileSize));
		CloseHandle(fileHandle);

		DWORD dwMappingDesiredAccess{ SafeInt<DWORD>(dwDesiredAccess == GENERIC_READ ? FILE_MAP_READ : FILE_MAP_WRITE) };

		ptr = MapViewOfFile(
			mappingHandle,
			dwMappingDesiredAccess,
			0,
			0,
			FileSize);

		if (!ptr) { ErrorFormatMessage::exGetLastError(); }
		CloseHandle(mappingHandle);
//	memSize = size;
		isMapped = TRUE;
	}

	VirtualBuffer(std::wstring lpFileName, DWORD dwDesiredAccess = GENERIC_READ, DWORD dwShareMode = FILE_SHARE_READ) {
		auto fileHandle = CreateFile(lpFileName.data(), dwDesiredAccess, dwShareMode, nullptr, OPEN_EXISTING, 0, nullptr);
		GetFileSizeEx(fileHandle, reinterpret_cast<PLARGE_INTEGER>(&fileSize));
		auto mappingHandle = CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
		CloseHandle(fileHandle);
		ptr = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, 0);
		CloseHandle(mappingHandle);
//	memSize = size;
		isMapped = TRUE;
	}

	VirtualBuffer() { }

	VirtualBuffer(size_t allocSize):VirtualBuffer(nullptr, allocSize) { }

	VirtualBuffer(LPVOID lpAddress, size_t allocSize): requestedSize{allocSize} {
		//printf("construct VirtualBuffer(%p, %Iu).ptr = %p\n", lpAddress, allocSize, ptr);
		ptr = VirtualAlloc(lpAddress, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if(!ptr) {
			ErrorFormatMessage::exGetLastError();
		}
		//printf("ready VirtualBuffer(%Iu).ptr = %p\n", size, ptr);
	}

	~VirtualBuffer() {
		//printf("~VirtualBuffer(%Iu).ptr = %p\n", size, ptr);
		if(ptr) {
			if(isMapped) {
				auto success = UnmapViewOfFile(ptr);
				if(!success) {
					ErrorFormatMessage::exGetLastError();
				} else {
					ptr = nullptr;
				}
			} else {
				auto success = VirtualFree(ptr, 0, MEM_RELEASE);
				if(!success) {
					ErrorFormatMessage::exGetLastError();
				} else {
					ptr = nullptr;
				}
			}
		}
	}

	void lock() {
		//const auto err = VirtualLock(ptr, size);
		const auto success = VirtualLock(ptr, size);
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
		//requestedSize
	}

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	void prefetch() {
		WIN32_MEMORY_RANGE_ENTRY mre{ptr, size};
		const auto success = PrefetchVirtualMemory(GetCurrentProcess(), 1, &mre, 0);
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
	}

	void prefetchFile() {
		WIN32_MEMORY_RANGE_ENTRY mre{ptr, fileSize};
		const auto success = PrefetchVirtualMemory(GetCurrentProcess(), 1, &mre, 0);
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
	}
#endif // (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

	void unlock() {
		const auto success = VirtualUnlock(ptr, size);
		if(!success) {
			lastError = GetLastError();
			if(ERROR_NOT_LOCKED == lastError) {
				puts("ERROR_NOT_LOCKED");
			} else {
				ErrorFormatMessage::exGetLastError();
			}
		}
	}

	void commit() {
		auto success = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
	}

	void decommit() {
		auto success = VirtualFree(ptr, 0, MEM_DECOMMIT);
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
	}

	void reserve() {
		auto success = (ptr = VirtualAlloc(ptr, requestedSize, MEM_RESERVE, PAGE_READWRITE));
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
	}

	void release() {
		auto success = VirtualFree(ptr, 0, MEM_RELEASE);
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
		ptr = 0;
	}

	void recommit() {
		decommit();
		commit();
	}

	void reset() {
		auto success = VirtualAlloc(ptr, size, MEM_RESET, PAGE_NOACCESS);
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
	}

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	void reset_undo() {
		auto success = VirtualAlloc(ptr, size, MEM_RESET_UNDO, PAGE_NOACCESS);
		if(!success) {
			ErrorFormatMessage::exGetLastError();
		}
	}
#endif // (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

	//uint8_t* ptr_uint8_t(size_t subscript=0) { return reinterpret_cast<uint8_t*>(ptr) + subscript;}
#define ptrCast(cType) \
	operator cType() { return reinterpret_cast<cType>(ptr); } \
	operator const cType() { return reinterpret_cast<cType>(ptr); } \
	operator volatile cType() { return reinterpret_cast<cType>(ptr); } \
	operator volatile const cType() { return reinterpret_cast<cType>(ptr); }

	ptrCast(void*);
	ptrCast(__wchar_t*);
//ptrCast(char16_t*);
//ptrCast(char32_t*);
	ptrCast(long double*);
	ptrCast(double*);
	ptrCast(float*);

	ptrCast(unsigned __int64*);
	ptrCast(__int64*);
	ptrCast(unsigned __int32*);
	ptrCast(__int32*);
	ptrCast(unsigned __int16*);
	ptrCast(__int16*);
	ptrCast(unsigned __int8*);
	ptrCast(__int8*);

	
	size_t GET_size() {
		if(!ptr) { return 0; }
		MEMORY_BASIC_INFORMATION mbi{nullptr, nullptr, 0, 0, 0, 0, 0};
		auto numBytes = VirtualQuery(ptr, &mbi, sizeof(mbi));
		if(!numBytes) {
			ErrorFormatMessage::exGetLastError();
			return 0;
		}
		return mbi.RegionSize;
	}
	__declspec(property(get = GET_size)) size_t size;

	uint8_t* GET_ptr_uint8_t() { return reinterpret_cast<uint8_t*>(ptr); } __declspec(property(get = GET_ptr_uint8_t)) uint8_t* ptr_uint8_t;
	uint16_t* GET_ptr_uint16_t() { return reinterpret_cast<uint16_t*>(ptr); } __declspec(property(get = GET_ptr_uint16_t)) uint16_t* ptr_uint16_t;
	uint32_t* GET_ptr_uint32_t() { return reinterpret_cast<uint32_t*>(ptr); } __declspec(property(get = GET_ptr_uint32_t)) uint32_t* ptr_uint32_t;
	uint64_t* GET_ptr_uint64_t() { return reinterpret_cast<uint64_t*>(ptr); } __declspec(property(get = GET_ptr_uint64_t)) uint64_t* ptr_uint64_t;

	int8_t* GET_ptr_int8_t() { return reinterpret_cast<int8_t*>(ptr); } __declspec(property(get = GET_ptr_int8_t)) int8_t* ptr_int8_t;
	int16_t* GET_ptr_int16_t() { return reinterpret_cast<int16_t*>(ptr); } __declspec(property(get = GET_ptr_int16_t)) int16_t* ptr_int16_t;
	int32_t* GET_ptr_int32_t() { return reinterpret_cast<int32_t*>(ptr); } __declspec(property(get = GET_ptr_int32_t)) int32_t* ptr_int32_t;
	int64_t* GET_ptr_int64_t() { return reinterpret_cast<int64_t*>(ptr); } __declspec(property(get = GET_ptr_int64_t)) int64_t* ptr_int64_t;

	char* GET_ptr_char() { return reinterpret_cast<char*>(ptr); } __declspec(property(get = GET_ptr_char)) char* ptr_char;
//char_t* GET_ptr_char_t() { return reinterpret_cast<char_t*>(ptr); } __declspec(property(get = GET_ptr_char_t)) char_t* ptr_char_t;
	wchar_t* GET_ptr_wchar_t() { return reinterpret_cast<wchar_t*>(ptr); } __declspec(property(get = GET_ptr_wchar_t)) wchar_t* ptr_wchar_t;
	char16_t* GET_ptr_char16_t() { return reinterpret_cast<char16_t*>(ptr); } __declspec(property(get = GET_ptr_char16_t)) char16_t* ptr_char16_t;
	char32_t* GET_ptr_char32_t() { return reinterpret_cast<char32_t*>(ptr); } __declspec(property(get = GET_ptr_char32_t)) char32_t* ptr_char32_t;

#ifdef NDEBUG
	uint8_t GET_array_uint8_t(size_t n) { return ptr_uint8_t[n]; } __declspec(property(get = GET_array_uint8_t)) uint8_t array_uint8_t[];
	uint16_t GET_array_uint16_t(size_t n) { return ptr_uint16_t[n]; } __declspec(property(get = GET_array_uint16_t)) uint16_t array_uint16_t[];
	uint32_t GET_array_uint32_t(size_t n) { return ptr_uint32_t[n]; } __declspec(property(get = GET_array_uint32_t)) uint32_t array_uint32_t[];
	uint64_t GET_array_uint64_t(size_t n) { return ptr_uint64_t[n]; } __declspec(property(get = GET_array_uint64_t)) uint64_t array_uint64_t[];

	int8_t GET_array_int8_t(size_t n) { return ptr_int8_t[n]; } __declspec(property(get = GET_array_int8_t)) int8_t array_int8_t[];
	int16_t GET_array_int16_t(size_t n) { return ptr_int16_t[n]; } __declspec(property(get = GET_array_int16_t)) int16_t array_int16_t[];
	int32_t GET_array_int32_t(size_t n) { return ptr_int32_t[n]; } __declspec(property(get = GET_array_int32_t)) int32_t array_int32_t[];
	int64_t GET_array_int64_t(size_t n) { return ptr_int64_t[n]; } __declspec(property(get = GET_array_int64_t)) int64_t array_int64_t[];
#endif
#ifdef _DEBUG
	uint8_t GET_array_uint8_t(size_t n=0) { if(n * sizeof(decltype(GET_array_uint8_t())) >= fileSize) { throw("Out of Bounds"); } return ptr_uint8_t[n]; } __declspec(property(get = GET_array_uint8_t)) uint8_t array_uint8_t[];
	uint16_t GET_array_uint16_t(size_t n=0) { if(n * sizeof(decltype(GET_array_uint16_t())) >= fileSize) { throw("Out of Bounds"); } return ptr_uint16_t[n]; } __declspec(property(get = GET_array_uint16_t)) uint16_t array_uint16_t[];
	uint32_t GET_array_uint32_t(size_t n=0) { if(n * sizeof(decltype(GET_array_uint32_t())) >= fileSize) { throw("Out of Bounds"); } return ptr_uint32_t[n]; } __declspec(property(get = GET_array_uint32_t)) uint32_t array_uint32_t[];
	uint64_t GET_array_uint64_t(size_t n=0) { if(n * sizeof(decltype(GET_array_uint64_t())) >= fileSize) { throw("Out of Bounds"); } return ptr_uint64_t[n]; } __declspec(property(get = GET_array_uint64_t)) uint64_t array_uint64_t[];

	int8_t GET_array_int8_t(size_t n=0) { if(n * sizeof(decltype(GET_array_int8_t())) >= fileSize) { throw("Out of Bounds"); } return ptr_int8_t[n]; } __declspec(property(get = GET_array_int8_t)) int8_t array_int8_t[];
	int16_t GET_array_int16_t(size_t n=0) { if(n * sizeof(decltype(GET_array_int16_t())) >= fileSize) { throw("Out of Bounds"); } return ptr_int16_t[n]; } __declspec(property(get = GET_array_int16_t)) int16_t array_int16_t[];
	int32_t GET_array_int32_t(size_t n=0) { if(n * sizeof(decltype(GET_array_int32_t())) >= fileSize) { throw("Out of Bounds"); } return ptr_int32_t[n]; } __declspec(property(get = GET_array_int32_t)) int32_t array_int32_t[];
	int64_t GET_array_int64_t(size_t n=0) { if(n * sizeof(decltype(GET_array_int64_t())) >= fileSize) { throw("Out of Bounds"); } return ptr_int64_t[n]; } __declspec(property(get = GET_array_int64_t)) int64_t array_int64_t[];
#endif

};