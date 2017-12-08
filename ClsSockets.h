#pragma once


#include "msGuids.hpp"

namespace Sockets {
	template<typename sig>
	int getExtFuncPtr(SOCKET sock, DWORD dwIoControlCode, LPGUID guid, sig *ret) {
		DWORD dwBytes{ 0 };
		DWORD sizeofInput = sizeof(GUID);
		DWORD sizeofOutput = sizeof(*ret);
		return WSAIoctl(
			sock,
			dwIoControlCode,
			guid,
			sizeofInput,
			ret,
			sizeofOutput,
			&dwBytes,
			nullptr,
			nullptr);
	}

	template<typename sig>
	sig retExtFuncPtr(SOCKET sock, DWORD dwIoControlCode, const GUID *guid) {
		DWORD dwBytes{ 0 };
		DWORD sizeofInput = sizeof(GUID);
		sig ret{ nullptr };
		DWORD sizeofOutput = sizeof(ret);
		const auto size =  WSAIoctl(
			sock,
			dwIoControlCode,
			(LPVOID)guid,
			sizeofInput,
			&ret,
			sizeofOutput,
			&dwBytes,
			nullptr,
			nullptr);
		if (!ret) {
			const auto lastError = WSAGetLastError();
			printf("WSAIoctl lastError: %d\n", lastError);
			getchar();
		}
		return ret;
	}


	class GuidMsTcpIp {
	public:
		__m128i m128_TRANSMITFILE;
		GUID GUID_WSAID_TRANSMITFILE;
		GUID GUID_WSAID_ACCEPTEX;
		GUID GUID_WSAID_GETACCEPTEXSOCKADDRS;
		GUID GUID_WSAID_TRANSMITPACKETS;
		GUID GUID_WSAID_CONNECTEX;
		GUID GUID_WSAID_DISCONNECTEX;
		GUID GUID_WSAID_WSARECVMSG;
		GUID GUID_WSAID_WSASENDMSG;
		GUID GUID_WSAID_WSAPOLL;
		GUID GUID_WSAID_MULTIPLE_RIO;
		GuidMsTcpIp() : m128_TRANSMITFILE{ guid_to_m128i<0xb5367df0,0xcbac,0x11cf, 0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92>() } {
			GUID_WSAID_TRANSMITFILE = WSAID_TRANSMITFILE;
			GUID_WSAID_ACCEPTEX = WSAID_ACCEPTEX;
			GUID_WSAID_GETACCEPTEXSOCKADDRS = WSAID_GETACCEPTEXSOCKADDRS;
			GUID_WSAID_TRANSMITPACKETS = WSAID_TRANSMITPACKETS;
			GUID_WSAID_CONNECTEX = WSAID_CONNECTEX;
			GUID_WSAID_DISCONNECTEX = WSAID_DISCONNECTEX;
			GUID_WSAID_WSARECVMSG = WSAID_WSARECVMSG;
			GUID_WSAID_WSASENDMSG = WSAID_WSASENDMSG;
			GUID_WSAID_WSAPOLL = WSAID_WSAPOLL;
			GUID_WSAID_MULTIPLE_RIO = WSAID_MULTIPLE_RIO;
		}
	};

	class XMIT_PACKETS : public std::vector<TRANSMIT_PACKETS_ELEMENT> {
	public:
		void memCopy(const void *pBuffer, size_t cLength = 0) {
			auto packetMem = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cLength);
			CopyMemory(packetMem, pBuffer, cLength);
			mem(packetMem, cLength);
		}
		void mem(const void *pBuffer, size_t cLength = 0) {
			resize(size() + 1);
			auto dst = data() + size() - 1;
			SecureZeroMemory(dst, sizeof(*dst));
			dst->dwElFlags = TP_ELEMENT_MEMORY;
			dst->cLength = SafeInt<ULONG>(cLength);
			dst->pBuffer = (decltype(dst->pBuffer))pBuffer;
		}

		void file(std::wstring path, __int64 nFileOffset = 0, size_t cLength = 0) {
			file(CreateFileW(
				path.c_str(),
				GENERIC_READ,
				FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ,
				nullptr,
				OPEN_EXISTING,
				FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,
				nullptr),
				nFileOffset,
				cLength);
		}

		void file(HANDLE hFile, __int64 nFileOffset = 0, size_t cLength = 0) {
			resize(size() + 1);
			auto dst = data() + size() - 1;
			SecureZeroMemory(dst, sizeof(*dst));
			dst->dwElFlags = TP_ELEMENT_FILE;
			dst->cLength = SafeInt<ULONG>(cLength);
			dst->nFileOffset.QuadPart = nFileOffset;
			dst->hFile = hFile;
		}
		void eop() {
			auto dst = this->end() - 1;
			dst->dwElFlags |= TP_ELEMENT_EOP;
		}
	};

	class Win10SocketLib {
	public:
		WSADATA wsaData{ 0 };
		Win10SocketLib() {
			const auto startupErr{ WSAStartup(MAKEWORD(2, 2), &wsaData) };
			switch (startupErr) {
			case ERROR_SUCCESS: return;
			case WSASYSNOTREADY: throw std::runtime_error("The underlying network subsystem is not ready for network communication.");
			case WSAVERNOTSUPPORTED: throw std::runtime_error("The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.");
			case WSAEINPROGRESS: throw std::runtime_error("A blocking Windows Sockets 1.1 operation is in progress.");
			case WSAEPROCLIM: throw std::runtime_error("A limit on the number of tasks supported by the Windows Sockets implementation has been reached.");
			case WSAEFAULT: throw std::runtime_error("The lpWSAData parameter is not a valid pointer.");
			default: throw std::runtime_error("Unknown Error");
			}
		}
		~Win10SocketLib() {
			WSACleanup();
		}
	};

	class MsSockFuncPtrs {
	public:
		LPFN_ACCEPTEX AcceptEx{ nullptr };
		LPFN_CONNECTEX ConnectEx{ nullptr };
		LPFN_DISCONNECTEX DisconnectEx{ nullptr };
		LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs{ nullptr };
		LPFN_TRANSMITFILE TransmitFile{ nullptr };
		LPFN_TRANSMITPACKETS TransmitPackets{ nullptr };
		LPFN_WSARECVMSG WSARecvMsg{ nullptr };
		LPFN_WSASENDMSG WSASendMsg{ nullptr };
	//LPFN_WSAPOLL WSAPoll{ nullptr };
		MsSockFuncPtrs(SOCKET sock, GuidMsTcpIp &wsaId) :
			//WSAPoll{ retExtFuncPtr<LPFN_WSAPOLL>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_WSAPOLL) }
			AcceptEx{ retExtFuncPtr<LPFN_ACCEPTEX>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_ACCEPTEX) },
			ConnectEx{ retExtFuncPtr<LPFN_CONNECTEX>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_CONNECTEX) },
			DisconnectEx{ retExtFuncPtr<LPFN_DISCONNECTEX>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_DISCONNECTEX) },
			GetAcceptExSockaddrs{ retExtFuncPtr<LPFN_GETACCEPTEXSOCKADDRS>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_GETACCEPTEXSOCKADDRS) },
			TransmitFile{ retExtFuncPtr<LPFN_TRANSMITFILE>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_TRANSMITFILE) },
			TransmitPackets{ retExtFuncPtr<LPFN_TRANSMITPACKETS>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_TRANSMITPACKETS) },
			WSARecvMsg{ retExtFuncPtr<LPFN_WSARECVMSG>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_WSARECVMSG) },
			WSASendMsg{ retExtFuncPtr<LPFN_WSASENDMSG>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, Sockets::m128_WSASENDMSG) }
		 {

		}
	};

	class MsSockFuncs {
	public:
		GuidMsTcpIp wsaId;
		MsSockFuncPtrs funcPtrs;

		MsSockFuncs(SOCKET sock) : wsaId{}, funcPtrs{sock, wsaId } {

		}
	};

	class GenericWin10Socket {
	public:
		SOCKET sock{ INVALID_SOCKET };
		MsSockFuncs msSockFuncs;
		static GuidMsTcpIp GUID_WSAID;

		RIO_EXTENSION_FUNCTION_TABLE rioTable{
			sizeof(RIO_EXTENSION_FUNCTION_TABLE),
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr };

		LPFN_RIORECEIVE RIOReceive;
		LPFN_RIORECEIVEEX RIOReceiveEx;
		LPFN_RIOSEND RIOSend;
		LPFN_RIOSENDEX RIOSendEx;
		LPFN_RIOCLOSECOMPLETIONQUEUE RIOCloseCompletionQueue;
		LPFN_RIOCREATECOMPLETIONQUEUE RIOCreateCompletionQueue;
		LPFN_RIOCREATEREQUESTQUEUE RIOCreateRequestQueue;
		LPFN_RIODEQUEUECOMPLETION RIODequeueCompletion;
		LPFN_RIODEREGISTERBUFFER RIODeregisterBuffer;
		LPFN_RIONOTIFY RIONotify;
		LPFN_RIOREGISTERBUFFER RIORegisterBuffer;
		LPFN_RIORESIZECOMPLETIONQUEUE RIOResizeCompletionQueue;
		LPFN_RIORESIZEREQUESTQUEUE RIOResizeRequestQueue;

		SOCKET getSocket() { return sock; }
		HANDLE getHandle() { return reinterpret_cast<HANDLE>(sock); }
		ULONG_PTR getSocketNumber() { return sock; }
		bool wasMoved{ false };
		GenericWin10Socket(GenericWin10Socket&& o) noexcept :
		sock(std::move(o.sock)),
		msSockFuncs{ std::move(o.msSockFuncs) },
			RIOReceive{ o.RIOReceive },
		RIOReceiveEx{ o.RIOReceiveEx },
		RIOSend{ o.RIOSend },
		RIOSendEx{ o.RIOSendEx },
		RIOCloseCompletionQueue{ o.RIOCloseCompletionQueue },
		RIOCreateCompletionQueue{ o.RIOCreateCompletionQueue },
		RIOCreateRequestQueue{ o.RIOCreateRequestQueue },
		RIODequeueCompletion{ o.RIODequeueCompletion },
		RIODeregisterBuffer{ o.RIODeregisterBuffer },
		RIONotify{ o.RIONotify },
		RIORegisterBuffer{ o.RIORegisterBuffer },
		RIOResizeCompletionQueue{ o.RIOResizeCompletionQueue },
		RIOResizeRequestQueue{ o.RIOResizeRequestQueue }
 {
			o.wasMoved = true;
			o.sock = INVALID_SOCKET;
		}
		GenericWin10Socket(int intAddressFamily, int intType, int intProtocol, DWORD dwFlags) : sock{ WSASocketW(intAddressFamily, intType, intProtocol, 0, 0, dwFlags) }, msSockFuncs{ sock } {
			SetFileCompletionNotificationModes(reinterpret_cast<HANDLE>(sock), FILE_SKIP_SET_EVENT_ON_HANDLE);
			//printf("ND(%u) SND(%u) RCV(%u) ", get_TCP_NODELAY(), get_SO_SNDBUF(), get_SO_RCVBUF());
			set_TCP_NODELAY(TRUE);
			set_SO_SNDBUF(0);
			set_SO_RCVBUF(0);
			//printf("ND(%u) SND(%u) RCV(%u)\n", get_TCP_NODELAY(), get_SO_SNDBUF(), get_SO_RCVBUF());

			{
#if 0
				getExtFuncPtr<LPFN_ACCEPTEX>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GUID_WSAID.GUID_WSAID_ACCEPTEX, &AcceptEx);
				getExtFuncPtr<LPFN_CONNECTEX>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GUID_WSAID.GUID_WSAID_CONNECTEX, &ConnectEx);
				getExtFuncPtr<LPFN_DISCONNECTEX>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GUID_WSAID.GUID_WSAID_DISCONNECTEX, &DisconnectEx);
				getExtFuncPtr<LPFN_TRANSMITFILE>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GUID_WSAID.GUID_WSAID_TRANSMITFILE, &TransmitFile);
				getExtFuncPtr<LPFN_TRANSMITPACKETS>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GUID_WSAID.GUID_WSAID_TRANSMITPACKETS, &TransmitPackets);
				getExtFuncPtr<LPFN_WSASENDMSG>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GUID_WSAID.GUID_WSAID_WSASENDMSG, &WSASendMsg);
				getExtFuncPtr<LPFN_WSAPOLL>(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GUID_WSAID.GUID_WSAID_WSAPOLL, &WSAPoll);
#endif
#if 1
				if (WSA_FLAG_REGISTERED_IO == dwFlags) {
					getExtFuncPtr<RIO_EXTENSION_FUNCTION_TABLE>(sock, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &GUID_WSAID.GUID_WSAID_MULTIPLE_RIO, &rioTable);
					RIOReceive = rioTable.RIOReceive;
					RIOReceiveEx = rioTable.RIOReceiveEx;
					RIOSend = rioTable.RIOSend;
					RIOSendEx = rioTable.RIOSendEx;
					RIOCloseCompletionQueue = rioTable.RIOCloseCompletionQueue;
					RIOCreateCompletionQueue = rioTable.RIOCreateCompletionQueue;
					RIOCreateRequestQueue = rioTable.RIOCreateRequestQueue;
					RIODequeueCompletion = rioTable.RIODequeueCompletion;
					RIODeregisterBuffer = rioTable.RIODeregisterBuffer;
					RIONotify = rioTable.RIONotify;
					RIORegisterBuffer = rioTable.RIORegisterBuffer;
					RIOResizeCompletionQueue = rioTable.RIOResizeCompletionQueue;
					RIOResizeRequestQueue = rioTable.RIOResizeRequestQueue;

				}
#endif
			}

			{
				BOOL InBuffer{ TRUE };
				BOOL OutBuffer{ 0 };
				DWORD dwBytes{ 0 };
				const auto err = { WSAIoctl(
					sock,
					SIO_LOOPBACK_FAST_PATH,
					&InBuffer,
					sizeof(InBuffer),
					&OutBuffer,
					sizeof(OutBuffer),
					&dwBytes,
					NULL,
					NULL) };
				//printf("InBuffer: %d, OutBuffer: %d, dwBytes: %d, err: %d\n",
				//	InBuffer, OutBuffer, dwBytes, err);
			}
		}

		~GenericWin10Socket() {
			if (INVALID_SOCKET == sock) {
				return;
			}
			closesocket(sock);
		}

		DWORD getsockopt_DWORD(int level, int optname) {
			DWORD v{ 0 };
			int sizeof_v{ sizeof(v) };
			getsockopt(sock, level, optname, reinterpret_cast<char *>(&v), &sizeof_v);
			return v;
		}

		void setsockopt_DWORD(int level, int optname, DWORD v) {
			setsockopt(sock, level, optname, reinterpret_cast<const char *>(&v), sizeof(v));
		}

		DWORD get_SO_RCVBUF() { return getsockopt_DWORD(SOL_SOCKET, SO_RCVBUF); }
		DWORD get_SO_SNDBUF() { return getsockopt_DWORD(SOL_SOCKET, SO_SNDBUF); }
		DWORD get_TCP_NODELAY() { return getsockopt_DWORD(IPPROTO_TCP, TCP_NODELAY); }

		void set_SO_RCVBUF(DWORD v) { setsockopt_DWORD(SOL_SOCKET, SO_RCVBUF, v); }
		void set_SO_SNDBUF(DWORD v) { setsockopt_DWORD(SOL_SOCKET, SO_SNDBUF, v); }
		void set_TCP_NODELAY(DWORD v) { setsockopt_DWORD(IPPROTO_TCP, TCP_NODELAY, v); }
	};

	template<int intAddressFamily, int intType, int intProtocol, DWORD dwFlags> class Win10Socket: public GenericWin10Socket {
	public:

		Win10Socket() : GenericWin10Socket{ intAddressFamily, intType, IPPROTO_TCP, dwFlags } {

			
		}
		~Win10Socket() { }



#if 0
		int _setsockopt() {

			SOL_SOCKET
			//SO_BROADCAST	BOOL	Configures a socket for sending broadcast data.
			//SO_CONDITIONAL_ACCEPT	BOOL	Enables incoming connections are to be accepted or rejected by the application, not by the protocol stack.
			//SO_DEBUG	BOOL	Enables debug output.Microsoft providers currently do not output any debug information.
				SO_DONTLINGER	BOOL	Does not block close waiting for unsent data to be sent.Setting this option is equivalent to setting SO_LINGER with l_onoff set to zero.
				SO_DONTROUTE	BOOL	Sets whether outgoing data should be sent on interface the socket is bound to and not a routed on some other interface.This option is not supported on ATM sockets(results in an error).
			//SO_GROUP_PRIORITY	int	Reserved.
			//SO_KEEPALIVE	BOOL	Enables sending keep - alive packets for a socket connection.Not supported on ATM sockets(results in an error).
			//SO_LINGER	LINGER	Lingers on close if unsent data is present.
			//SO_OOBINLINE	BOOL	Indicates that out - of - bound data should be returned in - line with regular data.This option is only valid for connection - oriented protocols that support out - of - band data.For a discussion of this topic, see Protocol Independent Out - Of - band Data.
		0		SO_RCVBUF	int	Specifies the total per - socket buffer space reserved for receives.
		TF	SO_REUSEADDR	BOOL	Allows the socket to be bound to an address that is already in use.For more information, see bind.Not applicable on ATM sockets.
		TF	SO_EXCLUSIVEADDRUSE	BOOL	Enables a socket to be bound for exclusive access.Does not require administrative privilege.
		0		SO_RCVTIMEO	DWORD	Sets the timeout, in milliseconds, for blocking receive calls.
		0		SO_SNDBUF	int	Specifies the total per - socket buffer space reserved for sends.
		0		SO_SNDTIMEO	DWORD	The timeout, in milliseconds, for blocking send calls.
		*		SO_UPDATE_ACCEPT_CONTEXT	int	Updates the accepting socket with the context of the listening socket.
				SO_UPDATE_CONNECT_CONTEXT
			//PVD_CONFIG	Service Provider Dependent	This object stores the configuration information for the service provider associated with socket s.The exact format of this data structure is service provider specific.
				SO_PORT_SCALABILITY
				SO_REUSE_UNICASTPORT
				SO_ERROR
				SO_MAX_MSG_SIZE
				SO_MAXDG
				SO_MAXPATHDG
				SO_RANDOMIZE_PORT
				
			IPPROTO_TCP
		T		TCP_NODELAY	BOOL	Disables the Nagle algorithm for send coalescing. This socket option is included for backward compatibility with Windows Sockets 1.1
#endif
#if 0
			int setsockopt(
				sock,
				_In_       int    level,
				_In_       int    optname,
				_In_ const char   *optval,
				_In_       int    optlen
			);
		}
#endif

#if 0
		int _getsockopt() {

			int getsockopt(
				sock,
				_In_    int    level,
				_In_    int    optname,
				_Out_   char   *optval,
				_Inout_ int    *optlen
			);
		}
#endif

#if 0
		WSAGetOverlappedResult
			BOOL WSAAPI WSAGetOverlappedResult(
				_In_  SOCKET          s,
				_In_  LPWSAOVERLAPPED lpOverlapped,
				_Out_ LPDWORD         lpcbTransfer,
				_In_  BOOL            fWait,
				_Out_ LPDWORD         lpdwFlags
			);

#endif

		int bindAddress(uint16_t port, uint32_t addr = INADDR_ANY) {
			struct sockaddr_in Addr = { 0 };
			Addr.sin_family = AF_INET;
			INADDR_ANY;
			Addr.sin_addr.S_un.S_addr = htonl(addr);
			Addr.sin_port = htons(80);
			return bind(sock, reinterpret_cast<LPSOCKADDR>(&Addr), sizeof(struct sockaddr_in));
		}

		int bindAddress(uint16_t port, uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
			struct sockaddr_in Addr = { 0 };
			Addr.sin_family = AF_INET;
			INADDR_ANY;
			Addr.sin_addr.S_un.S_un_b = { a, b, c, d };//INADDR_ANY;
			Addr.sin_port = htons(80);
			return bind(sock, reinterpret_cast<LPSOCKADDR>(&Addr), sizeof(struct sockaddr_in));
		}

		int startListening(int backlog = SOMAXCONN) {
			return listen(sock, backlog);
		}

		HANDLE iocp(HANDLE hIOCP, ULONG_PTR CompletionKey = 0) {
			return CreateIoCompletionPort(
				reinterpret_cast<HANDLE>(sock),
				hIOCP,
				CompletionKey,
				0);
		}




		BOOL disconnectReuse(LPOVERLAPPED lpOverlapped) {
			return DisconnectEx(sock, lpOverlapped, TF_REUSE_SOCKET, 0);
		}

		BOOL xmitPackets(LPTRANSMIT_PACKETS_ELEMENT lpPacketArray, DWORD nElementCount, LPOVERLAPPED lpOverlapped, DWORD dwFlags) {
			return TransmitPackets(sock, lpPacketArray, nElementCount, 0, lpOverlapped, dwFlags);
		}

		BOOL xmitPackets(XMIT_PACKETS &xpa, LPOVERLAPPED lpOverlapped, DWORD dwFlags) {
			return xmitPackets(xpa.data(), SafeInt<DWORD>(xpa.size()), lpOverlapped, dwFlags);
		}

		BOOL xmitFile(LPOVERLAPPED lpOverlapped, LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers, DWORD flags) {
			return TransmitFile(
				sock,
				nullptr,
				0,
				0,
				lpOverlapped,
				lpTransmitBuffers,
				flags);
		}

		BOOL read(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped) {
			DWORD NumberOfBytesRead{ 0 };
		//return ReadFile(reinterpret_cast<HANDLE>(sock), lpBuffer, nNumberOfBytesToRead, &NumberOfBytesRead, lpOverlapped);
			WSABUF bufs{ nNumberOfBytesToRead, reinterpret_cast<CHAR *>(lpBuffer)};
			DWORD flags{ MSG_PUSH_IMMEDIATE };
			return WSARecv(sock, &bufs, 1, nullptr, &flags, lpOverlapped, nullptr);
		}
			BOOL acceptFromListener(SOCKET sListenSocket, LPSOCKADDR_STORAGE addresses, LPOVERLAPPED lpOverlapped) {
				DWORD dwBytesReceived{ 0 };
				return AcceptEx(
					sListenSocket,
					sock,
					addresses,
					0,
					sizeof(SOCKADDR_STORAGE),
					sizeof(SOCKADDR_STORAGE),
					&dwBytesReceived,
					lpOverlapped
				);
			}
#if 0
			GetAcceptExSockaddrs
				_ConnectEx() {
				/*
				BOOL PASCAL ConnectEx(
				_In_     SOCKET                s,
				_In_     const struct sockaddr *name,
				_In_     int                   namelen,
				_In_opt_ PVOID                 lpSendBuffer,
				_In_     DWORD                 dwSendDataLength,
				_Out_    LPDWORD               lpdwBytesSent,
				_In_     LPOVERLAPPED          lpOverlapped
				);
				*/
			}
			_DisconnectEx() {
				BOOL DisconnectEx(
					_In_ SOCKET       hSocket,
					_In_ LPOVERLAPPED lpOverlapped,
					_In_ DWORD        dwFlags,
					_In_ DWORD        reserved
				);
			}
			_TransmitFile() {
				BOOL PASCAL TransmitFile(
					SOCKET                  hSocket,
					HANDLE                  hFile,
					DWORD                   nNumberOfBytesToWrite,
					DWORD                   nNumberOfBytesPerSend,
					LPOVERLAPPED            lpOverlapped,
					LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
					DWORD                   dwFlags
				);
			}
			_TransmitPackets() {
				BOOL PASCAL TransmitPackets(
					SOCKET                     hSocket,
					LPTRANSMIT_PACKETS_ELEMENT lpPacketArray,
					DWORD                      nElementCount,
					DWORD                      nSendSize,
					LPOVERLAPPED               lpOverlapped,
					DWORD                      dwFlags
				);
			}
			_WSARecv() {
				int WSARecv(
					_In_    SOCKET                             s,
					_Inout_ LPWSABUF                           lpBuffers,
					_In_    DWORD                              dwBufferCount,
					_Out_   LPDWORD                            lpNumberOfBytesRecvd,
					_Inout_ LPDWORD                            lpFlags,
					_In_    LPWSAOVERLAPPED                    lpOverlapped,
					_In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				);
			}
			_WSARecvFrom() {
				int WSARecvFrom(
					_In_    SOCKET                             s,
					_Inout_ LPWSABUF                           lpBuffers,
					_In_    DWORD                              dwBufferCount,
					_Out_   LPDWORD                            lpNumberOfBytesRecvd,
					_Inout_ LPDWORD                            lpFlags,
					_Out_   struct sockaddr                    *lpFrom,
					_Inout_ LPINT                              lpFromlen,
					_In_    LPWSAOVERLAPPED                    lpOverlapped,
					_In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				);
			}
			_WSARecvMsg() {
				int WSARecvMsg(
					_In_    SOCKET                             s,
					_Inout_ LPWSAMSG                           lpMsg,
					_Out_   LPDWORD                            lpdwNumberOfBytesRecvd,
					_In_    LPWSAOVERLAPPED                    lpOverlapped,
					_In_    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				);
			}
			_WSASend() {

				int WSASend(
					_In_  SOCKET                             s,
					_In_  LPWSABUF                           lpBuffers,
					_In_  DWORD                              dwBufferCount,
					_Out_ LPDWORD                            lpNumberOfBytesSent,
					_In_  DWORD                              dwFlags,
					_In_  LPWSAOVERLAPPED                    lpOverlapped,
					_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				);
			}
			_WSASendMsg() {

				int WSASendMsg(
					_In_  SOCKET                             s,
					_In_  LPWSAMSG                           lpMsg,
					_In_  DWORD                              dwFlags,
					_Out_ LPDWORD                            lpNumberOfBytesSent,
					_In_  LPWSAOVERLAPPED                    lpOverlapped,
					_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				);
			}
			_WSASendTo() {
				int WSASendTo(
					_In_  SOCKET                             s,
					_In_  LPWSABUF                           lpBuffers,
					_In_  DWORD                              dwBufferCount,
					_Out_ LPDWORD                            lpNumberOfBytesSent,
					_In_  DWORD                              dwFlags,
					_In_  const struct sockaddr              *lpTo,
					_In_  int                                iToLen,
					_In_  LPWSAOVERLAPPED                    lpOverlapped,
					_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				);
			}
			_WSAIoctl() {
				int WSAIoctl(
					_In_  SOCKET                             s,
					_In_  DWORD                              dwIoControlCode,
					_In_  LPVOID                             lpvInBuffer,
					_In_  DWORD                              cbInBuffer,
					_Out_ LPVOID                             lpvOutBuffer,
					_In_  DWORD                              cbOutBuffer,
					_Out_ LPDWORD                            lpcbBytesReturned,
					_In_  LPWSAOVERLAPPED                    lpOverlapped,
					_In_  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
				);
#endif

#if 0
				//FIONBIO Enable or disable non - blocking mode on socket s.
				//FIONREAD Determine the amount of data that can be read atomically from socket s
				//SIOCATMARK OOB
				SIO_ACQUIRE_PORT_RESERVATION
					SIO_ADDRESS_LIST_CHANGE
					SIO_ADDRESS_LIST_QUERY
					SIO_ASSOCIATE_HANDLE
					SIO_ASSOCIATE_PORT_RESERVATION
					SIO_BASE_HANDLE
					SIO_BSP_HANDLE
					SIO_BSP_HANDLE_SELECT
					SIO_BSP_HANDLE_POLL
					SIO_CHK_QOS
					SIO_ENABLE_CIRCULAR_QUEUEING
					SIO_FIND_ROUTE
					SIO_FLUSH
					SIO_GET_BROADCAST_ADDRESS
					SIO_GET_EXTENSION_FUNCTION_POINTER
					WSAID_ACCEPTEX 						The AcceptEx extension function.
					WSAID_CONNECTEX 						The ConnectEx extension function.
					WSAID_DISCONNECTEX 						The DisconnectEx extension function.
					WSAID_GETACCEPTEXSOCKADDRS 						The GetAcceptExSockaddrs extension function.
					WSAID_TRANSMITFILE 						The TransmitFile extension function.
					WSAID_TRANSMITPACKETS 						The TransmitPackets extension function.
					WSAID_WSARECVMSG 						The WSARecvMsg extension function.
					WSAID_WSASENDMSG 						The WSASendMsg extension function.
					SIO_GET_GROUP_QOS
					SIO_GET_INTERFACE_LIST
					SIO_GET_INTERFACE_LIST_EX
					SIO_GET_QOS
					SIO_IDEAL_SEND_BACKLOG_CHANGE
					SIO_IDEAL_SEND_BACKLOG_QUERY
					SIO_KEEPALIVE_VALS
					SIO_MULTIPOINT_LOOPBACK
					SIO_MULTICAST_SCOPE
					SIO_QUERY_RSS_SCALABILITY_INFO
					SIO_QUERY_WFP_ALE_ENDPOINT_HANDLE
					SIO_RCVALL
					SIO_RCVALL_IGMPMCAST
					SIO_RCVALL_MCAST
					SIO_RELEASE_PORT_RESERVATION
					SIO_ROUTING_INTERFACE_CHANGE
					SIO_ROUTING_INTERFACE_QUERY
					SIO_SET_COMPATIBILITY_MODE
					SIO_SET_GROUP_QOS
					SIO_SET_QOS
					SIO_TRANSLATE_HANDLE
					SIO_UDP_CONNRESET
#endif
		};
			typedef Win10Socket<AF_INET, SOCK_STREAM, IPPROTO_TCP, WSA_FLAG_OVERLAPPED> OvTcp4;
			typedef Win10Socket<AF_INET6, SOCK_STREAM, IPPROTO_TCP, WSA_FLAG_OVERLAPPED> OvTcp6;
			typedef Win10Socket<AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_OVERLAPPED> OvUdp4;
			typedef Win10Socket<AF_INET6, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_OVERLAPPED> OvUdp6;
			typedef Win10Socket<AF_INET, SOCK_STREAM, IPPROTO_TCP, WSA_FLAG_REGISTERED_IO> RioTcp4;
			typedef Win10Socket<AF_INET6, SOCK_STREAM, IPPROTO_TCP, WSA_FLAG_REGISTERED_IO> RioTcp6;
			typedef Win10Socket<AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO> RioUdp4;
			typedef Win10Socket<AF_INET6, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO> RioUdp6;

	//others
	//	GetAddrInfoEx
	//	GetAddrInfoExCancel
	//	GetAddrInfoExOverlappedResult
	//	SetAddrInfoEx(

	//other 2
	//		WSAConnectByList
	//		WSAConnectByName
	//		WSADeleteSocketPeerTargetName
	//		WSAProviderConfigChange
	//		WSAQuerySocketSecurity
	//		WSASetSocketPeerTargetName
	//		WSASetSocketSecurity
};


#ifdef MainCPP //define in only one cpp file
Sockets::GuidMsTcpIp Sockets::GenericWin10Socket::GUID_WSAID;
#endif