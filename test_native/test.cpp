#include <array>
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

std::vector<std::string> string_split(std::string str, char delimiter) {
	std::stringstream ss(str);
	std::string line;
	std::vector<std::string> lines;
	while (std::getline(ss, line, delimiter)) {
		lines.push_back(std::move(line));
	}
	return lines;
}

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
	auto page_lines = string_split(page, '\n');

	std::vector<std::string> result;
	for (auto line : page_lines) {
		if (substr.empty() || ((line.find(substr) != std::string::npos) && (line.find("#") == std::string::npos))) {
			result.push_back(std::move(line));
		}
	}

	return result;
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


//
// Counter
//
class TestCounter : public TestBase {
protected:

	void SetUp() {
		TestBase::SetUp();
		registry->Serve(default_port.c_str());
	}

	void TearDown() {
		TestBase::TearDown();
		registry->StopServing();
	}

	int FetchCounter(std::string name) {
		std::string line = FetchLine(name);
		auto split = string_split(line, ' ');
		if (line.empty() || split.empty()) {
			return 0;
		}
		return std::stoi(split.back());
	}
};

TEST_F(TestCounter, MakeCounter) {
	auto n = RandomString();
	EXPECT_TRUE(FetchLines(n).empty());
	registry->MakeCounter(n.c_str(), 0, nullptr, nullptr);
	EXPECT_FALSE(FetchLines(n).empty());
}

TEST_F(TestCounter, MakeCounterWithLabels) {
	auto n = RandomString();
	const char* label_names[] = { RandomString().c_str(), RandomString().c_str() };
	const char* label_values[] = { RandomString().c_str(), RandomString().c_str() };
	registry->MakeCounter(n.c_str(), 2, label_names, label_values);

	auto line = FetchLine(n);
	EXPECT_TRUE(line.find(label_names[0]) != std::string::npos);
	EXPECT_TRUE(line.find(label_names[1]) != std::string::npos);
	EXPECT_TRUE(line.find(label_values[0]) != std::string::npos);
	EXPECT_TRUE(line.find(label_values[1]) != std::string::npos);
}

TEST_F(TestCounter, Increment) {
	auto n = RandomString();
	auto c = registry->MakeCounter(n.c_str(), 0, nullptr, nullptr);
	EXPECT_EQ(FetchCounter(n), 0);
	c->Increment();
	EXPECT_EQ(FetchCounter(n), 1);
	c->Increment(10);
	EXPECT_EQ(FetchCounter(n), 11);
}

TEST_F(TestCounter, DecrementFails) {
	auto n = RandomString();
	auto c = registry->MakeCounter(n.c_str(), 0, nullptr, nullptr);
	EXPECT_EQ(FetchCounter(n), 0);
	c->Increment(-1);
	EXPECT_EQ(FetchCounter(n), 0);
}


//
// Gauge
//

class TestGauge : public TestBase {
protected:

	void SetUp() {
		TestBase::SetUp();
		registry->Serve(default_port.c_str());
	}

	void TearDown() {
		TestBase::TearDown();
		registry->StopServing();
	}

	float FetchGauge(std::string name) {
		std::string line = FetchLine(name);
		auto split = string_split(line, ' ');
		if (line.empty() || split.empty()) {
			return 0;
		}
		return std::stof(split.back());
	}
};

TEST_F(TestGauge, MakeGauge) {
	auto n = RandomString();
	EXPECT_TRUE(FetchLines(n).empty());
	registry->MakeGauge(n.c_str(), 0, nullptr, nullptr);
	EXPECT_FALSE(FetchLines(n).empty());
}

TEST_F(TestGauge, MakeGaugeWithLabels) {
	auto n = RandomString();
	const char* label_names[] = { RandomString().c_str(), RandomString().c_str() };
	const char* label_values[] = { RandomString().c_str(), RandomString().c_str() };
	registry->MakeGauge(n.c_str(), 2, label_names, label_values);

	auto line = FetchLine(n);
	EXPECT_TRUE(line.find(label_names[0]) != std::string::npos);
	EXPECT_TRUE(line.find(label_names[1]) != std::string::npos);
	EXPECT_TRUE(line.find(label_values[0]) != std::string::npos);
	EXPECT_TRUE(line.find(label_values[1]) != std::string::npos);
}

TEST_F(TestGauge, Increment) {
	auto n = RandomString();
	auto g = registry->MakeGauge(n.c_str(), 0, nullptr, nullptr);
	EXPECT_EQ(FetchGauge(n), 0);
	g->Increment();
	EXPECT_EQ(FetchGauge(n), 1);
	g->Increment(10);
	EXPECT_EQ(FetchGauge(n), 11);
}

