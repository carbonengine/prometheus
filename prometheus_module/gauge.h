#ifndef GAUGE_H
#define GAUGE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <prometheus/gauge.h>

#include "gauge_interface.h"

struct _object;
typedef _object PyObject;

namespace prometheus_module {
	class MetricFactory;
} //namespace prometheus_module

namespace prometheus_module {

class Gauge : public GaugeInterface {
public:

	Gauge(prometheus::Gauge& gauge, prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& labels);
	Gauge(prometheus_module::MetricFactory& factory, const std::string& name, const std::map<std::string, std::string>& labels);
	~Gauge();

	void Increment() override;
	void Increment(double value) override;

	void Decrement() override;
	void Decrement(double value) override;

	void Set(double value) override;

	GaugeInterface* WithLabelValues(const char* values[], int num_values) override;

	Gauge* WithLabelValues(std::vector<std::string> values);
	const std::string& name();
	const std::vector<std::string>& label_names();

	void set_wrapped(prometheus::Gauge& wrapped);

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Gauge* wrapped, PyObject* family=nullptr);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
