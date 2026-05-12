// Copyright © 2019 CCP ehf.

// STL
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <thread>

// Prometheus
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

// prometheus_module
#include "metric_registry.h"
#include "counter.h"
#include "gauge.h"
#include "histogram.h"
#include "summary.h"
using namespace prometheus_module;


// Python linkage

static PyMethodDef ModuleMethods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef prometheusModule = {
	PyModuleDef_HEAD_INIT,
	"prometheus",
	nullptr,
	-1,
	ModuleMethods
};

#define CCP_CONCATENATE_DIRECT(s1, s2) s1##s2
#define CCP_CONCATENATE(s1, s2) CCP_CONCATENATE_DIRECT(s1, s2)

PyMODINIT_FUNC CCP_CONCATENATE(PyInit_prometheus_module, CCP_BUILD_FLAVOR)(void)
{
	PyObject* module = PyModule_Create(&prometheusModule);

	MetricRegistry::RegisterPythonObject(module);
	Counter::RegisterPythonObject(module);
	Gauge::RegisterPythonObject(module);
	Histogram::RegisterPythonObject(module);
	Summary::RegisterPythonObject(module);

	return module;
}

