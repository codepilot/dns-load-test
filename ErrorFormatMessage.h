#pragma once

class ErrorFormatMessage {
public:
	ErrorFormatMessage(int errorCode) {
		LPVOID lpMsgBuf;
		DWORD err = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS/*_In_      DWORD dwFlags*/,
			nullptr/*_In_opt_  LPCVOID lpSource*/,
			errorCode/*_In_      DWORD dwMessageId*/,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)/*_In_      DWORD dwLanguageId*/,
			(LPTSTR)&lpMsgBuf/*_Out_     LPTSTR lpBuffer*/,
			0/*_In_      DWORD nSize*/,
			nullptr/*_In_opt_  va_list *Arguments*/);
		OutputDebugString(reinterpret_cast<LPCTSTR>(lpMsgBuf));
		LocalFree(lpMsgBuf);
		DebugBreak();
	}
	static void OutputDebug(const int errorCode) {
		switch(errorCode) {
			case 10055: //out of buffer
				InterlockedIncrement64(&GlobalIncompletionCount);
				//break;
			default:
			{
				LPVOID lpMsgBuf;
				DWORD err = FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS/*_In_      DWORD dwFlags*/,
					nullptr/*_In_opt_  LPCVOID lpSource*/,
					errorCode/*_In_      DWORD dwMessageId*/,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)/*_In_      DWORD dwLanguageId*/,
					(LPTSTR)&lpMsgBuf/*_Out_     LPTSTR lpBuffer*/,
					0/*_In_      DWORD nSize*/,
					nullptr/*_In_opt_  va_list *Arguments*/);
				OutputDebugString(reinterpret_cast<LPCTSTR>(lpMsgBuf));
				LocalFree(lpMsgBuf);
				DebugBreak();
			}
		}
	}
	static void exGetLastError() {
		const auto lastError{ GetLastError() };
		if (lastError) {
			OutputDebug(lastError);
		}
	}
	static void exWSAGetLastError() {
		const auto lastError{ WSAGetLastError() };
		if (lastError) {
			OutputDebug(lastError);
		}
	}
};