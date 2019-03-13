#ifndef GAUGE_H
#define GAUGE_H

#include <memory>

#include <prometheus/gauge.h>

struct _object;
typedef _object PyObject;

namespace prometheus_module {

class Gauge {
public:

	Gauge(prometheus::Gauge& gauge);

	void Increment();
	void Increment(double value);

	void Decrement();
	void Decrement(double value);

	void Set(double value);

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Gauge* wrapped);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
