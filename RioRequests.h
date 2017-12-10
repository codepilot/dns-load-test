#pragma once

namespace rio {
	class ExRequest {
	public:
		const BOOL isSend;
		ExRequest(BOOL isSend) : isSend{ isSend } { }
	};

	class SendExRequest: public ExRequest {
	public:
		LPFN_RIOSENDEX RIOSendEx;
		RIO_RQ requests;
		RIO_BUF pData;
		RIO_BUF pRemoteAddress;
		LARGE_INTEGER PerfFreq{ 0 };
		LARGE_INTEGER PerfCountStart{ 0 };
		LARGE_INTEGER PerfCountCompleted{ 0 };
		double_t freqToMicroseconds{ 0 };
		Statistics::StandardDeviation stats;

		SendExRequest(LPFN_RIOSENDEX _RIOSendEx, RIO_RQ _requests, RIO_BUF _pData, RIO_BUF _pRemoteAddress) : ExRequest{ true } {
			RIOSendEx = _RIOSendEx;
			requests = _requests;
			pData = _pData;
			pRemoteAddress = _pRemoteAddress;
			QueryPerformanceFrequency(&PerfFreq);
			freqToMicroseconds = 1e6 / PerfFreq.QuadPart;
		}

		static void deferSendAll(std::vector<SendExRequest> &ser) {
			for (auto &n : ser) {
				n.deferSend();
			}
		}

		static void sendAll(std::vector<SendExRequest> &ser) {
			for (auto &n : ser) {
				n.send();
			}
		}

		static double_t sum(std::vector<double_t> dv) {
			double_t sum{ 0 };
			for (auto &n : dv) {
				sum += n;
			}
			return sum;
		}

		static double_t avg(std::vector<double_t> dv) {
			return sum(dv) / static_cast<double_t>(dv.size());
		}

		PAPCFUNC AccumFunc{ nullptr };
		HANDLE AccumThread{ nullptr };
		size_t AccumSize{ 0xFFFFFFFF };
		void completed() {
			QueryPerformanceCounter(&PerfCountCompleted);
			const auto timeTaken{ PerfCountCompleted.QuadPart - PerfCountStart.QuadPart };
			stats.addSample(static_cast<double_t>(timeTaken) * freqToMicroseconds);
			if (stats.count() == AccumSize) {
				QueueUserAPC(AccumFunc, AccumThread, static_cast<ULONG_PTR>( stats.sum() ));
				stats.clear();
			}
		}

		void commit() {
			auto status = RIOSendEx(
				requests,
				nullptr,
				0,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				RIO_MSG_COMMIT_ONLY,
				this);
		}

		void deferSend() {
			QueryPerformanceCounter(&PerfCountStart);
			if (GlobalSendCount >= SafeInt<__int64>( MaxSendCount )) {
				return;
			}
			InterlockedIncrement64(&GlobalSendCount);
			auto status = RIOSendEx(
				requests,
				pData.BufferId? &pData:nullptr,
				pData.BufferId ? 1 : 0,
				nullptr,
				pRemoteAddress.BufferId? &pRemoteAddress:nullptr,
				nullptr,
				nullptr,
				RIO_MSG_DEFER,
				this);
			if (!status || status == -1) {
				ErrorFormatMessage::exWSAGetLastError();
			}
		}

