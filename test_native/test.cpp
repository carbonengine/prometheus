#include <iostream>

#include <gtest/gtest.h>

#include <curl/curl.h>

#include <Python.h>

#include "metric_registry_interface.h"
#include "counter_interface.h"
#include "gauge_interface.h"
#include "histogram_interface.h"
#include "summary_interface.h"
using namespace prometheus_module;

#include "test.h"

prometheus_module::MetricRegistryInterface* TestBase::registry = nullptr;
CURL* TestBase::curl = nullptr;

void TestBase::StaticInitialize(MetricRegistryInterface* r) {
	registry = r;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
}

void TestBase::StaticShutdown() {
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

void TestBase::SetUp() {
	ASSERT_NE(registry, nullptr);
	ASSERT_NE(curl, nullptr);
}

size_t write_callback(char* contents, size_t size, size_t nmemb, void* output_buffer_void) {
	auto real_size = size * nmemb;
	auto output_buffer = static_cast<std::string*>(output_buffer_void);
	output_buffer->append((char*)contents, real_size);
	return real_size;
}

std::string TestBase::Fetch(std::string port) {
	std::string url = "http://localhost:" + port;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

	std::string output_buffer;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output_buffer);

	char error_buffer[CURL_ERROR_SIZE];
	error_buffer[0] = '\0';
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

	long http_code = 0;
	CURLcode res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	return output_buffer;
}

TEST_F(TestBase, SomeTest) {
	registry->Serve("20800");
	std::cout << Fetch("20800") << std::endl;
	registry->StopServing();
	EXPECT_EQ(1, 1);
}

