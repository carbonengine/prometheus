#ifndef METRIC_REGISTRY_H
#define METRIC_REGISTRY_H

#include <memory>

struct _object;
typedef _object PyObject;

class MetricRegistry {
public:

	MetricRegistry();

	// MakeCounter, MakeGauge, etc
	// Serve(endpoint), StopServing

	static void RegisterPythonObject(PyObject* module);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

#endif
