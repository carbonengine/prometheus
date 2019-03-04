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
#include "test_type.h"


// Hello world

static PyObject* hello_world(PyObject* self, PyObject* param) {
	std::cout << "hello from the module" << std::endl;
	return PyInt_FromLong(0L);
}

static PyObject* test_object(PyObject* self, PyObject* param) {
	TestType test("127.0.0.1:20800");

	std::cout << "Incrementing once per second for 60 seconds" << std::endl;
	for (int i = 0; i < 60; i++) {
		std::cout << i + 1 << " ";
		test.Increment();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << std::endl;

	return PyInt_FromLong(0L);
}


// Python linkage

static PyMethodDef ModuleMethods[] = {
	{"hello_world", hello_world, METH_NOARGS, "Hello world"},

	{"test_object", test_object, METH_NOARGS, "Test object"},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC initprometheus_module(void) {
    PyObject *m = Py_InitModule("prometheus_module", ModuleMethods);
    if (m == NULL)
        return;

    PrometheusError = PyErr_NewException("prometheus.error", NULL, NULL);
    Py_INCREF(PrometheusError);
    PyModule_AddObject(m, "error", PrometheusError);

	TestType::RegisterPythonObject(m);
}
