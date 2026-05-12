// Copyright © 2019 CCP ehf.

#include "metric_factory.h"
using namespace prometheus_module;

#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>

#include "counter.h"
#include "gauge.h"
#include "histogram.h"
#include "summary.h"

struct MetricFactory::Private {
	static std::string GetHashKey(const std::string& name, const std::map<std::string, std::string>& labels);
	static std::map<std::string, std::string> GetLabelNames(const std::map<std::string, std::string>& labels);

	std::shared_ptr<prometheus::Registry> registry;

	std::mutex factory_mutex;

	typedef prometheus::Family<prometheus::Counter> CounterFamily;
	std::unordered_map<std::string, CounterFamily*> counter_families;
	std::unordered_map<std::string, std::unique_ptr<prometheus_module::Counter> > counters;

	typedef prometheus::Family<prometheus::Gauge> GaugeFamily;
	std::unordered_map<std::string, GaugeFamily*> gauge_families;
	std::unordered_map<std::string, std::unique_ptr<prometheus_module::Gauge> > gauges;

	typedef prometheus::Family<prometheus::Histogram> HistogramFamily;
	std::unordered_map<std::string, HistogramFamily*> histogram_families;
	std::unordered_map<std::string, std::unique_ptr<prometheus_module::Histogram> > histograms;

	typedef prometheus::Family<prometheus::Summary> SummaryFamily;
	std::unordered_map<std::string, SummaryFamily*> summary_families;
	std::unordered_map<std::string, std::unique_ptr<prometheus_module::Summary> > summaries;
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

Counter& MetricFactory::MakeCounter(const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, MetricFactory::MakeMetricOption make_option, prometheus_module::Counter* wrapper) {
	std::lock_guard<std::mutex> lock(private_->factory_mutex);

	// If this metric already exists, then return it
	auto metric_hash = private_->GetHashKey(name, labels);
	if (make_option != MakeMetricOption::kPromoteFromLazy) {
		auto counter_iter = private_->counters.find(metric_hash);
		if (counter_iter != private_->counters.end()) {
			prometheus_module::Counter* result = counter_iter->second.get();
			return *result;
		}
	}

	// The metric doesn't exist yet, so see if its family does
	auto label_names_map = private_->GetLabelNames(labels);
	auto family_hash = private_->GetHashKey(name, label_names_map);
	auto family_iter = private_->counter_families.find(family_hash);
    prometheus::Family<prometheus::Counter>* family = nullptr;
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
	} else {
        family = family_iter->second;
    }

	// Create the metric (if not lazy) and create or update its wrapper
	if (make_option == MakeMetricOption::kLazy) {
		wrapper = new prometheus_module::Counter(*this, name, label_names, labels);
	}
	else {
		auto& prometheus_counter = family->Add(labels);

		if (make_option == MetricFactory::MakeMetricOption::kImmediate) {
			wrapper = new prometheus_module::Counter(prometheus_counter, *this, name, label_names);
		}
		else if (make_option == MakeMetricOption::kPromoteFromLazy && wrapper != nullptr) {
			wrapper->set_wrapped(prometheus_counter);
		}
	}

	// Store it
	if (make_option != MakeMetricOption::kPromoteFromLazy) {
		private_->counters.insert(std::make_pair(metric_hash, std::unique_ptr<prometheus_module::Counter>(wrapper))).first;
	}

	// Return it
	return *wrapper;
}

Gauge& MetricFactory::MakeGauge(const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, MetricFactory::MakeMetricOption make_option, prometheus_module::Gauge* wrapper) {
	std::lock_guard<std::mutex> lock(private_->factory_mutex);

	// If this metric already exists, then return it
	auto metric_hash = private_->GetHashKey(name, labels);
	if (make_option != MakeMetricOption::kPromoteFromLazy) {
		auto gauge_iter = private_->gauges.find(metric_hash);
		if (gauge_iter != private_->gauges.end()) {
			prometheus_module::Gauge *result = gauge_iter->second.get();
			return *result;
		}
	}

	// The metric doesn't exist yet, so see if its family does
	auto label_names_map = private_->GetLabelNames(labels);
	auto family_hash = private_->GetHashKey(name, label_names_map);
	auto family_iter = private_->gauge_families.find(family_hash);
	prometheus::Family<prometheus::Gauge>* family = nullptr;
	if (family_iter == private_->gauge_families.end()) {
		// If not, then create it
		std::map<std::string, std::string> empty_labels; ///< See MetricFactory::MakeCounter for an explanation of empty_labels
		family = &prometheus::BuildGauge().Name(name).Labels(empty_labels).Register(*private_->registry.get());
		private_->gauge_families.insert(std::make_pair(family_hash, family));
	} else {
        family = family_iter->second;
    }

	// Create the metric (if not lazy) and create or update its wrapper
	if (make_option == MakeMetricOption::kLazy) {
		wrapper = new prometheus_module::Gauge(*this, name, label_names, labels);
	}
	else {
		auto& prometheus_metric = family->Add(labels);
		std::vector<std::string> label_names_vec;
		for (auto& label_names_iter : label_names_map) {
			label_names_vec.push_back(label_names_iter.first);
		}

		if (make_option == MetricFactory::MakeMetricOption::kImmediate) {
			wrapper = new prometheus_module::Gauge(prometheus_metric, *this, name, label_names_vec);
		}
		else if (make_option == MakeMetricOption::kPromoteFromLazy && wrapper != nullptr) {
			wrapper->set_wrapped(prometheus_metric);
		}
	}

	// Store it
	if (make_option != MakeMetricOption::kPromoteFromLazy) {
		private_->gauges.insert(std::make_pair(metric_hash, std::unique_ptr<prometheus_module::Gauge>(wrapper))).first;
	}

	// Return it
	return *wrapper;
}

