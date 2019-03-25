#include "summary.h"

// STL
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <thread>

// Python
#include <Python.h>

// Prometheus
#include <prometheus/summary.h>
using namespace prometheus_module;

struct Summary::Private {
	Private(prometheus::Summary& wrapped) :
		summary(wrapped)
	{
	}

	prometheus::Summary& summary;
};

Summary::Summary(prometheus::Summary& summary) :
	private_(std::make_unique<Private>(summary))
{
}

void Summary::Observe(double value) {
	private_->summary.Observe(value);
}


// Python linkage

#include <Python.h>

typedef struct {
	PyObject_HEAD
	std::unique_ptr<Summary> summary;
} SummaryPyObject;

static int Summary_init(SummaryPyObject *self, PyObject *args, PyObject *kwds) {
	PyObject* capsule = NULL;

	static char *kwlist[] = { "capsule", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &capsule))
		return -1;

	Summary* wrapped = (Summary*)PyCapsule_GetPointer(capsule, NULL);
	self->summary.reset(wrapped);
	return 0;
}

static void Summary_dealloc(SummaryPyObject* self) {
	self->summary.reset(nullptr);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* Summary_Observe(SummaryPyObject* self, PyObject* args, PyObject* keywords) {
	double value = 0.0;
	static char* keyword_list[] = {"value", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "d", keyword_list, &value)) {
		Py_RETURN_FALSE;
	}

	self->summary->Observe(value);

	Py_RETURN_TRUE;
}

static PyMethodDef SummaryPyMethods[] = {
	{"Observe", (PyCFunction)Summary_Observe, METH_VARARGS | METH_KEYWORDS, "Observe the specified value."},

	{NULL}  /* Sentinel */
};

static PyTypeObject SummaryPyType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.Summary",             /* tp_name */
	sizeof(SummaryPyObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Summary_dealloc,        /* tp_dealloc */
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
	"Summary metric for prometheus_module",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	SummaryPyMethods,   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Summary_init, /* tp_init */
	0,                         /* tp_alloc */
	0,                         /* tp_new */
};

void Summary::RegisterPythonObject(PyObject* module) {
	SummaryPyType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&SummaryPyType) < 0) {
		return;
	}

	Py_INCREF(&SummaryPyType);
	PyModule_AddObject(module, "Summary", (PyObject *)&SummaryPyType);
}

PyObject* Summary::CreatePythonObject(Summary* wrapped) {
	PyObject* capsule = PyCapsule_New((void*)wrapped, NULL, NULL);
	PyObject* obj = PyObject_CallObject((PyObject*)&SummaryPyType, Py_BuildValue("(O)", capsule));
	return obj;
}

