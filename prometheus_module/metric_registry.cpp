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

static void MetricRegistry_dealloc(MetricRegistryPyObject* self) {
	self->metric_registry.reset(nullptr);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* MetricRegistry_Serve(MetricRegistryPyObject* self, PyObject* py_bind_address) {
	// todo: failure modes for args here
	char* bind_address = PyString_AsString(py_bind_address);
	
	// todo: validate bind_address
	//		 prometheus-cpp is not well-behaved with strings like ":1234" or "localhost:1234"
	//		 see following comment for valid string examples

	// Port string spec from CivetWeb:
	// https://github.com/civetweb/civetweb/blob/a714efa0a0f36607f70226f75269cd3f9361204b/src/civetweb.c#L14244
	//	* Valid listening port specification is: [ip_address:]port[s]
	//	* Examples for IPv4: 80, 443s, 127.0.0.1:3128, 192.0.2.3:8080s
	//	* Examples for IPv6: [::]:80, [::1]:80,
	//	*   [2001:0db8:7654:3210:FEDC:BA98:7654:3210]:443s
	//	*   see https://tools.ietf.org/html/rfc3513#section-2.2
	//	* In order to bind to both, IPv4 and IPv6, you can either add
	//	* both ports using 8080,[::]:8080, or the short form +8080.
	//	* Both forms differ in detail: 8080,[::]:8080 create two sockets,
	//	* one only accepting IPv4 the other only IPv6. +8080 creates
	//	* one socket accepting IPv4 and IPv6. Depending on the IPv6
	//	* environment, they might work differently, or might not work
	//	* at all - it must be tested what options work best in the
	//	* relevant network environment.

	self->metric_registry->Serve(bind_address);

	Py_RETURN_TRUE;
}

static PyObject* MetricRegistry_StopServing(MetricRegistryPyObject* self) {
	self->metric_registry->StopServing();
	Py_RETURN_TRUE;
}

static PyMethodDef MetricRegistryPyMethods[] = {
	{"Serve", (PyCFunction)MetricRegistry_Serve, METH_O, "Start serving metrics at the specified [ip:]port. To serve multiple ports, use comma separation: [ip:]port,[ip:]port[,...]"},
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

