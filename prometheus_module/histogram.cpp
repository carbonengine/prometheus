#include "histogram.h"

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
#include <prometheus/histogram.h>
using namespace prometheus_module;

struct Histogram::Private {
	Private(prometheus::Histogram& wrapped) :
		histogram(wrapped)
	{
	}

	prometheus::Histogram& histogram;
};

Histogram::Histogram(prometheus::Histogram& histogram) :
	private_(std::make_unique<Private>(histogram))
{
}

void Histogram::Observe(double value) {
	private_->histogram.Observe(value);
}


// Python linkage

#include <Python.h>

typedef struct {
	PyObject_HEAD
	std::unique_ptr<Histogram> histogram;
} HistogramPyObject;

static int Histogram_init(HistogramPyObject *self, PyObject *args, PyObject *kwds) {
	PyObject* capsule = NULL;

	static char *kwlist[] = { "capsule", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &capsule))
		return -1;

	Histogram* wrapped = (Histogram*)PyCapsule_GetPointer(capsule, NULL);
	self->histogram.reset(wrapped);
	return 0;
}

static void Histogram_dealloc(HistogramPyObject* self) {
	self->histogram.reset(nullptr);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* Histogram_Observe(HistogramPyObject* self, PyObject* arg) {
	if (!PyFloat_Check(arg)) {
		Py_RETURN_FALSE;
	}

	self->histogram->Observe(PyFloat_AsDouble(arg));

	Py_RETURN_TRUE;
}

static PyMethodDef HistogramPyMethods[] = {
	{"Observe", (PyCFunction)Histogram_Observe, METH_O, "Observe the specified value."},

	{NULL}  /* Sentinel */
};

static PyTypeObject HistogramPyType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.Histogram",             /* tp_name */
	sizeof(HistogramPyObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Histogram_dealloc,        /* tp_dealloc */
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
	"Histogram metric for prometheus_module",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	HistogramPyMethods,   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Histogram_init, /* tp_init */
	0,                         /* tp_alloc */
	0,                         /* tp_new */
};

void Histogram::RegisterPythonObject(PyObject* module) {
	HistogramPyType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&HistogramPyType) < 0) {
		return;
	}

	Py_INCREF(&HistogramPyType);
	PyModule_AddObject(module, "Histogram", (PyObject *)&HistogramPyType);
}

PyObject* Histogram::CreatePythonObject(Histogram* wrapped) {
	PyObject* capsule = PyCapsule_New((void*)wrapped, NULL, NULL);
	PyObject* obj = PyObject_CallObject((PyObject*)&HistogramPyType, Py_BuildValue("(O)", capsule));
	return obj;
}

