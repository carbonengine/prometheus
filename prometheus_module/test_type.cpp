#include "test_type.h"

TestType::TestType(const char* endpoint)
	: exposer_(std::make_unique<prometheus::Exposer>(endpoint)),
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
	std::unique_ptr<TestType> test_type;
} MetricTestObject;

static int MetricTest_init(MetricTestObject *self, PyObject *args, PyObject *kwds) {
	self->test_type = std::make_unique<TestType>("127.0.0.1:20800");
	return 0;
}

static void MetricTest_dealloc(MetricTestObject* self)
{
	self->test_type.reset(nullptr);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject MetricTestType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.MetricTest",             /* tp_name */
	sizeof(MetricTestObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)MetricTest_dealloc,        /* tp_dealloc */
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
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	0,					       /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)MetricTest_init, /* tp_init */
	0,                         /* tp_alloc */
	0,                         /* tp_new */
};

static PyMethodDef MetricTestMethods[] = {
	{NULL}  /* Sentinel */
};

void TestType::RegisterPythonObject(PyObject* module) {
	MetricTestType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&MetricTestType) < 0)
		return;

	Py_INCREF(&MetricTestType);
	PyModule_AddObject(module, "MetricTest", (PyObject *)&MetricTestType);
}
