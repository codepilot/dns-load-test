#pragma once

namespace UserHandle {
	class Handle {
		void replaceHandle(HANDLE newHandle) {
			if (!(handle == nullptr || handle == INVALID_HANDLE_VALUE)) {
				CloseHandle(handle);
				handle = nullptr;
			}
			handle = newHandle;
		}
	public:
		HANDLE handle{ nullptr };
		Handle() : handle{ nullptr } {} //blank
		Handle& operator= (const Handle& src) { //copy
			replaceHandle(duplicate(src.handle));
		}
		Handle& operator= (const HANDLE& src) { //copy
			replaceHandle(duplicate(src));
		}
		Handle& operator= (Handle&& src) { //move
			replaceHandle(duplicateClose(src.handle));
		}
		Handle& operator= (HANDLE&& src) { //move
			replaceHandle(duplicateClose(src));
		}
		Handle(HANDLE _handle) : handle{ _handle } {} //constructor
		Handle(const Handle &src) : handle{ duplicate(src.handle) } { } // copy constructor
		Handle(Handle&& src) noexcept : handle{ duplicateClose(src.handle) } { } // move constructor

		static HANDLE duplicate(const HANDLE &src) {
			HANDLE ret{ nullptr };
			if (!DuplicateHandle(GetCurrentProcess(), src, GetCurrentProcess(), &ret, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
				ErrorFormatMessage::exGetLastError();
			}
			return ret;
		}
		static HANDLE duplicateClose(HANDLE &src) {
			HANDLE ret{ nullptr };
			if (!DuplicateHandle(GetCurrentProcess(), src, GetCurrentProcess(), &ret, 0, TRUE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)) {
				ErrorFormatMessage::exGetLastError();
			}
			src = 0;
			return ret;
		}
		operator HANDLE() { return handle; }
	};

}