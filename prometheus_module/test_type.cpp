#include "test_type.h"

TestType::TestType()
	: exposer_(std::make_unique<prometheus::Exposer>("127.0.0.1:20800")),
	  registry_(std::make_shared<prometheus::Registry>())
{
	std::map <std::string, std::string> labels = { {"label1", "value1"}, {"label2", "value2"} };
	auto& counter_family = prometheus::BuildCounter().Name("TestType Counter").Labels(labels).Register(*registry_);
	counter_ = &counter_family.Add(labels);

	exposer_->RegisterCollectable(registry_);
}

void TestType::Increment() {
	counter_->Increment();
}
