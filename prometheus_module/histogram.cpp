// Copyright © 2019 CCP ehf.

#include "histogram.h"

// STL
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

// Prometheus
#include <prometheus/histogram.h>
using namespace prometheus_module;

// prometheus_module
#include "metric_factory.h"
#include "utilities.h"

struct Histogram::Private {
	Private(prometheus::Histogram* wrapped, prometheus_module::MetricFactory& factory) :
		histogram(wrapped),
		factory(factory)
	{
	}

	void LazyInstantiate(prometheus_module::Histogram* self) {
		if (histogram != nullptr) {
			return;
		}

		factory.MakeHistogram(name, labels, lazy_labels, boundaries, MetricFactory::MakeMetricOption::kPromoteFromLazy, self);
	}

	prometheus::Histogram* histogram;
	prometheus_module::MetricFactory& factory;
	std::string name;
	std::vector<std::string> labels;
	std::vector<double> boundaries;

	std::map<std::string, std::string> lazy_labels;
};

Histogram::Histogram(prometheus::Histogram& histogram, prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& labels, const std::vector<double>& boundaries) :
	private_(std::make_unique<Private>(&histogram, factory))
{
	private_->name = name;
	private_->labels = labels;
	private_->boundaries = boundaries;
}

Histogram::Histogram(prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels, const std::vector<double>& boundaries) :
	private_(std::make_unique<Private>(nullptr, factory))
{
	private_->name = name;
	private_->boundaries = boundaries;
	private_->lazy_labels = labels;

	private_->labels = label_names;
}

Histogram::~Histogram() = default;

void Histogram::Observe(double value) {
	private_->LazyInstantiate(this);
	private_->histogram->Observe(value);
}

HistogramInterface* Histogram::WithLabelValues(int num_values, const char* values[]) {
	std::vector<std::string> values_vec;
	for (auto i = 0; i < num_values; i++) {
		values_vec.push_back(values[i]);
	}
	return WithLabelValues(values_vec);
}

Histogram* Histogram::WithLabelValues(std::vector<std::string> values) {
	if (values.size() != private_->labels.size()) {
		return nullptr;
	}

	std::map<std::string, std::string> labels;
	for (auto i = 0; i < private_->labels.size(); i++) {
		labels.insert(std::make_pair(private_->labels[i], values[i]));
	}

	Histogram& result = private_->factory.MakeHistogram(private_->name, private_->labels, labels, private_->boundaries);
	return &result;
}

const std::string& Histogram::name() {
	return private_->name;
}

const std::vector<std::string>& Histogram::label_names() {
	return private_->labels;
}

void Histogram::set_wrapped(prometheus::Histogram& wrapped) {
	private_->histogram = &wrapped;
}

typedef struct {
	PyObject_HEAD
	std::unique_ptr<Histogram> histogram;
	PyObject* family;
	std::unordered_map<std::string, PyObject*>* cache;
} HistogramPyObject;