Histogram& MetricFactory::MakeHistogram(const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, const std::vector<double>& boundaries, MetricFactory::MakeMetricOption make_option, prometheus_module::Histogram* wrapper) {
	std::lock_guard<std::mutex> lock(private_->factory_mutex);

	// If this metric already exists, then return it
	auto metric_hash = private_->GetHashKey(name, labels);
	if (make_option != MakeMetricOption::kPromoteFromLazy) {
		auto histogram_iter = private_->histograms.find(metric_hash);
		if (histogram_iter != private_->histograms.end()) {
			prometheus_module::Histogram *result = histogram_iter->second.get();
			return *result;
		}
	}

	// The metric doesn't exist yet, so see if its family does
	auto label_names_map = private_->GetLabelNames(labels);
	auto family_hash = private_->GetHashKey(name, label_names_map);
	auto family_iter = private_->histogram_families.find(family_hash);
    prometheus::Family<prometheus::Histogram>* family = nullptr;
	if (family_iter == private_->histogram_families.end()) {
		// If not, then create it
		std::map<std::string, std::string> empty_labels; ///< See MetricFactory::MakeCounter for an explanation of empty_labels
		family = &prometheus::BuildHistogram().Name(name).Labels(empty_labels).Register(*private_->registry.get());
		private_->histogram_families.insert(std::make_pair(family_hash, family));
	} else {
        family = family_iter->second;
    }

	// Create the metric (if not lazy) and create or update its wrapper
	if (make_option == MakeMetricOption::kLazy) {
		wrapper = new prometheus_module::Histogram(*this, name, label_names, labels, boundaries);
	}
	else {
		auto& prometheus_metric = family->Add(labels, boundaries);
		std::vector<std::string> label_names_vec;
		for (auto& label_names_iter : label_names_map) {
			label_names_vec.push_back(label_names_iter.first);
		}

		if (make_option == MetricFactory::MakeMetricOption::kImmediate) {
			wrapper = new prometheus_module::Histogram(prometheus_metric, *this, name, label_names_vec, boundaries);
		}
		else if (make_option == MakeMetricOption::kPromoteFromLazy && wrapper != nullptr) {
			wrapper->set_wrapped(prometheus_metric);
		}
	}

	// Store it
	if (make_option != MakeMetricOption::kPromoteFromLazy) {
		private_->histograms.insert(std::make_pair(metric_hash, std::unique_ptr<prometheus_module::Histogram>(wrapper))).first;
	}

	// Return it
	return *wrapper;
}

Summary& MetricFactory::MakeSummary(const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, const std::vector<std::pair<double, double> >& quantiles, int total_window_size_seconds, int window_partitions, MetricFactory::MakeMetricOption make_option, prometheus_module::Summary* wrapper) {
	std::lock_guard<std::mutex> lock(private_->factory_mutex);

	// If this metric already exists, then return it
	auto metric_hash = private_->GetHashKey(name, labels);
	if (make_option != MakeMetricOption::kPromoteFromLazy) {
		auto metric_iter = private_->summaries.find(metric_hash);
		if (metric_iter != private_->summaries.end()) {
			prometheus_module::Summary* result = metric_iter->second.get();
			return *result;
		}
	}

	// The metric doesn't exist yet, so see if its family does
	auto label_names_map = private_->GetLabelNames(labels);
	auto family_hash = private_->GetHashKey(name, label_names_map);
	auto family_iter = private_->summary_families.find(family_hash);
	prometheus::Family<prometheus::Summary>* family = nullptr;
	if (family_iter == private_->summary_families.end()) {
		// If not, then create it
		std::map<std::string, std::string> empty_labels; ///< See MetricFactory::MakeCounter for an explanation of empty_labels
		family = &prometheus::BuildSummary().Name(name).Labels(empty_labels).Register(*private_->registry.get());
		private_->summary_families.insert(std::make_pair(family_hash, family));
	} else {
        family = family_iter->second;
    }

	// Convert to prometheus quantiles type
	prometheus::Summary::Quantiles quantiles_converted;
	if (!quantiles.empty()) {
		for (auto q : quantiles) {
			quantiles_converted.push_back(prometheus::detail::CKMSQuantiles::Quantile(q.first, q.second));
		}
	}

	// If the window is invalid or unspecified, default to a 5-minute window, split into 5 partitions (of 1 minute each)
	if (total_window_size_seconds <= 0) {
		total_window_size_seconds = 300;
	}

	if (window_partitions <= 0) {
		window_partitions = 5;
	}

	// Create the metric (if not lazy) and create or update its wrapper
	if (make_option == MakeMetricOption::kLazy) {
		wrapper = new prometheus_module::Summary(*this, name, label_names, labels, quantiles, total_window_size_seconds, window_partitions);
	}
	else {
		auto& prometheus_metric = family->Add(labels, quantiles_converted, std::chrono::seconds{ total_window_size_seconds / window_partitions }, window_partitions);
		std::vector<std::string> label_names_vec;
		for (auto& label_names_iter : label_names_map) {
			label_names_vec.push_back(label_names_iter.first);
		}

		if (make_option == MetricFactory::MakeMetricOption::kImmediate) {
			wrapper = new prometheus_module::Summary(prometheus_metric, *this, name, label_names_vec, quantiles, total_window_size_seconds, window_partitions);
		}
		else if (make_option == MakeMetricOption::kPromoteFromLazy && wrapper != nullptr) {
			wrapper->set_wrapped(prometheus_metric);
		}
	}

	// Store it
	if (make_option != MakeMetricOption::kPromoteFromLazy) {
		private_->summaries.insert(std::make_pair(metric_hash, std::unique_ptr<prometheus_module::Summary>(wrapper))).first;
	}

	// Return it
	return *wrapper;
}
