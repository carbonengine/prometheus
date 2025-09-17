#include "gauge.h"

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
#include <prometheus/gauge.h>
using namespace prometheus_module;

// prometheus_module
#include "metric_factory.h"
#include "utilities.h"

struct Gauge::Private {
	Private(prometheus::Gauge* wrapped, prometheus_module::MetricFactory& factory) :
		gauge(wrapped),
		factory(factory)
	{
	}

	void LazyInstantiate(prometheus_module::Gauge* self) {
		if (gauge != nullptr) {
			return;
		}

		factory.MakeGauge(name, labels, lazy_labels, MetricFactory::MakeMetricOption::kPromoteFromLazy, self);
	}

	prometheus::Gauge* gauge;
	prometheus_module::MetricFactory& factory;
	std::string name;
	std::vector<std::string> labels;

	std::map<std::string, std::string> lazy_labels;
};

Gauge::Gauge(prometheus::Gauge& gauge, prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& labels) :
	private_(std::make_unique<Private>(&gauge, factory))
{
	private_->name = name;
	private_->labels = labels;
}

Gauge::Gauge(prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels) :
	private_(std::make_unique<Private>(nullptr, factory))
{
	private_->name = name;
	private_->lazy_labels = labels;

	private_->labels = label_names;
}

Gauge::~Gauge() = default;

void Gauge::Increment() {
	private_->LazyInstantiate(this);
	private_->gauge->Increment();
}

void Gauge::Increment(double value) {
	private_->LazyInstantiate(this);
	private_->gauge->Increment(value);
}

void Gauge::Decrement() {
	private_->LazyInstantiate(this);
	private_->gauge->Decrement();
}

void Gauge::Decrement(double value) {
	private_->LazyInstantiate(this);
	private_->gauge->Decrement(value);
}

void Gauge::Set(double value) {
	private_->LazyInstantiate(this);
	private_->gauge->Set(value);
}

GaugeInterface* Gauge::WithLabelValues(int num_values, const char* values[]) {
	std::vector<std::string> values_vec;
	for (auto i = 0; i < num_values; i++) {
		values_vec.push_back(values[i]);
	}
	return WithLabelValues(values_vec);
}

Gauge* Gauge::WithLabelValues(std::vector<std::string> values) {
	if (values.size() != private_->labels.size()) {
		return nullptr;
	}

	std::map<std::string, std::string> labels;
	for (auto i = 0; i < private_->labels.size(); i++) {
		labels.insert(std::make_pair(private_->labels[i], values[i]));
	}

	Gauge& result = private_->factory.MakeGauge(private_->name, private_->labels, labels);
	return &result;
}

const std::string& Gauge::name() {
	return private_->name;
}

const std::vector<std::string>& Gauge::label_names() {
	return private_->labels;
}

void Gauge::set_wrapped(prometheus::Gauge& wrapped) {
	private_->gauge = &wrapped;
}

typedef struct {
	PyObject_HEAD
	std::unique_ptr<Gauge> gauge;
	PyObject* family;
	std::unordered_map<std::string, PyObject*>* cache;
} GaugePyObject;

