#ifndef COUNTER_H
#define COUNTER_H

#include <memory>

#include <prometheus/counter.h>

struct _object;
typedef _object PyObject;

class Counter {
public:

	Counter(prometheus::Counter& counter);

	void Increment();
	void Increment(double value);

	static void RegisterPythonObject(PyObject* module);

private:

	struct Private;
	std::unique_ptr<Private> private_;
};

#endif
