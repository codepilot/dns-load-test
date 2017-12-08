#pragma once

#include <sstream>

namespace Statistics {
	class RunningAverage {
	public:
		size_t sampleCount{ 0 };
		double_t sampleSum{ 0 };
		auto addSample(double_t sample) {
			sampleSum += sample;
			sampleCount++;
		}
		auto sum() {
			return sampleSum;
		}
		auto avg() {
			return sampleSum / static_cast<double_t>(sampleCount);
		}
		auto clear() {
			SecureZeroMemory(this, sizeof(RunningAverage));
		}
	};

	class StandardDeviation {
	public:
		std::vector<double_t> samples;
		RunningAverage runningAverage;
		auto clear() {
			runningAverage.clear();
			samples.resize(0);
		}
		auto addSample(double_t sample) {
			samples.push_back(sample);
			runningAverage.addSample(sample);
		}
		auto count() {
			return runningAverage.sampleCount;
		}
		auto sum() {
			return runningAverage.sampleSum;
		}
		auto average() {
			return runningAverage.avg();
		}
		auto variance() {
			RunningAverage variance;
			const auto sampleAverage = average();
			for (auto &sample : samples) {
				variance.addSample(pow(sample - sampleAverage, 2));
			}
			return variance.avg();
		}
		auto standardDeviation() {
			return sqrt(variance());
		}
		auto statistics() {
			std::stringstream ret;
			ret
				<< "count: " << count() << ", "
				<< "sum: " << sum() << ", "
				<< "average: " << average() << ", "
				<< "variance: " << variance() << ", "
				<< "standardDeviation: " << standardDeviation();
			return ret.str();
		}
	};
}