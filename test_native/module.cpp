#include <iostream>

// Python
#include <Python.h>

// gtest
#include <gtest/gtest.h>


// Python linkage

static PyObject* RunTests(PyObject* self, PyObject* param) {
	int argc = 1;
	char* argv[] = {"nothing.exe"};	//< gtest falsely accuses us of not calling InitGoogleTest if this is empty
	::testing::InitGoogleTest(&argc, (char**)argv);

	RUN_ALL_TESTS();

	Py_RETURN_TRUE;
}

static PyMethodDef ModuleMethods[] = {
	{"RunTests", RunTests, METH_NOARGS, "Run all the native tests"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC inittest_native(void) {
    Py_InitModule("test_native", ModuleMethods);
}
