// Copyright © 2019 CCP ehf.

#ifndef SUMMARYINTERFACE_H
#define SUMMARYINTERFACE_H

namespace prometheus_module {

class SummaryInterface {
public:

	virtual void Observe(double value) = 0;

	virtual SummaryInterface* WithLabelValues(int num_values, const char* values[]) = 0;
};

} // namespace prometheus_module

#endif
