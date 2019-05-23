#ifndef SUMMARYINTERFACE_H
#define SUMMARYINTERFACE_H

namespace prometheus_module {

class SummaryInterface {
public:

	virtual void Observe(double value) = 0;

	virtual SummaryInterface* WithLabelValues(const char* values[], int num_values) = 0;
};

} // namespace prometheus_module

#endif
