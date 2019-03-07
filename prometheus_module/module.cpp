// STL
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <thread>

// Python
#include <Python.h>

static PyObject* PrometheusError;

// Prometheus
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

// Project
#include "metric_registry.h"


// Hello world

static PyObject* hello_world(PyObject* self, PyObject* param) {
	std::cout << "hello from the module" << std::endl;
	return PyInt_FromLong(0L);
}



// Python linkage

static PyMethodDef ModuleMethods[] = {
	{"hello_world", hello_world, METH_NOARGS, "Hello world"},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC initprometheus_module(void) {
    PyObject *m = Py_InitModule("prometheus_module", ModuleMethods);
    if (m == NULL)
        return;

    PrometheusError = PyErr_NewException("prometheus.error", NULL, NULL);
    Py_INCREF(PrometheusError);
    PyModule_AddObject(m, "error", PrometheusError);

	MetricRegistry::RegisterPythonObject(m);
}

