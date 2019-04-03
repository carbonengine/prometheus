#ifndef TEST_H
#define TEST_H

#include <string>

#include <gtest/gtest.h>

#include <curl/curl.h>

#include "metric_registry_interface.h"
using namespace prometheus_module;

class TestBase : public ::testing::Test {
public:

	static void StaticInitialize(prometheus_module::MetricRegistryInterface* r);
	static void StaticShutdown();

protected:

	static prometheus_module::MetricRegistryInterface* registry;
	static CURL* curl;

	void SetUp() override;

	std::string Fetch(std::string port);
};

#endif
