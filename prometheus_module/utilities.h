// Copyright © 2019 CCP ehf.

#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>

struct _object;
typedef _object PyObject;

namespace prometheus_module {

class Utilities {
public:

	static std::string ConvertString(PyObject* str);
};

} //namespace prometheus_module

#endif