TEST_F(TestGauge, Decrement) {
	auto n = RandomString();
	auto g = registry->MakeGauge(n.c_str(), 0, nullptr, nullptr);
	EXPECT_EQ(FetchGauge(n), 0);
	g->Decrement();
	EXPECT_EQ(FetchGauge(n), -1);
	g->Decrement(10);
	EXPECT_EQ(FetchGauge(n), -11);
}

TEST_F(TestGauge, Set) {
	auto n = RandomString();
	auto g = registry->MakeGauge(n.c_str(), 0, nullptr, nullptr);
	EXPECT_EQ(FetchGauge(n), 0);
	g->Set(999);
	EXPECT_EQ(FetchGauge(n), 999);
}

TEST_F(TestGauge, SetFloat) {
	auto n = RandomString();
	auto g = registry->MakeGauge(n.c_str(), 0, nullptr, nullptr);
	EXPECT_EQ(FetchGauge(n), 0);
	g->Set(999.9);
	EXPECT_FLOAT_EQ(FetchGauge(n), 999.9f);
}


//
// Histogram
//

class TestHistogram : public TestBase {
protected:

	struct Histogram {
		int count;
		float sum;
		std::vector<int> buckets;

		Histogram() {
			count = 0;
			sum = 0.f;
		}
	};

	void SetUp() {
		TestBase::SetUp();
		registry->Serve(default_port.c_str());
	}

	void TearDown() {
		TestBase::TearDown();
		registry->StopServing();
	}

	Histogram FetchHistogram(std::string name) {
		Histogram res;

		auto lines = FetchLines(name);
		if (lines.empty()) {
			return res;
		}

		for (auto line : lines) {
			if (line.find("_count") != std::string::npos) {
				res.count = std::stoi(string_split(line, ' ').back());
			}
			if (line.find("_sum") != std::string::npos) {
				res.sum = std::stof(string_split(line, ' ').back());
			}
			if (line.find("_bucket") != std::string::npos) {
				res.buckets.push_back(std::stoi(string_split(line, ' ').back()));
			}
		}

		return res;
	}
};

TEST_F(TestHistogram, MakeHistogram) {
	auto n = RandomString();
	EXPECT_TRUE(FetchLines(n).empty());
	registry->MakeHistogram(n.c_str(), 0, nullptr, nullptr, 0, nullptr);
	EXPECT_FALSE(FetchLines(n).empty());
}

TEST_F(TestHistogram, MakeHistogramWithLabels) {
	auto n = RandomString();
	const char* label_names[] = { RandomString().c_str(), RandomString().c_str() };
	const char* label_values[] = { RandomString().c_str(), RandomString().c_str() };
	registry->MakeHistogram(n.c_str(), 2, label_names, label_values, 0, nullptr);

	auto line = FetchLine(n);
	EXPECT_TRUE(line.find(label_names[0]) != std::string::npos);
	EXPECT_TRUE(line.find(label_names[1]) != std::string::npos);
	EXPECT_TRUE(line.find(label_values[0]) != std::string::npos);
	EXPECT_TRUE(line.find(label_values[1]) != std::string::npos);
}

TEST_F(TestHistogram, MakeHistogramWithBoundaries) {
	auto n = RandomString();
	double boundaries[] = { 10.0, 100.0, 1000.0 };
	auto h = registry->MakeHistogram(n.c_str(), 0, nullptr, nullptr, 3, boundaries);

	auto values = FetchHistogram(n);
	EXPECT_EQ(values.count, 0);
	EXPECT_FLOAT_EQ(values.sum, 0.f);
	EXPECT_EQ(values.buckets.size(), 4); // (-Inf, 10], (10, 100], (100, 1000], (1000, +Inf)
}

TEST_F(TestHistogram, Observe) {
	auto n = RandomString();
	double boundaries[] = { 10.0, 100.0, 1000.0 };
	auto h = registry->MakeHistogram(n.c_str(), 0, nullptr, nullptr, 3, boundaries);

	h->Observe(1.0);
	h->Observe(10.0);
	h->Observe(100.0);
	h->Observe(1000.0);
	h->Observe(10000.0);

	auto values = FetchHistogram(n);
	EXPECT_EQ(values.count, 5);
	EXPECT_FLOAT_EQ(values.sum, 1.0 + 10.0 + 100.0 + 1000.0 + 10000.0);
	ASSERT_EQ(values.buckets.size(), 4);
	EXPECT_EQ(values.buckets[0], 2);
	EXPECT_EQ(values.buckets[1], 3);
	EXPECT_EQ(values.buckets[2], 4);
	EXPECT_EQ(values.buckets[3], 5);
}
