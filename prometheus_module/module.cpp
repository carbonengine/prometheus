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

// Project
#include "metric_registry.h"
#include "counter.h"
using namespace prometheus_module;


// Python linkage

static PyMethodDef ModuleMethods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC initprometheus_module(void) {
    PyObject *m = Py_InitModule("prometheus_module", ModuleMethods);
    if (m == NULL)
        return;

	MetricRegistry::RegisterPythonObject(m);
	Counter::RegisterPythonObject(m);
}

