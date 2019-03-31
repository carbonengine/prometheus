#ifndef SUMMARYINTERFACE_H
#define SUMMARYINTERFACE_H

namespace prometheus_module {

class SummaryInterface {
public:

	virtual void Observe(double value) = 0;
};

} // namespace prometheus_module

#endif
