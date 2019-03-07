#include "metric_registry.h"

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
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

struct MetricRegistry::Private {
	std::unique_ptr<prometheus::Exposer> exposer_;
	std::shared_ptr<prometheus::Registry> registry_;
};

MetricRegistry::MetricRegistry() :
	private_(std::make_unique<Private>())
{
	private_->registry_ = std::make_shared<prometheus::Registry>();
}

void MetricRegistry::Serve(const char* bind_address) {
	StopServing();

	private_->exposer_ = std::make_unique<prometheus::Exposer>(bind_address, "");
}

void MetricRegistry::StopServing() {
	private_->exposer_ = nullptr;
}


// Python linkage

#include <Python.h>

typedef struct {
	PyObject_HEAD
	std::unique_ptr<MetricRegistry> metric_registry;
} MetricRegistryPyObject;

static int MetricRegistry_init(MetricRegistryPyObject *self, PyObject *args, PyObject *kwds) {
	self->metric_registry = std::make_unique<MetricRegistry>();
	return 0;
}

static void MetricRegistry_dealloc(MetricRegistryPyObject* self)
{
	self->metric_registry.reset(nullptr);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* MetricRegistry_Serve(MetricRegistryPyObject* self) {
	self->metric_registry->Serve("127.0.0.1:20800");
	return Py_None;
}

static PyObject* MetricRegistry_StopServing(MetricRegistryPyObject* self) {
	self->metric_registry->StopServing();
	return Py_None;
}

static PyMethodDef MetricRegistryPyMethods[] = {
	{"Serve", (PyCFunction)MetricRegistry_Serve, METH_NOARGS, "Start serving metrics at the specified address:port"},
	{"StopServing", (PyCFunction)MetricRegistry_StopServing, METH_NOARGS, "Stop serving metrics"},

	{NULL}  /* Sentinel */
};

static PyTypeObject MetricRegistryPyType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.MetricRegistry",             /* tp_name */
	sizeof(MetricRegistryPyObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)MetricRegistry_dealloc,        /* tp_dealloc */
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
	"Metric registry, factory, and http handler",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	MetricRegistryPyMethods,   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)MetricRegistry_init, /* tp_init */
	0,                         /* tp_alloc */
	0,                         /* tp_new */
};

void MetricRegistry::RegisterPythonObject(PyObject* module) {
	MetricRegistryPyType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&MetricRegistryPyType) < 0)
		return;

	Py_INCREF(&MetricRegistryPyType);
	PyModule_AddObject(module, "MetricRegistry", (PyObject *)&MetricRegistryPyType);
}
