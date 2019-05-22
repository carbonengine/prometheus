#include "metric_factory.h"
using namespace prometheus_module;

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

#include "counter.h"
#include "gauge.h"
#include "histogram.h"

struct MetricFactory::Private {
	static std::string GetHashKey(const std::string& name, const std::map<std::string, std::string>& labels);
	static std::map<std::string, std::string> GetLabelNames(const std::map<std::string, std::string>& labels);

	std::shared_ptr<prometheus::Registry> registry;

	typedef prometheus::Family<prometheus::Counter> CounterFamily;
	std::unordered_map<std::string, CounterFamily*> counter_families;
	std::unordered_map<std::string, std::unique_ptr<prometheus_module::Counter> > counters;

	typedef prometheus::Family<prometheus::Gauge> GaugeFamily;
	std::unordered_map<std::string, GaugeFamily*> gauge_families;
	std::unordered_map<std::string, std::unique_ptr<prometheus_module::Gauge> > gauges;

	typedef prometheus::Family<prometheus::Histogram> HistogramFamily;
	std::unordered_map<std::string, HistogramFamily*> histogram_families;
	std::unordered_map<std::string, std::unique_ptr<prometheus_module::Histogram> > histograms;
};

std::string MetricFactory::Private::GetHashKey(const std::string& name, const std::map<std::string, std::string>& labels) {
	std::stringstream ss;
	ss << name;

	// STL map is already ordered, so there's no need to sort the labels
	// We will get consistent keys regardless.

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

		// label_names is used in the family_hash so we get one family per distinct set of label keys,
		// but when we actually create the family in prometheus-cpp, we need to do it with an empty label set.
		// Each "family" in prometheus-cpp is actually a set of shared key+value pairs, so we would end up
		// with duplicate label keys with empty values in every metric belonging to this family if we used label_names here.
		// For example, a family with labels ["my_label"] and a metric with {"my_label"="my_value"} would end up looking like:
		// my_metric{my_label="",my_label="my_value"}
		// if we used label_names instead of empty_labels.
		std::map<std::string, std::string> empty_labels;
		family = &prometheus::BuildCounter().Name(name).Labels(empty_labels).Register(*private_->registry.get());
		private_->counter_families.insert(std::make_pair(family_hash, family));
	}

	// Create the metric
	auto& prometheus_counter = family->Add(labels);
	std::vector<std::string> label_names_vec;
	for (auto& label_names_iter : label_names) {
		label_names_vec.push_back(label_names_iter.first);
	}
	std::unique_ptr<prometheus_module::Counter> ptr = std::make_unique<prometheus_module::Counter>(prometheus_counter, *this, name, label_names_vec);

	// Store it
	auto result_iter = private_->counters.insert(std::make_pair(metric_hash, std::move(ptr))).first;

	// Return it
	prometheus_module::Counter* result = result_iter->second.get();
	return *result;
}

Gauge& MetricFactory::MakeGauge(const std::string& name, const std::map<std::string, std::string>& labels) {
	// If this metric already exists, then return it
	auto metric_hash = private_->GetHashKey(name, labels);
	auto gauge_iter = private_->gauges.find(metric_hash);
	if (gauge_iter != private_->gauges.end()) {
		prometheus_module::Gauge *result = gauge_iter->second.get();
		return *result;
	}

	// The metric doesn't exist yet, so see if its family does
	auto label_names = private_->GetLabelNames(labels);
	auto family_hash = private_->GetHashKey(name, label_names);
	auto family_iter = private_->gauge_families.find(family_hash);
	auto family = family_iter->second;
	if (family_iter == private_->gauge_families.end()) {
		// If not, then create it
		std::map<std::string, std::string> empty_labels; ///< See MetricFactory::MakeCounter for an explanation of empty_labels
		family = &prometheus::BuildGauge().Name(name).Labels(empty_labels).Register(*private_->registry.get());
		private_->gauge_families.insert(std::make_pair(family_hash, family));
	}

	// Create the metric
	auto& prometheus_metric = family->Add(labels);
	std::vector<std::string> label_names_vec;
	for (auto& label_names_iter : label_names) {
		label_names_vec.push_back(label_names_iter.first);
	}
	std::unique_ptr<prometheus_module::Gauge> ptr = std::make_unique<prometheus_module::Gauge>(prometheus_metric, *this, name, label_names_vec);

	// Store it
	auto result_iter = private_->gauges.insert(std::make_pair(metric_hash, std::move(ptr))).first;

	// Return it
	prometheus_module::Gauge* result = result_iter->second.get();
	return *result;
}

Histogram& MetricFactory::MakeHistogram(const std::string& name, const std::map<std::string, std::string>& labels, const std::vector<double>& boundaries) {
	// If this metric already exists, then return it
	auto metric_hash = private_->GetHashKey(name, labels);
	auto histogram_iter = private_->histograms.find(metric_hash);
	if (histogram_iter != private_->histograms.end()) {
		prometheus_module::Histogram *result = histogram_iter->second.get();
		return *result;
	}

	// The metric doesn't exist yet, so see if its family does
	auto label_names = private_->GetLabelNames(labels);
	auto family_hash = private_->GetHashKey(name, label_names);
	auto family_iter = private_->histogram_families.find(family_hash);
	auto family = family_iter->second;
	if (family_iter == private_->histogram_families.end()) {
		// If not, then create it
		std::map<std::string, std::string> empty_labels; ///< See MetricFactory::MakeCounter for an explanation of empty_labels
		family = &prometheus::BuildHistogram().Name(name).Labels(empty_labels).Register(*private_->registry.get());
		private_->histogram_families.insert(std::make_pair(family_hash, family));
	}

	// Create the metric
	auto& prometheus_metric = family->Add(labels, boundaries);
	std::vector<std::string> label_names_vec;
	for (auto& label_names_iter : label_names) {
		label_names_vec.push_back(label_names_iter.first);
	}
	std::unique_ptr<prometheus_module::Histogram> ptr = std::make_unique<prometheus_module::Histogram>(prometheus_metric, *this, name, label_names_vec, boundaries);

	// Store it
	auto result_iter = private_->histograms.insert(std::make_pair(metric_hash, std::move(ptr))).first;

	// Return it
	prometheus_module::Histogram* result = result_iter->second.get();
	return *result;
}
