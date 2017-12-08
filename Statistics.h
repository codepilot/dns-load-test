#pragma once

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
	};

	class StandardDeviation {
	public:
		std::vector<double_t> samples;
		RunningAverage avg;
		auto addSample(double_t sample) {
			samples.push_back(sample);
			avg.addSample(sample);
		}
		auto stddev() {
			RunningAverage variance;
			const auto sampleAverage = avg.avg();
			for (auto &sample : samples) {
				variance.addSample(pow(sample - sampleAverage, 2));
			}
			return sqrt(variance.avg());
		}
	};
}