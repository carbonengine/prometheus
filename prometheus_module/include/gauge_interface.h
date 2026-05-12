// Copyright © 2019 CCP ehf.

#ifndef GAUGEINTERFACE_H
#define GAUGEINTERFACE_H

namespace prometheus_module {

class GaugeInterface {
public:

	virtual void Increment() = 0;
	virtual void Increment(double value) = 0;

	virtual void Decrement() = 0;
	virtual void Decrement(double value) = 0;

	virtual void Set(double value) = 0;

	virtual GaugeInterface* WithLabelValues(int num_values, const char* values[]) = 0;
};

} // namespace prometheus_module

#endif
