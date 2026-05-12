// Copyright © 2019 CCP ehf.

#ifndef SUMMARY_H
#define SUMMARY_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <prometheus/summary.h>

#include "summary_interface.h"

struct _object;
typedef _object PyObject;

namespace prometheus_module {
	class MetricFactory;
} //namespace prometheus_module

namespace prometheus_module {

class Summary : public SummaryInterface {
public:

	Summary(prometheus::Summary& summary, prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& labels, const std::vector<std::pair<double, double> >& quantiles, int total_window_size_seconds, int window_partitions);
	Summary(prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, const std::vector<std::pair<double, double> >& quantiles, int total_window_size_seconds, int window_partitions);
	~Summary();

	void Observe(double value) override;

	SummaryInterface* WithLabelValues(int num_values, const char* values[]) override;

	Summary* WithLabelValues(std::vector<std::string> values);
	const std::string& name();
	const std::vector<std::string>& label_names();

	void set_wrapped(prometheus::Summary& wrapped);

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Summary* wrapped, PyObject* family=nullptr);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
