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

// Python
#include <Python.h>

static PyObject* PrometheusError;


// Hello world

static PyObject* hello_world(PyObject* self, PyObject* param) {
	std::cout << "hello from the module" << std::endl;
	return PyInt_FromLong(0L);
}

prometheus::Exposer exposer("127.0.0.1:20800");
std::shared_ptr<prometheus::Registry> registry = std::make_shared<prometheus::Registry>();
prometheus::Counter* counter = nullptr;

static PyObject* test_setup(PyObject* self, PyObject* param) {
	std::cout << "test setup" << std::endl;

	std::map <std::string, std::string> labels = { {"label1", "value1"}, {"label2", "value2"} };
	auto& counter_family = prometheus::BuildCounter().Name("Test Counter").Labels(labels).Register(*registry);
	counter = &counter_family.Add(labels);

	exposer.RegisterCollectable(registry);

	return PyInt_FromLong(0L);
}

static PyObject* test_increment(PyObject* self, PyObject* param) {
	std::cout << "test increment" << std::endl;

	if (counter == nullptr) {
		std::cout << "null counter" << std::endl;
		return PyInt_FromLong(0L);
	}

	std::cout << "Incrementing once per second for 60 seconds" << std::endl;
	for (int i = 0; i < 60; i++) {
		std::cout << i + 1 << " ";
		counter->Increment();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << std::endl;

	return PyInt_FromLong(0L);
}


// Python linkage

static PyMethodDef ModuleMethods[] = {
	{"hello_world", hello_world, METH_NOARGS, "Hello world"},

	{"test_setup", test_setup, METH_NOARGS, "Test setup"},
	{"test_increment", test_increment, METH_NOARGS, "Test increment"},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC initprometheus_module(void) {
    PyObject *m = Py_InitModule("prometheus_module", ModuleMethods);
    if (m == NULL)
        return;

    PrometheusError = PyErr_NewException("prometheus.error", NULL, NULL);
    Py_INCREF(PrometheusError);
    PyModule_AddObject(m, "error", PrometheusError);
}
