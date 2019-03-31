#ifndef COUNTER_H
#define COUNTER_H

#include <memory>

#include <prometheus/counter.h>

#include "counter_interface.h"

struct _object;
typedef _object PyObject;

namespace prometheus_module {

class Counter : public CounterInterface {
public:

	Counter(prometheus::Counter& counter);

	void Increment() override;
	void Increment(double value) override;

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Counter* wrapped);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
