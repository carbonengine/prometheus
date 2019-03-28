#ifndef METRIC_REGISTRY_H
#define METRIC_REGISTRY_H

#include <map>
#include <memory>
#include <vector>

struct _object;
typedef _object PyObject;

namespace prometheus_module {
	class Counter;
	class Gauge;
	class Histogram;
	class Summary;
}

namespace prometheus_module {

class MetricRegistry {
public:

	MetricRegistry();

	Counter* MakeCounter(const char* name, const std::map<std::string, std::string>& labels);
	Gauge* MakeGauge(const char* name, const std::map<std::string, std::string>& labels);
	Summary* MakeSummary(const char* name, const std::map <std::string, std::string>& labels, const std::vector<std::pair<double,double> >& quantiles);
	Histogram* MakeHistogram(const char* name, const std::map <std::string, std::string>& labels, const std::vector<double>& boundaries);

	bool Serve(const char* bind_address);
	void StopServing();

	static void RegisterPythonObject(PyObject* module);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif

