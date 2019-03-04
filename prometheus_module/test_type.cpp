#include "test_type.h"

TestType::TestType()
	: exposer_(std::make_unique<prometheus::Exposer>("127.0.0.1:20800")),
	  registry_(std::make_shared<prometheus::Registry>())
{
	std::map <std::string, std::string> labels = { {"label1", "value1"}, {"label2", "value2"} };
	auto& counter_family = prometheus::BuildCounter().Name("TestType Counter").Labels(labels).Register(*registry_);
	counter_ = &counter_family.Add(labels);

	exposer_->RegisterCollectable(registry_);
}

void TestType::Increment() {
	counter_->Increment();
}


// Python linkage

#include <Python.h>

typedef struct {
	PyObject_HEAD
		/* Type-specific fields go here. */
} MetricTestObject;

static PyTypeObject MetricTestType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.MetricTest",             /* tp_name */
	sizeof(MetricTestObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	0,                         /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_compare */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Metric test object",      /* tp_doc */
};

static PyMethodDef MetricTestMethods[] = {
	{NULL}  /* Sentinel */
};

void TestType::RegisterPythonObject(PyObject* module) {
	MetricTestType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&MetricTestType) < 0)
		return;

	PyModule_AddObject(module, "MetricTest", (PyObject *)&MetricTestType);
}
