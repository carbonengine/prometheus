#ifndef METRIC_REGISTRY_H
#define METRIC_REGISTRY_H

#include <memory>

struct _object;
typedef _object PyObject;

class Counter;

class MetricRegistry {
public:

	MetricRegistry();

	// MakeCounter, MakeGauge, etc
	Counter* MakeCounter(const char* name);

	void Serve(const char* bind_address);
	void StopServing();

	static void RegisterPythonObject(PyObject* module);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

#endif

