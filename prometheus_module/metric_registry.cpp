#include "metric_registry.h"

// STL
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <thread>

// Python
#include <Python.h>

// Prometheus
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

struct MetricRegistry::Private {
	std::unique_ptr<prometheus::Exposer> exposer_;
	std::shared_ptr<prometheus::Registry> registry_;
};

MetricRegistry::MetricRegistry() :
	private_(std::make_unique<Private>())
{
	private_->registry_ = std::make_shared<prometheus::Registry>();
}
