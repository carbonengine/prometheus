#include "metric_manager.h"

#include <sstream>

#include <prometheus/family.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/summary.h>

namespace prometheus_module {

template<typename FamilyType, typename ModuleType>
std::string MetricManager<FamilyType, ModuleType>::GetHashKey(const std::string& name, const std::map<std::string, std::string> labels) {
	std::stringstream ss;
	ss << name;

	for (auto& elem : labels) {
		ss << "|";
		ss << elem.first;
		ss << "=";
		ss << elem.second;
	}

	return ss.str();
}

template<typename FamilyType, typename ModuleType>
void MetricManager<FamilyType, ModuleType>::AddFamily(const std::string& hash_key, FamilyType& family) {
	families_.insert(std::make_pair(hash_key, family));
}

template<typename FamilyType, typename ModuleType>
FamilyType* MetricManager<FamilyType, ModuleType>::GetFamily(const std::string& hash_key) {
	auto iter = families_.find(hash_key);
	if (iter == families_.end()) {
		return nullptr;
	}
	FamilyType& family = *iter;
	return &family;
}

template<typename FamilyType, typename ModuleType>
void MetricManager<FamilyType, ModuleType>::AddMetric(const std::string& hash_key, ModuleType& metric) {
	metrics_.insert(std::make_pair(hash_key, metric));
}

template<typename FamilyType, typename ModuleType>
ModuleType* MetricManager<FamilyType, ModuleType>::GetMetric(const std::string& hash_key) {
	auto iter = metrics_.find(hash_key);
	if (iter == metrics_.end()) {
		return nullptr;
	}
	MetricType& metric = *iter;
	return &metric;
}


// Provide instantiations for the types prometheus_module will actually use.
// This way, we don't need to put the implementation in the header (in other words, so that this cpp file can exist meaningfully).
// It's okay because we know all the template parameters we'll ever use beforehand.
template class MetricManager<prometheus::Family<prometheus::Counter>, prometheus_module::Counter>;
template class MetricManager<prometheus::Family<prometheus::Gauge>, prometheus_module::Gauge>;
template class MetricManager<prometheus::Family<prometheus::Histogram>, prometheus_module::Histogram>;
template class MetricManager<prometheus::Family<prometheus::Summary>, prometheus_module::Summary>;

} // namespace prometheus_module
