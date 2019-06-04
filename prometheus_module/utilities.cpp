#include "utilities.h"
using namespace prometheus_module;

#include "Python.h"

std::string Utilities::ConvertString(PyObject* str) {
	std::string result;

	if (str == nullptr) {
		return result;
	}

	if (PyUnicode_Check(str)) {
		PyObject* tmp = PyUnicode_AsUTF8String(str);
		if (tmp != nullptr) {
			result = PyString_AsString(tmp);
			Py_DecRef(tmp);
		}
	}
	else if (PyBytes_Check(str)) {
		result = PyBytes_AsString(str);
	}
	else if (PyString_Check(str)) {
		result = PyString_AsString(str);
	}

	return result;
}

