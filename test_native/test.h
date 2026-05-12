// Copyright © 2019 CCP ehf.

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
	static std::string default_port;

	void SetUp() override;

	std::string Fetch(std::string port);
	std::string FetchLine(std::string substr, std::string port="");
	std::vector<std::string> FetchLines(std::string substr, std::string port="");
	bool IsServerListening(std::string port = "");
	std::string RandomString(int length = 6);
};

#endif
