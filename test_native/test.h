#ifndef TEST_H
#define TEST_H

#include <gtest/gtest.h>

#include "metric_registry_interface.h"
using namespace prometheus_module;

class MetricTestFixture : public ::testing::Test {
protected:

	void SetUp() override;
	void TearDown() override;

	static prometheus_module::MetricRegistryInterface* registry;
};

#endif
