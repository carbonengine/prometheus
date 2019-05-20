#ifndef COUNTER_H
#define COUNTER_H

#include <memory>

#include <prometheus/counter.h>

#include "counter_interface.h"

struct _object;
typedef _object PyObject;

namespace prometheus_module {
	class MetricFactory;
} //namespace prometheus_module

namespace prometheus_module {

class Counter : public CounterInterface {
public:

	Counter(prometheus::Counter& counter, prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& labels);
	~Counter();

	void Increment() override;
	void Increment(double value) override;

	CounterInterface* WithLabelValues(const char* values[], int num_values) override;

	Counter* WithLabelValues(std::vector<std::string> values);
	const std::string& name();
	const std::vector<std::string>& label_names();

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Counter* wrapped);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
