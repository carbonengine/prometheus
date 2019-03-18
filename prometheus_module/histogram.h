#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <memory>

#include <prometheus/histogram.h>

struct _object;
typedef _object PyObject;

namespace prometheus_module {

class Histogram {
public:

	Histogram(prometheus::Histogram& histogram);

	void Observe(double value);

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Histogram* wrapped);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
