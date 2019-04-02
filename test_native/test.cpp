#include <iostream>

#include <gtest/gtest.h>

#include <Python.h>

#include "metric_registry_interface.h"
#include "counter_interface.h"
#include "gauge_interface.h"
#include "histogram_interface.h"
#include "summary_interface.h"
using namespace prometheus_module;

#include "test.h"

prometheus_module::MetricRegistryInterface* MetricTestFixture::registry = nullptr;

void MetricTestFixture::SetUp() {
	std::cout << "MetricTestFixture::SetUp" << std::endl;
	ASSERT_NE(registry, nullptr);
}

TEST_F(MetricTestFixture, SomeTest) {
	std::cout << "MetricTestFixture::SomeTest" << std::endl;
	EXPECT_EQ(1, 1);
}

