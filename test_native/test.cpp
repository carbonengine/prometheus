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

void MetricTestFixture::SetUp() {
	ASSERT_NE(registry, nullptr);
}

void MetricTestFixture::TearDown() {
}

TEST_F(MetricTestFixture, SomeTest) {
	EXPECT_EQ(1, 1);
}

