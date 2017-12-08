#pragma once

namespace rio {
	class RioSock {
	public:
		Sockets::GenericWin10Socket sock;
		rio::RequestQueue rq;

		RioSock() :sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO) {

		}

		void init(rio::CompletionQueue &sendCQ, rio::CompletionQueue &recvCQ, ULONG  MaxOutstandingReceive, ULONG  MaxOutstandingSend, PVOID  SocketContext = nullptr) {
			rq.init(sock, sendCQ, recvCQ, MaxOutstandingReceive, MaxOutstandingSend, this);

		}

		void queueSendEx(std::vector<SendExRequest> &ser, PRIO_BUF pData, PRIO_BUF pRemoteAddress) {
			ser.push_back({ sock.RIOSendEx , rq.requests , pData, pRemoteAddress });
		}

		void sendEx(PRIO_BUF pData, PRIO_BUF pRemoteAddress) {
			auto status = sock.RIOSendEx(
				rq.requests,
				pData,
				pData ? 1 : 0,
				nullptr,
				pRemoteAddress,
				nullptr,
				nullptr,
				0,
				nullptr);
			if (!status || status == -1) {
				ErrorFormatMessage::exWSAGetLastError();
			}
		}
	};
}