static int Histogram_init(HistogramPyObject *self, PyObject *args, PyObject *kwds) {
	PyObject* capsule = NULL;
	PyObject* family = NULL;

	static const char *kwlist[] = { "capsule", "family", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", (char**)kwlist, &capsule, &family))
		return -1;

	Histogram* wrapped = (Histogram*)PyCapsule_GetPointer(capsule, NULL);
	self->histogram.reset(wrapped);

	if (family != NULL) {
		self->family = family;
		Py_IncRef(family);
		self->cache = nullptr;
	}
	else {
		self->family = reinterpret_cast<PyObject*>(self);
		self->cache = new std::unordered_map<std::string, PyObject*>();
	}

	return 0;
}

static void Histogram_dealloc(HistogramPyObject* self) {
	self->histogram.reset(nullptr);

	if (self->family != reinterpret_cast<PyObject*>(self)) {
		Py_DecRef(self->family);
	}

	delete self->family;

	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* Histogram_Observe(HistogramPyObject* self, PyObject* args, PyObject* keywords) {
	double value = 0.0;
	static const char* keyword_list[] = {"value", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "d", (char**)keyword_list, &value)) {
		Py_RETURN_FALSE;
	}

	self->histogram->Observe(value);

	Py_RETURN_TRUE;
}

static PyObject* Histogram_WithLabelValues(HistogramPyObject* self, PyObject* args, PyObject* keywords) {
	PyObject* arg_labels = NULL;

	static const char* keyword_list[] = { "labels", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "|O", (char**)keyword_list, &arg_labels)) {
		return Py_BuildValue("O", self);
	}

	// Convert labels dictionary to map
	std::map<std::string, std::string> labels;
	if (arg_labels != NULL && PyDict_Check(arg_labels)) {
		PyObject* py_key = NULL;
		PyObject* py_value = NULL;
		Py_ssize_t pos = 0;

		while (PyDict_Next(arg_labels, &pos, &py_key, &py_value)) {
			std::string key = prometheus_module::Utilities::ConvertString(py_key);
			std::string value = prometheus_module::Utilities::ConvertString(py_value);

			if (!key.empty()) {
				labels.insert(std::make_pair(key, value));
			}
		}
	}

	// Strip invalid label names
	auto& valid_label_names = self->histogram->label_names();
	std::vector<std::string> final_labels;
	for (auto& name : valid_label_names) {
		auto&& iter = labels.find(name);
		if (iter != labels.end()) {
			final_labels.push_back(iter->second);
		}
		else {
			final_labels.push_back("");
		}
	}

	// Build cache key
	std::map<std::string, std::string> final_labels_with_values;
	for (auto&& label_name : final_labels) {
		final_labels_with_values.insert(std::make_pair(label_name, labels[label_name]));
	}

	std::stringstream ss;
	ss << "h|";
	ss << self->histogram->name();
	for (auto&& iter : final_labels_with_values) {
		ss << "|" << iter.first << "=" << iter.second;
	}
	std::string cache_key = ss.str();

	// Check cache for the metric
	HistogramPyObject* family = reinterpret_cast<HistogramPyObject*>(self->family);
	auto&& family_iter = family->cache->find(cache_key);
	if (family_iter != family->cache->end()) {
		PyObject* result = family_iter->second;
		Py_IncRef(result);
		return Py_BuildValue("O", result);
	}

	// Not in cache, so create a new metric
	auto native_metric = self->histogram->WithLabelValues(final_labels);
	if (native_metric == nullptr) {
		return Py_BuildValue("O", self);
	}

	// Cache it
	PyObject* python_metric = prometheus_module::Histogram::CreatePythonObject(native_metric, reinterpret_cast<PyObject*>(family));
	Py_IncRef(python_metric);
	family->cache->insert(std::make_pair(cache_key, python_metric));

	// And done
	return Py_BuildValue("O", python_metric);
}

static PyMethodDef HistogramPyMethods[] = {
	{"Observe", (PyCFunction)Histogram_Observe, METH_VARARGS | METH_KEYWORDS, "Observe the specified value."},

	{"WithLabelValues", (PyCFunction)Histogram_WithLabelValues, METH_VARARGS | METH_KEYWORDS, "Returns the histogram with the specified label values."},

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

PyObject* Histogram::CreatePythonObject(Histogram* wrapped, PyObject* family) {
	PyObject* capsule = PyCapsule_New((void*)wrapped, NULL, NULL);
	PyObject* obj = nullptr;
	if (family != nullptr) {
		obj = PyObject_Call((PyObject*)&HistogramPyType, Py_BuildValue("(O)", capsule), Py_BuildValue("{s:O}", "family", family));
	}
	else {
		obj = PyObject_CallObject((PyObject*)&HistogramPyType, Py_BuildValue("(O)", capsule));
	}
	return obj;
}
