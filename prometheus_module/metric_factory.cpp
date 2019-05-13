#include "metric_factory.h"
using namespace prometheus_module;

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/registry.h>

#include "counter.h"

struct MetricFactory::Private {
	static std::string GetHashKey(const std::string& name, const std::map<std::string, std::string>& labels);
	static std::map<std::string, std::string> GetLabelNames(const std::map<std::string, std::string>& labels);

	std::shared_ptr<prometheus::Registry> registry;

	typedef prometheus::Family<prometheus::Counter> CounterFamily;
	std::unordered_map<std::string, CounterFamily*> counter_families;
	std::unordered_map<std::string, std::unique_ptr<prometheus_module::Counter> > counters;
};

std::string MetricFactory::Private::GetHashKey(const std::string& name, const std::map<std::string, std::string>& labels) {
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

std::map<std::string, std::string> MetricFactory::Private::GetLabelNames(const std::map<std::string, std::string>& labels) {
	std::map<std::string, std::string> result;
	for (auto& iter : labels) {
		result.insert(std::make_pair(iter.first, ""));
	}
	return result;
}

MetricFactory::MetricFactory(std::shared_ptr<prometheus::Registry> registry) :
	private_(std::make_unique<Private>())
{
	private_->registry = registry;
}

MetricFactory::~MetricFactory() = default;

Counter& MetricFactory::MakeCounter(const std::string& name, const std::map<std::string, std::string>& labels) {
	// If this metric already exists, then return it
	auto metric_hash = private_->GetHashKey(name, labels);
	auto counter_iter = private_->counters.find(metric_hash);
	if (counter_iter != private_->counters.end()) {
		prometheus_module::Counter* result = counter_iter->second.get();
		return *result;
	}

	// The metric doesn't exist yet, so see if its family does
	auto label_names = private_->GetLabelNames(labels);
	auto family_hash = private_->GetHashKey(name, label_names);
	auto family_iter = private_->counter_families.find(family_hash);
	auto family = family_iter->second;
	if (family_iter == private_->counter_families.end()) {
		// If not, then create it
		family = &prometheus::BuildCounter().Name(name).Labels(label_names).Register(*private_->registry.get());
		private_->counter_families.insert(std::make_pair(family_hash, family));
	}

	// Create the metric
	auto& prometheus_counter = family->Add(labels);
	std::unique_ptr<prometheus_module::Counter> ptr = std::make_unique<prometheus_module::Counter>(prometheus_counter, *this);

	// Store it
	auto result_iter = private_->counters.insert(std::make_pair(metric_hash, std::move(ptr))).first;

	// Return it
	prometheus_module::Counter* result = result_iter->second.get();
	return *result;
}
