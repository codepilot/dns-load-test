#pragma once

namespace rio {
	class SendExRequest {
	public:
		LPFN_RIOSENDEX RIOSendEx;
		RIO_RQ requests;
		PRIO_BUF pData;
		PRIO_BUF pRemoteAddress;
		LARGE_INTEGER PerfFreq{ 0 };
		LARGE_INTEGER PerfCountStart{ 0 };
		LARGE_INTEGER PerfCountCompleted{ 0 };
		double_t freqToMicroseconds{ 0 };
		Statistics::StandardDeviation stats;

		SendExRequest(LPFN_RIOSENDEX _RIOSendEx, RIO_RQ _requests, PRIO_BUF _pData, PRIO_BUF _pRemoteAddress) {
			RIOSendEx = _RIOSendEx;
			requests = _requests;
			pData = _pData;
			pRemoteAddress = _pRemoteAddress;
			QueryPerformanceFrequency(&PerfFreq);
			freqToMicroseconds = 1e6 / PerfFreq.QuadPart;
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

		void send() {
			QueryPerformanceCounter(&PerfCountStart);
			auto status = RIOSendEx(
				requests,
				pData,
				pData ? 1 : 0,
				nullptr,
				pRemoteAddress,
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