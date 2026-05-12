// Copyright © 2019 CCP ehf.

#include "utilities.h"
using namespace prometheus_module;

#include "Python.h"

std::string Utilities::ConvertString(PyObject* str) {
	std::string result;

	if (str == nullptr)
	{
		return result;
	}

	if (PyUnicode_Check(str))
	{
		result = PyUnicode_AsUTF8(str);
	}
	else if (PyBytes_Check(str))
	{
		result = PyBytes_AsString(str);
	}

	return result;
}
