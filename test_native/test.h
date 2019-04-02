#ifndef TEST_H
#define TEST_H

#include <gtest/gtest.h>

#include "metric_registry_interface.h"
using namespace prometheus_module;

class MetricTestFixture : public ::testing::Test {
public:

	static prometheus_module::MetricRegistryInterface* registry;

protected:

	void SetUp() override;
};

#endif
