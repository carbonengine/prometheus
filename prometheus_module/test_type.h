#ifndef TESTTYPE_H
#define TESTTYPE_H

// STL
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <thread>

// Prometheus
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

// Python
#include <Python.h>

class TestType {
public:

	TestType();

	void Increment();

private:

	std::unique_ptr<prometheus::Exposer> exposer_;
	std::shared_ptr<prometheus::Registry> registry_;
	prometheus::Counter* counter_;
};

#endif