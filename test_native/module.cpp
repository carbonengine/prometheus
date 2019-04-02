#include <iostream>

// Python
#include <Python.h>

// gtest
#include <gtest/gtest.h>

// prometheus_module
#include "metric_registry_interface.h"

// test_native
#include "test.h"


// Python linkage

static PyObject* RunTests(PyObject *self, PyObject *args, PyObject *kwds) {
	PyObject* capsule = NULL;

	static char *kwlist[] = { "capsule", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &capsule))
		Py_RETURN_FALSE;

	prometheus_module::MetricRegistryInterface* registry = (MetricRegistryInterface*)PyCapsule_GetPointer(capsule, NULL);
	MetricTestFixture::registry = registry;

	int argc = 1;
	char* argv[] = {"nothing.exe"};	//< gtest falsely accuses us of not calling InitGoogleTest if this is empty
	::testing::InitGoogleTest(&argc, (char**)argv);

	auto num_failures = RUN_ALL_TESTS();
	std::cout << "RUN_ALL_TESTS returned " << num_failures << std::endl;
	if (num_failures) {
		Py_RETURN_FALSE;
	}

	Py_RETURN_TRUE;
}

static PyMethodDef ModuleMethods[] = {
	{"RunTests", (PyCFunction)RunTests, METH_VARARGS | METH_KEYWORDS, "Run all the native tests"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC inittest_native(void) {
    Py_InitModule("test_native", ModuleMethods);
}
