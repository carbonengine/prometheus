#ifndef SUMMARY_H
#define SUMMARY_H

#include <memory>

#include <prometheus/summary.h>

#include "summary_interface.h"

struct _object;
typedef _object PyObject;

namespace prometheus_module {

class Summary : public SummaryInterface {
public:

	Summary(prometheus::Summary& summary);

	void Observe(double value) override;

	static void RegisterPythonObject(PyObject* module);
	static PyObject* CreatePythonObject(Summary* wrapped);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

} //namespace prometheus_module

#endif
