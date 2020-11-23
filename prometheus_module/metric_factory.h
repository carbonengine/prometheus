#ifndef METRIC_FACTORY_H
#define METRIC_FACTORY_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace prometheus {
	class Registry;
}

namespace prometheus_module {
	class Counter;
	class Gauge;
	class Histogram;
	class Summary;
}

namespace prometheus_module {

class MetricFactory {
public:

	MetricFactory(std::shared_ptr<prometheus::Registry> registry);
	~MetricFactory();

	enum class MakeMetricOption {
		kImmediate,
		kLazy,
		kPromoteFromLazy
	};

	Counter& MakeCounter(const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, MakeMetricOption make_option = MakeMetricOption::kImmediate, prometheus_module::Counter* wrapper=nullptr);
	Gauge& MakeGauge(const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, MakeMetricOption make_option = MakeMetricOption::kImmediate, prometheus_module::Gauge* wrapper=nullptr);
	Histogram& MakeHistogram(const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, const std::vector<double>& boundaries, MakeMetricOption make_option = MakeMetricOption::kImmediate, prometheus_module::Histogram* wrapper=nullptr);
	Summary& MakeSummary(const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, const std::vector<std::pair<double, double> >& quantiles, int total_window_size_seconds, int window_partitions, MakeMetricOption make_option = MakeMetricOption::kImmediate, prometheus_module::Summary* wrapper=nullptr);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif //METRIC_FACTORY_H