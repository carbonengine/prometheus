#ifndef COUNTERINTERFACE_H
#define COUNTERINTERFACE_H

namespace prometheus_module {

class CounterInterface {
public:

	virtual void Increment() = 0;
	virtual void Increment(double value) = 0;

	virtual CounterInterface* WithLabelValues(const char* values[]) = 0;
};

} // namespace prometheus_module

#endif
