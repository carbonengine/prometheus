#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
std::string TestBase::default_port = "20800";

void TestBase::StaticInitialize(MetricRegistryInterface* r) {
	registry = r;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	srand(1111);
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

std::string TestBase::FetchLine(std::string substr, std::string port) {
	std::vector<std::string> lines = FetchLines(substr, port);
	if (!lines.empty()) {
		return lines[0];
	}
	return "";
}

std::vector<std::string> TestBase::FetchLines(std::string substr, std::string port) {
	if (port.empty()) {
		port = default_port;
	}

	std::string page = Fetch(port);

	std::stringstream ss(page);
	std::string line;
	std::vector<std::string> lines;
	while (std::getline(ss, line, '\n')) {
		if (substr.empty() || ((line.find(substr) != std::string::npos) && (line.find("#") == std::string::npos))) {
			lines.push_back(std::move(line));
		}
	}

	return lines;
}

bool TestBase::IsServerListening(std::string port) {
	if (port.empty()) {
		port = default_port;
	}

	std::string page = Fetch(port);

	if (page.find("exposer") != std::string::npos) {
		return true;
	}

	return false;
}

std::string TestBase::RandomString(int length) {
	std::string result = "";
	for (int i = 0; i < length; i++) {
		result += ('A' + (rand() % 26));
	}
	return result;
}


//
// Serving
//
class TestServing : public TestBase {
protected:

	void ExpectServeSuccess(std::string port="") {
		EXPECT_FALSE(IsServerListening(port));
		EXPECT_TRUE(registry->Serve(port.c_str()));
		registry->StopServing();
		EXPECT_FALSE(IsServerListening(port));
	}

	void ExpectServeFailure(std::string port = "") {
		EXPECT_FALSE(registry->Serve(port.c_str()));
	}
};

TEST_F(TestServing, ServerStartStop) {
	EXPECT_FALSE(IsServerListening());
	EXPECT_TRUE(registry->Serve(default_port.c_str()));
	EXPECT_TRUE(IsServerListening());
	registry->StopServing();
	EXPECT_FALSE(IsServerListening());
}

TEST_F(TestServing, BadPortFormats) {
	ExpectServeFailure("invalid_string");
	ExpectServeFailure("http://localhost:20800"); // must not contain the protocol prefix
	ExpectServeFailure("localhost:20800"); // does not support hostnames
	ExpectServeFailure(":20800"); // must not prefix the port with a colon unless an ip address is specified
	ExpectServeFailure("");
}

TEST_F(TestServing, GoodPortFormats) {
	ExpectServeSuccess("20800");
	ExpectServeSuccess("127.0.0.1:20800");
	ExpectServeSuccess("[::]:20800");
	// todo: test ssl (specify port with a trailing 's', e.g. '443s')
	// todo: test multiple ports in one string (separate ports with a comma, e.g. '20800,20801,[::]:20800', each gets its own socket)
	// todo: test ipv4 and ipv6 in one socket (specify port with a leading '+', e.g. '+20800', one socket serves both)
}

