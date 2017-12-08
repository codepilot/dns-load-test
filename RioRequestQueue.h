#pragma once

namespace rio {
	class RequestQueue {
	public:
		LPFN_RIOCREATEREQUESTQUEUE RIOCreateRequestQueue;
		LPFN_RIORESIZEREQUESTQUEUE RIOResizeRequestQueue;
		RIO_RQ requests;
		void init(Sockets::GenericWin10Socket &sock, rio::CompletionQueue &sendCQ, rio::CompletionQueue &recvCQ, ULONG  MaxOutstandingReceive, ULONG  MaxOutstandingSend, PVOID  SocketContext = nullptr) {
			RIOCreateRequestQueue = sock.RIOCreateRequestQueue;
			RIOResizeRequestQueue = sock.RIOResizeRequestQueue;
			requests = RIOCreateRequestQueue(
				sock.getSocket(),
				MaxOutstandingReceive,
				1,
				MaxOutstandingSend,
				1,
				recvCQ.completion,
				sendCQ.completion,
				SocketContext
			);
			if (!requests) {
				ErrorFormatMessage::exWSAGetLastError();
			}
		}
	};
}