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
		std::vector<double_t> completionTimes;

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

		void completed() {
			QueryPerformanceCounter(&PerfCountCompleted);
			const auto timeTaken{ PerfCountCompleted.QuadPart - PerfCountStart.QuadPart };
			completionTimes.push_back(static_cast<double_t>(timeTaken) * freqToMicroseconds);
			if (completionTimes.size() >= 1000) {
				const auto avgMicroSeconds = avg(completionTimes);
				printf("timeTaken(%I64u): %fus, %fMbps\n", completionTimes.size(), avgMicroSeconds, static_cast<double_t>(pData->Length) * 8.0 / avgMicroSeconds);
				completionTimes.resize(0);
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