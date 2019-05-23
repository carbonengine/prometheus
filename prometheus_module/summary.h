#ifndef SUMMARY_H
#define SUMMARY_H

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
	~Summary();

	void Observe(double value) override;

	SummaryInterface* WithLabelValues(const char* values[], int num_values) override;

	Summary* WithLabelValues(std::vector<std::string> values);
	const std::string& name();
	const std::vector<std::string>& label_names();

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Summary* wrapped, PyObject* family=nullptr);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
