// Copyright © 2019 CCP ehf.

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <prometheus/histogram.h>

#include "histogram_interface.h"

struct _object;
typedef _object PyObject;

namespace prometheus_module {
	class MetricFactory;
} //namespace prometheus_module

namespace prometheus_module {

class Histogram : public HistogramInterface {
public:

	Histogram(prometheus::Histogram& histogram, prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& labels, const std::vector<double>& boundaries);
	Histogram(prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, const std::vector<double>& boundaries);
	~Histogram();

	void Observe(double value) override;

	HistogramInterface* WithLabelValues(int num_values, const char* values[]) override;

	Histogram* WithLabelValues(std::vector<std::string> values);
	const std::string& name();
	const std::vector<std::string>& label_names();

	void set_wrapped(prometheus::Histogram& wrapped);

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Histogram* wrapped, PyObject* family=nullptr);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
