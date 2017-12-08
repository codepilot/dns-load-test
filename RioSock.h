#pragma once

namespace rio {
	//typedef std::array<uint8_t, 4> IPv4_Address;
	//typedef std::array<uint16_t, 8> IPv6_Address;
	class IPv4_Address {
	public:
		std::array<uint8_t, 4> numbers{ 0, 0, 0, 0 };
		uint8_t& operator[](std::size_t i) { return numbers[i]; } // write
		const uint8_t& operator[](std::size_t i) const { return numbers[i]; } //read
	};

	class RioSock {
	public:
		Sockets::GenericWin10Socket sock;
		rio::RequestQueue rq;

		RioSock() :sock(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSA_FLAG_REGISTERED_IO) {

		}

		void bindIPv4(IPv4_Address addr) {
			sockaddr_in LocalAddr{ AF_INET , htons(0),{ addr[0], addr[1], addr[2], addr[3] } };
			if(bind(sock.getSocket(), (struct sockaddr *) &LocalAddr, sizeof(LocalAddr)) != 0) {
				ErrorFormatMessage::exWSAGetLastError();
			}
		}

		void init(rio::CompletionQueue &sendCQ, rio::CompletionQueue &recvCQ, ULONG  MaxOutstandingReceive, ULONG  MaxOutstandingSend, PVOID  SocketContext = nullptr) {
			rq.init(sock, sendCQ, recvCQ, MaxOutstandingReceive, MaxOutstandingSend, this);
		}

		void init(IPv4_Address addr, rio::CompletionQueue &sendCQ, rio::CompletionQueue &recvCQ, ULONG  MaxOutstandingReceive, ULONG  MaxOutstandingSend, PVOID  SocketContext = nullptr) {
			rq.init(sock, sendCQ, recvCQ, MaxOutstandingReceive, MaxOutstandingSend, this);
			bindIPv4(addr);
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