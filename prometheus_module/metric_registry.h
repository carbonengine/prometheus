#ifndef METRIC_REGISTRY_H
#define METRIC_REGISTRY_H

#include <memory>

struct _object;
typedef _object PyObject;

class MetricRegistry {
public:

	MetricRegistry();

	// MakeCounter, MakeGauge, etc

	void Serve(const char* bind_address);
	void StopServing();

	static void RegisterPythonObject(PyObject* module);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

#endif

