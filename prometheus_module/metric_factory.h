#ifndef METRIC_FACTORY_H
#define METRIC_FACTORY_H

#include <map>
#include <memory>
#include <string>

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

	Counter& MakeCounter(const std::string& name, const std::map<std::string, std::string>& labels);
	Gauge& MakeGauge(const std::string& name, const std::map<std::string, std::string>& labels);
	//Histogram& MakeHistogram(/*stuff*/);
	//Summary& MakeSummary(/*stuff*/);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif //METRIC_FACTORY_H