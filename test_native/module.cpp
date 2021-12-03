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

static PyObject* RunTests(PyObject *, PyObject *args, PyObject *kwds) {
	PyObject* capsule = nullptr;

	static const char *kwlist[] = { "capsule", nullptr };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", (char**)kwlist, &capsule))
		Py_RETURN_FALSE;

	auto* registry = (MetricRegistryInterface*)PyCapsule_GetPointer(capsule, nullptr);
	TestBase::StaticInitialize(registry);

	int argc = 1;
	static const char* argv[] = {"nothing.exe"};	//< gtest falsely accuses us of not calling InitGoogleTest if this is empty
	::testing::InitGoogleTest(&argc, (char**)argv);

	auto has_failures = RUN_ALL_TESTS();

	TestBase::StaticShutdown();

	if (has_failures) {
		Py_RETURN_FALSE;
	}

	Py_RETURN_TRUE;
}

static PyMethodDef ModuleMethods[] = {
	{"RunTests", (PyCFunction)RunTests, METH_VARARGS | METH_KEYWORDS, "Run all the native tests"},
    {nullptr, nullptr, 0, nullptr}        /* Sentinel */
};

PyMODINIT_FUNC inittest_native(void) {
    Py_InitModule("test_native", ModuleMethods);
}