static int Gauge_init(GaugePyObject *self, PyObject *args, PyObject *kwds) {
	PyObject* capsule = NULL;
	PyObject* family = NULL;

	static const char *kwlist[] = { "capsule", "family", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", (char**)kwlist, &capsule, &family))
		return -1;

	Gauge* wrapped = (Gauge*)PyCapsule_GetPointer(capsule, NULL);
	self->gauge.reset(wrapped);

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

static void Gauge_dealloc(GaugePyObject* self) {
	self->gauge.reset(nullptr);

	if (self->family != reinterpret_cast<PyObject*>(self)) {
		Py_DecRef(self->family);
	}

	delete self->family;

	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* Gauge_Increment(GaugePyObject* self, PyObject* args, PyObject* keywords) {
	// value is optional.  If it is not passed in from Python, then Python won't touch the value
	// That means we can do a direct comparison (no floating-point threshold shenanigans) to determine whether
	//		or not a value was passed in.  Python doesn't reveal that information any other way.
	// I chose zero for the sentinel since it would be a no-op, and therefore useless as a value.
	double sentinel = 0.0;
	double value = sentinel;

	static const char* keyword_list[] = {"value", NULL};

	if (PyArg_ParseTupleAndKeywords(args, keywords, "|d", (char**)keyword_list, &value) && value != sentinel) {
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
	static const char* keyword_list[] = {"value", NULL};

	if (PyArg_ParseTupleAndKeywords(args, keywords, "|d", (char**)keyword_list, &value) && value != sentinel) {
		self->gauge->Decrement(value);
	}
	else {
		self->gauge->Decrement();
	}

	Py_RETURN_TRUE;
}

static PyObject* Gauge_Set(GaugePyObject* self, PyObject* args, PyObject* keywords) {
	double value = 0.0;
	static const char* keyword_list[] = {"value", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "d", (char**)keyword_list, &value)) {
		Py_RETURN_FALSE;
	}

	self->gauge->Set(value);

	Py_RETURN_TRUE;
}

static PyObject* Gauge_WithLabelValues(GaugePyObject* self, PyObject* args, PyObject* keywords) {
	PyObject* arg_labels = NULL;

	static const char* keyword_list[] = {"labels", NULL};

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
	auto& valid_label_names = self->gauge->label_names();
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
	ss << "g|";
	ss << self->gauge->name();
	for (auto&& iter : final_labels_with_values) {
		ss << "|" << iter.first << "=" << iter.second;
	}
	std::string cache_key = ss.str();

	// Check cache for the metric
	GaugePyObject* family = reinterpret_cast<GaugePyObject*>(self->family);
	auto&& family_iter = family->cache->find(cache_key);
	if (family_iter != family->cache->end()) {
		PyObject* result = family_iter->second;
		Py_IncRef(result);
		return Py_BuildValue("O", result);
	}

	// Not in cache, so create a new metric
	auto native_metric = self->gauge->WithLabelValues(final_labels);
	if (native_metric == nullptr) {
		return Py_BuildValue("O", self);
	}

	// Cache it
	PyObject* python_metric = prometheus_module::Gauge::CreatePythonObject(native_metric, reinterpret_cast<PyObject*>(family));
	Py_IncRef(python_metric);
	family->cache->insert(std::make_pair(cache_key, python_metric));

	// And done
	return Py_BuildValue("O", python_metric);
}

static PyMethodDef GaugePyMethods[] = {
	{"Increment", (PyCFunction)Gauge_Increment, METH_VARARGS | METH_KEYWORDS, "Increment the gauge. Optionally, specify a value to increment by (default 1.0). If a value is specified, it must be non-zero."},
	{"Decrement", (PyCFunction)Gauge_Decrement, METH_VARARGS | METH_KEYWORDS, "Decrement the gauge. Optionally, specify a value to decrement by (default 1.0). If a value is specified, it must be non-zero."},
	{"Set", (PyCFunction)Gauge_Set, METH_VARARGS | METH_KEYWORDS, "Set the gauge to the specified value."},

	{"WithLabelValues", (PyCFunction)Gauge_WithLabelValues, METH_VARARGS | METH_KEYWORDS, "Returns the gauge with the specified label values."},

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

PyObject* Gauge::CreatePythonObject(Gauge* wrapped, PyObject* family) {
	PyObject* capsule = PyCapsule_New((void*)wrapped, NULL, NULL);
	PyObject* obj = nullptr;
	if (family != nullptr) {
		obj = PyObject_Call((PyObject*)&GaugePyType, Py_BuildValue("(O)", capsule), Py_BuildValue("{s:O}", "family", family));
	}
	else {
		obj = PyObject_CallObject((PyObject*)&GaugePyType, Py_BuildValue("(O)", capsule));
	}
	return obj;
}
