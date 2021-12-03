#include "counter.h"

// STL
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

// Python
#include <Python.h>

// Prometheus
#include <prometheus/counter.h>
using namespace prometheus_module;

// prometheus_module
#include "metric_factory.h"
#include "utilities.h"

struct Counter::Private {
	Private(prometheus::Counter* wrapped, prometheus_module::MetricFactory& factory) :
		counter(wrapped),
		factory(factory)
	{
	}

	void LazyInstantiate(prometheus_module::Counter* self) {
		if (counter != nullptr) {
			return;
		}

		factory.MakeCounter(name, labels, lazy_labels, MetricFactory::MakeMetricOption::kPromoteFromLazy, self);
	}

	prometheus::Counter* counter;
	prometheus_module::MetricFactory& factory;
	std::string name;
	std::vector<std::string> labels;

	std::map<std::string, std::string> lazy_labels;
};

Counter::Counter(prometheus::Counter& counter, prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& labels) :
	private_(std::make_unique<Private>(&counter, factory))
{
	private_->name = name;
	private_->labels = labels;
}

Counter::Counter(prometheus_module::MetricFactory& factory, const std::string& name, const std::vector<std::string>& label_names, const std::map<std::string, std::string>& labels) :
	private_(std::make_unique<Private>(nullptr, factory))
{
	private_->name = name;
	private_->lazy_labels = labels;

	private_->labels = label_names;
}

Counter::~Counter() = default;

void Counter::Increment() {
	private_->LazyInstantiate(this);
	private_->counter->Increment();
}

void Counter::Increment(double value) {
	private_->LazyInstantiate(this);
	private_->counter->Increment(value);
}

CounterInterface* Counter::WithLabelValues(int num_values, const char* values[]) {
	std::vector<std::string> values_vec;
	for (auto i = 0; i < num_values; i++) {
		values_vec.push_back(values[i]);
	}
	return WithLabelValues(values_vec);
}

Counter* Counter::WithLabelValues(std::vector<std::string> values) {
	if (values.size() != private_->labels.size()) {
		return nullptr;
	}

	std::map<std::string, std::string> labels;
	for (auto i = 0; i < private_->labels.size(); i++) {
		labels.insert(std::make_pair(private_->labels[i], values[i]));
	}

	Counter& result = private_->factory.MakeCounter(private_->name, private_->labels, labels);
	return &result;
}

const std::string& Counter::name() {
	return private_->name;
}

const std::vector<std::string>& Counter::label_names() {
	return private_->labels;
}

void Counter::set_wrapped(prometheus::Counter& wrapped) {
	private_->counter = &wrapped;
}


// Python linkage

#include <Python.h>

typedef struct {
	PyObject_HEAD
	std::unique_ptr<Counter> counter;
	PyObject* family;
	std::unordered_map<std::string, PyObject*>* cache;
} CounterPyObject;

static int Counter_init(CounterPyObject *self, PyObject *args, PyObject *kwds) {
	PyObject* capsule = NULL;
	PyObject* family = NULL;

	static const char *kwlist[] = { "capsule", "family", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", (char**)kwlist, &capsule, &family))
		return -1;

	Counter* wrapped = (Counter*)PyCapsule_GetPointer(capsule, NULL);
	self->counter.reset(wrapped);

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

static void Counter_dealloc(CounterPyObject* self) {
	self->counter.reset(nullptr);

	if (self->family != reinterpret_cast<PyObject*>(self)) {
		Py_DecRef(self->family);
	}

	delete self->family;

	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* Counter_WithLabelValues(CounterPyObject* self, PyObject* args, PyObject* keywords) {
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
	auto& valid_label_names = self->counter->label_names();
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
	ss << "c|";
	ss << self->counter->name();
	for (auto&& iter : final_labels_with_values) {
		ss << "|" << iter.first << "=" << iter.second;
	}
	std::string cache_key = ss.str();

	// Check cache for the metric
	CounterPyObject* family = reinterpret_cast<CounterPyObject*>(self->family);
	auto&& family_iter = family->cache->find(cache_key);
	if (family_iter != family->cache->end()) {
		PyObject* result = family_iter->second;
		Py_IncRef(result);
		return Py_BuildValue("O", result);
	}

	// Not in cache, so create a new metric
	prometheus_module::Counter* native_counter = self->counter->WithLabelValues(final_labels);
	if (native_counter == nullptr) {
		return Py_BuildValue("O", self);
	}

	// Cache it
	PyObject* python_counter = prometheus_module::Counter::CreatePythonObject(native_counter, reinterpret_cast<PyObject*>(family));
	Py_IncRef(python_counter);
	family->cache->insert(std::make_pair(cache_key, python_counter));

	// And done
	return Py_BuildValue("O", python_counter);
}

static PyObject* Counter_Increment(CounterPyObject* self, PyObject* args, PyObject* keywords) {
	// value is optional.  If it is not passed in from Python, then Python won't touch the value
	// That means we can do a direct comparison (no floating-point threshold shenanigans) to determine whether
	//		or not a value was passed in.  Python doesn't reveal that information any other way.
	// I chose zero for the sentinel since it would be a no-op, and therefore useless as a value.
	double sentinel = 0.0;
	double value = sentinel;

	static const char* keyword_list[] = {"value", NULL};

	if (PyArg_ParseTupleAndKeywords(args, keywords, "|d", (char**)keyword_list, &value) && value != sentinel) {
		self->counter->Increment(value);
	}
	else {
		self->counter->Increment();
	}

	Py_RETURN_TRUE;
}

static PyMethodDef CounterPyMethods[] = {
	{"WithLabelValues", (PyCFunction)Counter_WithLabelValues, METH_VARARGS | METH_KEYWORDS, "Returns the counter with the specified label values."},
	{"Increment", (PyCFunction)Counter_Increment, METH_VARARGS | METH_KEYWORDS, "Increment the counter. Optionally, specify a value to increment by (default 1.0). If a value is specified, it must be non-zero."},

	{NULL}  /* Sentinel */
};

static PyTypeObject CounterPyType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.Counter",             /* tp_name */
	sizeof(CounterPyObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)Counter_dealloc,        /* tp_dealloc */
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
	"Counter metric for prometheus_module",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	CounterPyMethods,   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Counter_init, /* tp_init */
	0,                         /* tp_alloc */
	0,                         /* tp_new */
};

void Counter::RegisterPythonObject(PyObject* module) {
	CounterPyType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&CounterPyType) < 0)
		return;

	Py_INCREF(&CounterPyType);
	PyModule_AddObject(module, "Counter", (PyObject *)&CounterPyType);
}

PyObject* Counter::CreatePythonObject(Counter* wrapped, PyObject* family) {
	PyObject* capsule = PyCapsule_New((void*)wrapped, NULL, NULL);
	PyObject* obj = nullptr;
	if (family != nullptr) {
		obj = PyObject_Call((PyObject*)&CounterPyType, Py_BuildValue("(O)", capsule), Py_BuildValue("{s:O}", "family", family));
	}
	else {
		obj = PyObject_CallObject((PyObject*)&CounterPyType, Py_BuildValue("(O)", capsule));
	}
	return obj;
}
