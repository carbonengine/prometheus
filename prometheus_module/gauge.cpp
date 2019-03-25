#include "gauge.h"

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
#include <prometheus/gauge.h>
using namespace prometheus_module;

struct Gauge::Private {
	Private(prometheus::Gauge& wrapped) :
		gauge(wrapped)
	{
	}

	prometheus::Gauge& gauge;
};

Gauge::Gauge(prometheus::Gauge& gauge) :
	private_(std::make_unique<Private>(gauge))
{
}

void Gauge::Increment() {
	private_->gauge.Increment();
}

void Gauge::Increment(double value) {
	private_->gauge.Increment(value);
}

void Gauge::Decrement() {
	private_->gauge.Decrement();
}

void Gauge::Decrement(double value) {
	private_->gauge.Decrement(value);
}

void Gauge::Set(double value) {
	private_->gauge.Set(value);
}


// Python linkage

#include <Python.h>

typedef struct {
	PyObject_HEAD
	std::unique_ptr<Gauge> gauge;
} GaugePyObject;

static int Gauge_init(GaugePyObject *self, PyObject *args, PyObject *kwds) {
	PyObject* capsule = NULL;

	static char *kwlist[] = { "capsule", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &capsule))
		return -1;

	Gauge* wrapped = (Gauge*)PyCapsule_GetPointer(capsule, NULL);
	self->gauge.reset(wrapped);
	return 0;
}

static void Gauge_dealloc(GaugePyObject* self) {
	self->gauge.reset(nullptr);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* Gauge_Increment(GaugePyObject* self, PyObject* args, PyObject* keywords) {
	// value is optional.  If it is not passed in from Python, then Python won't touch the value
	// That means we can do a direct comparison (no floating-point threshold shenanigans) to determine whether
	//		or not a value was passed in.  Python doesn't reveal that information any other way.
	// I chose zero for the sentinel since it would be a no-op, and therefore useless as a value.
	double sentinel = 0.0;
	double value = sentinel;

	static char* keyword_list[] = {"value", NULL};

	if (PyArg_ParseTupleAndKeywords(args, keywords, "|d", keyword_list, &value) && value != sentinel) {
		self->gauge->Increment(value);
	} else {
		self->gauge->Increment();
	}

	Py_RETURN_TRUE;
}

static PyObject* Gauge_Decrement(GaugePyObject* self, PyObject* args, PyObject* keywords) {
	// value is optional.  If it is not passed in from Python, then Python won't touch the value
	// That means we can do a direct comparison (no floating-point threshold shenanigans) to determine whether
	//		or not a value was passed in.  Python doesn't reveal that information any other way.
	// I chose zero for the sentinel since it would be a no-op, and therefore useless as a value.
	double sentinel = 0.0;
	double value = sentinel;
	static char* keyword_list[] = {"value", NULL};

	if (PyArg_ParseTupleAndKeywords(args, keywords, "|d", keyword_list, &value) && value != sentinel) {
		self->gauge->Decrement(value);
	}
	else {
		self->gauge->Decrement();
	}

	Py_RETURN_TRUE;
}

static PyObject* Gauge_Set(GaugePyObject* self, PyObject* args, PyObject* keywords) {
	double value = 0.0;
	static char* keyword_list[] = {"value", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "d", keyword_list, &value)) {
		Py_RETURN_FALSE;
	}

	self->gauge->Set(value);

	Py_RETURN_TRUE;
}

static PyMethodDef GaugePyMethods[] = {
	{"Increment", (PyCFunction)Gauge_Increment, METH_VARARGS | METH_KEYWORDS, "Increment the gauge. Optionally, specify a value to increment by (default 1.0). If a value is specified, it must be non-zero."},
	{"Decrement", (PyCFunction)Gauge_Decrement, METH_VARARGS | METH_KEYWORDS, "Decrement the gauge. Optionally, specify a value to decrement by (default 1.0). If a value is specified, it must be non-zero."},
	{"Set", (PyCFunction)Gauge_Set, METH_VARARGS | METH_KEYWORDS, "Set the gauge to the specified value."},

	{NULL}  /* Sentinel */
};

static PyTypeObject GaugePyType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.Gauge",             /* tp_name */
	sizeof(GaugePyObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Gauge_dealloc,        /* tp_dealloc */
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
	"Gauge metric for prometheus_module",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	GaugePyMethods,   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Gauge_init, /* tp_init */
	0,                         /* tp_alloc */
	0,                         /* tp_new */
};

void Gauge::RegisterPythonObject(PyObject* module) {
	GaugePyType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&GaugePyType) < 0) {
		return;
	}

	Py_INCREF(&GaugePyType);
	PyModule_AddObject(module, "Gauge", (PyObject *)&GaugePyType);
}

PyObject* Gauge::CreatePythonObject(Gauge* wrapped) {
	PyObject* capsule = PyCapsule_New((void*)wrapped, NULL, NULL);
	PyObject* obj = PyObject_CallObject((PyObject*)&GaugePyType, Py_BuildValue("(O)", capsule));
	return obj;
}