		void send() {
			QueryPerformanceCounter(&PerfCountStart);
			if (GlobalSendCount >= SafeInt<__int64>( MaxSendCount )) {
				return;
			}
			*reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(globalSendBufferBuf) + pData.Offset) = static_cast<uint16_t>( InterlockedIncrement64(&GlobalSendCount));
			InterlockedIncrement64(&DnsRequestCounter);
			auto status = RIOSendEx(
				requests,
				pData.BufferId? &pData:nullptr,
				pData.BufferId ? 1 : 0,
				nullptr,
				pRemoteAddress.BufferId?&pRemoteAddress:nullptr,
				nullptr,
				nullptr,
				0,
				this);
			if (!status || status == -1) {
				ErrorFormatMessage::exWSAGetLastError();
			}
		}
	};

	class ReceiveExRequest : public ExRequest {
	public:
		LPFN_RIORECEIVEEX RIOReceiveEx;
		RIO_RQ requests;
		RIO_BUF pData;
		RIO_BUF pLocalAddress;
		RIO_BUF pRemoteAddress;
		LARGE_INTEGER PerfFreq{ 0 };
		LARGE_INTEGER PerfCountStart{ 0 };
		LARGE_INTEGER PerfCountCompleted{ 0 };
		double_t freqToMicroseconds{ 0 };
		Statistics::StandardDeviation stats;

		ReceiveExRequest(LPFN_RIORECEIVEEX _RIOReceiveEx, RIO_RQ _requests, RIO_BUF _pData, RIO_BUF _pLocalAddress, RIO_BUF _pRemoteAddress) : ExRequest{ false } {
			RIOReceiveEx = _RIOReceiveEx;
			requests = _requests;
			pData = _pData;
			pLocalAddress = _pLocalAddress;
			pRemoteAddress = _pRemoteAddress;
			QueryPerformanceFrequency(&PerfFreq);
			freqToMicroseconds = 1e6 / PerfFreq.QuadPart;
		}

		static void deferReceiveAll(std::vector<ReceiveExRequest> &ser) {
			for (auto &n : ser) {
				n.deferReceive();
			}
		}

		static void receiveAll(std::vector<ReceiveExRequest> &ser) {
			size_t recvNum = 0;
			for (auto &n : ser) {
				n.receive();
				recvNum++;
			}
		}

		static double_t sum(std::vector<double_t> dv) {
			double_t sum{ 0 };
			for (auto &n : dv) {
				sum += n;
			}
			return sum;
		}

		static double_t avg(std::vector<double_t> dv) {
			return sum(dv) / static_cast<double_t>(dv.size());
		}

		PAPCFUNC AccumFunc{ nullptr };
		HANDLE AccumThread{ nullptr };
		size_t AccumSize{ 0xFFFFFFFF };
		void completed() {
			QueryPerformanceCounter(&PerfCountCompleted);
			const auto timeTaken{ PerfCountCompleted.QuadPart - PerfCountStart.QuadPart };
			stats.addSample(static_cast<double_t>(timeTaken) * freqToMicroseconds);
			if (stats.count() == AccumSize) {
				QueueUserAPC(AccumFunc, AccumThread, static_cast<ULONG_PTR>(stats.sum()));
				stats.clear();
			}
		}

		void commit() {
			auto status = RIOReceiveEx(
				requests,
				nullptr,
				0,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				RIO_MSG_COMMIT_ONLY,
				this);
		}

		void deferReceive() {
			QueryPerformanceCounter(&PerfCountStart);
			InterlockedIncrement64(&GlobalReceiveCount);
			auto status = RIOReceiveEx(
				requests,
				pData.BufferId?&pData:nullptr,
				pData.BufferId ? 1 : 0,
				pLocalAddress.BufferId?&pLocalAddress:nullptr,
				pRemoteAddress.BufferId?&pRemoteAddress:nullptr,
				nullptr,
				nullptr,
				RIO_MSG_DEFER,
				this);
			if (!status || status == -1) {
				ErrorFormatMessage::exWSAGetLastError();
			}
		}

		void receive() {
			QueryPerformanceCounter(&PerfCountStart);
			InterlockedIncrement64(&GlobalReceiveCount);
			auto status = RIOReceiveEx(
				requests,
				pData.BufferId?&pData:nullptr,
				pData.BufferId ? 1 : 0,
				pLocalAddress.BufferId?&pLocalAddress:nullptr,
				pRemoteAddress.BufferId?&pRemoteAddress:nullptr,
				nullptr,
				nullptr,
				0,
				this);
			if (!status || status == -1) {
				ErrorFormatMessage::exWSAGetLastError();
			}
		}
	};

}