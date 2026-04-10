#ifndef METRIC_REGISTRY_H
#define METRIC_REGISTRY_H

#include <map>
#include <memory>
#include <vector>
#include <string>

#include "metric_registry_interface.h"

struct _object;
typedef _object PyObject;

namespace prometheus_module {
	class Counter;
	class Gauge;
	class Histogram;
	class Summary;

	class CounterInterface;
	class GaugeInterface;
	class HistogramInterface;
	class SummaryInterface;
}

namespace prometheus_module {

class MetricRegistry : public MetricRegistryInterface {
public:

	MetricRegistry();

	static void RegisterPythonObject(PyObject* module);

	Counter* MakeCounter(const char* name, const std::vector<std::string>& labels);
	Gauge* MakeGauge(const char* name, const std::vector<std::string>& labels);
	Summary* MakeSummary(const char* name, const std::vector<std::string>& labels, const std::vector<std::pair<double,double> >& quantiles, int total_window_size_seconds, int window_partitions);
	Histogram* MakeHistogram(const char* name, const std::vector<std::string>& labels, const std::vector<double>& boundaries);


	// MetricRegistryInterface implementation

	CounterInterface* MakeCounter(const char* name, int num_labels, const char* label_keys[]) override;
	GaugeInterface* MakeGauge(const char* name, int num_labels, const char* label_keys[]) override;
	SummaryInterface* MakeSummary(const char* name, int num_labels, const char* label_keys[], int num_quantiles, double quantile_values[], double quantile_tolerances[], int total_window_size_seconds, int window_partitions) override;
	HistogramInterface* MakeHistogram(const char* name, int num_labels, const char* label_keys[], int num_boundaries, double boundaries[]) override;

	bool Serve(const char* bind_address) override;
	void StopServing() override;


private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif

