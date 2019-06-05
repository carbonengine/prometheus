#ifndef HISTOGRAMINTERFACE_H
#define HISTOGRAMINTERFACE_H

namespace prometheus_module {

class HistogramInterface {
public:

	virtual void Observe(double value) = 0;

	virtual HistogramInterface* WithLabelValues(int num_values, const char* values[]) = 0;
};

} // namespace prometheus_module

#endif
