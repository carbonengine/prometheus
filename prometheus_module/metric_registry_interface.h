#ifndef METRICREGISTRYINTERFACE_H
#define METRICREGISTRYINTERFACE_H

namespace prometheus_module {
	class CounterInterface;
	class GaugeInterface;
	class HistogramInterface;
	class SummaryInterface;
} // namespace prometheus_module

namespace prometheus_module {

class MetricRegistryInterface {
public:

	virtual CounterInterface* MakeCounter(const char* name, int num_labels, const char* label_keys[], const char* label_values[]) = 0;
	virtual GaugeInterface* MakeGauge(const char* name, int num_labels, const char* label_keys[], const char* label_values[]) = 0;
	virtual SummaryInterface* MakeSummary(const char* name, int num_labels, const char* label_keys[], const char* label_values[], int num_quantiles, double quantile_values[], double quantile_tolerances[]) = 0;
	virtual HistogramInterface* MakeHistogram(const char* name, int num_labels, const char* label_keys[], const char* label_values[], int num_boundaries, double boundaries[]) = 0;

	virtual bool Serve(const char* bind_address) = 0;
	virtual void StopServing() = 0;
};

} // namespace prometheus_module

#endif
