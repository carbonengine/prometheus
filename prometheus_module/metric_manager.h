#ifndef METRIC_MANAGER_H
#define METRIC_MANAGER_H

#include <map>
#include <string>
#include <unordered_map>

namespace prometheus {
	template<typename T>
	class Family;

	class Counter;
	class Gauge;
	class Histogram;
	class Summary;
}

namespace prometheus_module {
	class Counter;
	class Gauge;
	class Histogram;
	class Summary;
}


namespace prometheus_module {

template<typename FamilyType, typename ModuleType>
class MetricManager {
public:

	static std::string GetHashKey(const std::string& name, const std::map<std::string, std::string> labels);

	static void AddFamily(const std::string& hash_key, FamilyType& family);
	static FamilyType* GetFamily(const std::string& hash_key);

	void AddMetric(const std::string& hash_key, ModuleType& metric);
	ModuleType* GetMetric(const std::string& hash_key);

private:

	static std::unordered_map<std::string, FamilyType&> families_;
	static std::unordered_map<std::string, ModuleType&> metrics_;
};

typedef MetricManager<prometheus::Family<prometheus::Counter>, prometheus_module::Counter> CounterManager;
typedef MetricManager<prometheus::Family<prometheus::Gauge>, prometheus_module::Gauge> GaugeManager;
typedef MetricManager<prometheus::Family<prometheus::Histogram>, prometheus_module::Histogram> HistogramManager;
typedef MetricManager<prometheus::Family<prometheus::Summary>, prometheus_module::Summary> SummaryManager;

} // namespace prometheus_module

#endif // METRIC_MANAGER_H