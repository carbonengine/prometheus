#ifndef METRIC_REGISTRY_H
#define METRIC_REGISTRY_H

#include <map>
#include <memory>

struct _object;
typedef _object PyObject;

namespace prometheus_module {
	class Counter;
}

namespace prometheus_module {

class MetricRegistry {
public:

	MetricRegistry();

	// MakeCounter, MakeGauge, etc
	Counter* MakeCounter(const char* name, const std::map<std::string, std::string>& labels);

	void Serve(const char* bind_address);
	void StopServing();

	static void RegisterPythonObject(PyObject* module);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif

