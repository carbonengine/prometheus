#include "counter.h"

// STL
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <thread>

// Python
#include <Python.h>

// Prometheus
#include <prometheus/counter.h>

struct Counter::Private {
	Private(prometheus::Counter& counter) :
		counter_(counter)
	{
	}

	prometheus::Counter& counter_;
};

Counter::Counter(prometheus::Counter& counter) :
	private_(std::make_unique<Private>(counter))
{
}

void Counter::Increment() {
	private_->counter_.Increment();
}

void Counter::Increment(double value) {
	private_->counter_.Increment(value);
}


// Python linkage

#include <Python.h>

typedef struct {
	PyObject_HEAD
	std::unique_ptr<Counter> counter;
} CounterPyObject;

static int Counter_init(CounterPyObject *self, PyObject *args, PyObject *kwds) {
	// self->counter must be set externally since its constructor needs a reference to a native type
	// This class is only intended to be instantiated by MetricRegistry, which is responsible for handling this.
	return 0;
}

static void Counter_dealloc(CounterPyObject* self) {
	self->counter.reset(nullptr);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* Counter_Increment(CounterPyObject* self, PyObject* py_value) {
	if (PyFloat_Check(py_value)) {
		double value = PyFloat_AsDouble(py_value);
		self->counter->Increment(value);
	}
	else {
		self->counter->Increment();
	}

	Py_RETURN_TRUE;
}

static PyMethodDef CounterPyMethods[] = {
	{"Increment", (PyCFunction)Counter_Increment, METH_O, "Increment the counter. Optionally, specify a value to increment by (default 1.0)."},

	{NULL}  /* Sentinel */
};

static PyTypeObject CounterPyType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.Counter",             /* tp_name */
	sizeof(CounterPyObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Counter_dealloc,        /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_compare */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Counter metric for prometheus_module",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	CounterPyMethods,   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Counter_init, /* tp_init */
	0,                         /* tp_alloc */
	0,                         /* tp_new */
};

void Counter::RegisterPythonObject(PyObject* module) {
	CounterPyType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&CounterPyType) < 0)
		return;

	Py_INCREF(&CounterPyType);
	PyModule_AddObject(module, "Counter", (PyObject *)&CounterPyType);
}

