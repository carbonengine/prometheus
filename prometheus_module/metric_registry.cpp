#include "metric_registry.h"

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
//#include <prometheus/exposer.h>
#include <prometheus/registry.h>

// prometheus-cpp-custom
#include "exposer.h"

// prometheus_module
#include "counter.h"
#include "gauge.h"
#include "histogram.h"
#include "summary.h"
using namespace prometheus_module;

struct MetricRegistry::Private {
	std::unique_ptr<prometheus_module::Exposer> exposer;
	std::shared_ptr<prometheus::Registry> registry;

	std::map<std::string, std::string> default_labels;

	prometheus::Summary::Quantiles default_quantiles;
	std::vector<double> default_boundaries;
};

MetricRegistry::MetricRegistry() :
	private_(std::make_unique<Private>())
{
	private_->registry = std::make_shared<prometheus::Registry>();

	auto default_error = 0.05;
	private_->default_quantiles = prometheus::Summary::Quantiles{ 
		{0.01, default_error},
		{0.1, default_error},
		{0.5, default_error},
		{0.9, default_error},
		{0.99, default_error}
	};
}

Counter* MetricRegistry::MakeCounter(const char* name, const std::map<std::string, std::string>& labels) {
	auto& family = prometheus::BuildCounter().Name(name).Labels(private_->default_labels).Register(*private_->registry);
	prometheus::Counter& prometheus_counter = family.Add(labels);

	return new prometheus_module::Counter(prometheus_counter);
}

Gauge* MetricRegistry::MakeGauge(const char* name, const std::map<std::string, std::string>& labels) {
	auto& family = prometheus::BuildGauge().Name(name).Labels(private_->default_labels).Register(*private_->registry);
	prometheus::Gauge& prometheus_gauge = family.Add(labels);

	return new prometheus_module::Gauge(prometheus_gauge);
}

Summary* MetricRegistry::MakeSummary(const char* name, const std::map <std::string, std::string>& labels, const std::vector<std::pair<double, double> >& quantiles, int total_window_size_seconds, int window_partitions) {
	// Assign labels
	const std::map<std::string, std::string>* final_labels = &private_->default_labels;
	if (!labels.empty()) {
		final_labels = &labels;
	}

	// Convert and assign quantiles
	prometheus::Summary::Quantiles* final_quantiles = &private_->default_quantiles;
	prometheus::Summary::Quantiles quantiles_converted;
	if (!quantiles.empty()) {
		for (auto e : quantiles) {
			quantiles_converted.push_back(prometheus::detail::CKMSQuantiles::Quantile(e.first, e.second));
		}
		final_quantiles = &quantiles_converted;
	}

	// If the window is invalid or unspecified, default to a 5-minute window, split into 5 partitions (of 1 minute each)
	if (total_window_size_seconds <= 0) {
		total_window_size_seconds = 300;
	}

	if (window_partitions <= 0) {
		window_partitions = 5;
	}


	// Create
	auto& family = prometheus::BuildSummary().Name(name).Labels(private_->default_labels).Register(*private_->registry);
	prometheus::Summary& prometheus_summary = family.Add(*final_labels, *final_quantiles, std::chrono::seconds{ total_window_size_seconds / window_partitions }, window_partitions);

	return new prometheus_module::Summary(prometheus_summary);
}

Histogram* MetricRegistry::MakeHistogram(const char* name, const std::map <std::string, std::string>& labels, const std::vector<double>& boundaries) {
	// Assign labels
	const std::map<std::string, std::string>* final_labels = &private_->default_labels;
	if (!labels.empty()) {
		final_labels = &labels;
	}

	// Assign boundaries
	const std::vector<double>* final_boundaries = &private_->default_boundaries;
	if (!boundaries.empty()) {
		final_boundaries = &boundaries;
	}

	// Create
	auto& family = prometheus::BuildHistogram().Name(name).Labels(private_->default_labels).Register(*private_->registry);
	prometheus::Histogram& prometheus_histogram = family.Add(*final_labels, *final_boundaries);

	return new prometheus_module::Histogram(prometheus_histogram);
}

CounterInterface* MetricRegistry::MakeCounter(const char* name, int num_labels, const char* label_keys[], const char* label_values[]) {
	std::map<std::string, std::string> labels;
	for (auto i = 0; i < num_labels; i++) {
		labels.insert(std::make_pair(label_keys[i], label_values[i]));
	}
	return MakeCounter(name, labels);
}

GaugeInterface* MetricRegistry::MakeGauge(const char* name, int num_labels, const char* label_keys[], const char* label_values[]) {
	std::map<std::string, std::string> labels;
	for (auto i = 0; i < num_labels; i++) {
		labels.insert(std::make_pair(label_keys[i], label_values[i]));
	}
	return MakeGauge(name, labels);
}

SummaryInterface* MetricRegistry::MakeSummary(const char* name, int num_labels, const char* label_keys[], const char* label_values[], int num_quantiles, double quantile_values[], double quantile_tolerances[], int total_window_size_seconds, int window_partitions) {
	std::map<std::string, std::string> labels;
	for (auto i = 0; i < num_labels; i++) {
		labels.insert(std::make_pair(label_keys[i], label_values[i]));
	}

	std::vector<std::pair<double, double> > quantiles;
	for (auto i = 0; i < num_quantiles; i++) {
		quantiles.push_back(std::make_pair(quantile_values[i], quantile_tolerances[i]));
	}

	return MakeSummary(name, labels, quantiles, total_window_size_seconds, window_partitions);
}

HistogramInterface* MetricRegistry::MakeHistogram(const char* name, int num_labels, const char* label_keys[], const char* label_values[], int num_boundaries, double boundary_values[]) {
	std::map<std::string, std::string> labels;
	for (auto i = 0; i < num_labels; i++) {
		labels.insert(std::make_pair(label_keys[i], label_values[i]));
	}

	std::vector<double> boundaries;
	for (auto i = 0; i < num_boundaries; i++) {
		boundaries.push_back(boundary_values[i]);
	}

	return MakeHistogram(name, labels, boundaries);
}

bool MetricRegistry::Serve(const char* bind_address) {
	StopServing();

	try {
		private_->exposer = std::make_unique<prometheus_module::Exposer>(bind_address, "");
	}
	catch(...) {
		return false;
	}

	private_->exposer->RegisterCollectable(private_->registry);

	return true;
}

void MetricRegistry::StopServing() {
	private_->exposer = nullptr;
}


// Python linkage

typedef struct {
	PyObject_HEAD
	std::unique_ptr<MetricRegistry> metric_registry;
} MetricRegistryPyObject;

static int MetricRegistry_init(MetricRegistryPyObject *self, PyObject *args, PyObject *kwds) {
	self->metric_registry = std::make_unique<MetricRegistry>();
	return 0;
}

static void MetricRegistry_dealloc(MetricRegistryPyObject* self) {
	self->metric_registry.reset(nullptr);
	Py_TYPE(self)->tp_free((PyObject*)self);
}


static PyObject* MetricRegistry_MakeCounter(MetricRegistryPyObject* self, PyObject* args, PyObject* keywords) {
	const char* arg_name = NULL;
	PyObject* arg_labels = NULL;

	static char* keyword_list[] = {"name", "labels", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "s|O", keyword_list, &arg_name, &arg_labels)) {
		Py_RETURN_NONE;
	}

	std::string name = "Unnamed Counter";
	if (arg_name != NULL) {
		name = arg_name;
	}

	std::map<std::string, std::string> labels;
	if (arg_labels != NULL && PyDict_Check(arg_labels)) {
		PyObject* py_key = NULL;
		PyObject* py_value = NULL;
		Py_ssize_t pos = 0;

		while (PyDict_Next(arg_labels, &pos, &py_key, &py_value)) {
			if (!PyString_Check(py_key) || !PyString_Check(py_value)) {
				continue;
			}

			const char* key = PyString_AsString(py_key);
			const char* value = PyString_AsString(py_value);
			labels.insert(std::make_pair(key, value));
		}
	}

	Counter* native_counter = self->metric_registry->MakeCounter(name.c_str(), labels);
	return Py_BuildValue("O", Counter::CreatePythonObject(native_counter));
}

static PyObject* MetricRegistry_MakeGauge(MetricRegistryPyObject* self, PyObject* args, PyObject* keywords) {
	const char* arg_name = NULL;
	PyObject* arg_labels = NULL;

	static char* keyword_list[] = {"name", "labels", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "s|O", keyword_list, &arg_name, &arg_labels)) {
		Py_RETURN_NONE;
	}

	std::string name = "Unnamed Gauge";
	if (arg_name != NULL) {
		name = arg_name;
	}

	std::map<std::string, std::string> labels;
	if (arg_labels != NULL && PyDict_Check(arg_labels)) {
		PyObject* py_key = NULL;
		PyObject* py_value = NULL;
		Py_ssize_t pos = 0;

		while (PyDict_Next(arg_labels, &pos, &py_key, &py_value)) {
			if (!PyString_Check(py_key) || !PyString_Check(py_value)) {
				continue;
			}

			const char* key = PyString_AsString(py_key);
			const char* value = PyString_AsString(py_value);
			labels.insert(std::make_pair(key, value));
		}
	}

	Gauge* native_gauge = self->metric_registry->MakeGauge(name.c_str(), labels);
	return Py_BuildValue("O", Gauge::CreatePythonObject(native_gauge));
}

static PyObject* MetricRegistry_MakeHistogram(MetricRegistryPyObject* self, PyObject* args, PyObject* keywords) {

	// Parse parameters
	const char* arg_name = NULL;
	PyObject* arg_labels = NULL;
	PyObject* arg_boundaries = NULL;

	static char* keyword_list[] = { "name", "labels", "boundaries", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "s|OO", keyword_list, &arg_name, &arg_labels, &arg_boundaries)) {
		Py_RETURN_NONE;
	}

	// Convert name
	std::string name = "Unnamed Histogram";
	if (arg_name != NULL) {
		name = arg_name;
	}

	// Convert labels
	std::map<std::string, std::string> labels;
	if (arg_labels != NULL && PyDict_Check(arg_labels)) {
		PyObject* py_key = NULL;
		PyObject* py_value = NULL;
		Py_ssize_t pos = 0;

		while (PyDict_Next(arg_labels, &pos, &py_key, &py_value)) {
			if (!PyString_Check(py_key) || !PyString_Check(py_value)) {
				continue;
			}

			const char* key = PyString_AsString(py_key);
			const char* value = PyString_AsString(py_value);
			labels.insert(std::make_pair(key, value));
		}
	}

	// Convert boundaries
	std::vector<double> boundaries;
	if (arg_boundaries != NULL && PyList_Check(arg_boundaries)) {
		auto num_elements = PyList_Size(arg_boundaries);
		for (auto i = 0; i < num_elements; i++) {
			PyObject* py_boundary = PyNumber_Float(PyList_GetItem(arg_boundaries, i));
			if (py_boundary == NULL || !PyFloat_Check(py_boundary)) {
				continue;
			}

			double boundary = PyFloat_AsDouble(py_boundary);
			boundaries.push_back(boundary);
		}
	}

	// Create the Histogram
	Histogram* native_histogram = self->metric_registry->MakeHistogram(name.c_str(), labels, boundaries);
	return Py_BuildValue("O", Histogram::CreatePythonObject(native_histogram));
}

static PyObject* MetricRegistry_MakeSummary(MetricRegistryPyObject* self, PyObject* args, PyObject* keywords) {

	// Parse parameters
	const char* arg_name = NULL;
	PyObject* arg_labels = NULL;
	PyObject* arg_quantiles = NULL;
	int window_size_seconds = 0;
	int window_partitions = 0;

	static char* keyword_list[] = { "name", "labels", "quantiles", "window_size_seconds", "window_partitions", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "s|OOii", keyword_list, &arg_name, &arg_labels, &arg_quantiles, &window_size_seconds, &window_partitions)) {
		Py_RETURN_NONE;
	}


	// Convert name
	std::string name = "Unnamed Summary";
	if (arg_name != NULL) {
		name = arg_name;
	}

	// Convert labels
	std::map<std::string, std::string> labels;
	if (arg_labels != NULL && PyDict_Check(arg_labels)) {
		PyObject* py_key = NULL;
		PyObject* py_value = NULL;
		Py_ssize_t pos = 0;

		while (PyDict_Next(arg_labels, &pos, &py_key, &py_value)) {
			if (!PyString_Check(py_key) || !PyString_Check(py_value)) {
				continue;
			}

			const char* key = PyString_AsString(py_key);
			const char* value = PyString_AsString(py_value);
			labels.insert(std::make_pair(key, value));
		}
	}

	// Convert quantiles
	std::vector<std::pair<double, double> > quantiles;
	if (arg_quantiles != NULL && PyList_Check(arg_quantiles)) {
		auto num_elements = PyList_Size(arg_quantiles);
		for (auto i = 0; i < num_elements; i++) {
			PyObject* tuple = PyList_GetItem(arg_quantiles, i);
			if (!PyTuple_Check(tuple)) {
				continue;
			}

			if (PyTuple_Size(tuple) < 2) {
				continue;
			}

			PyObject* py_quantile = PyNumber_Float(PyTuple_GetItem(tuple, 0));
			PyObject* py_error = PyNumber_Float(PyTuple_GetItem(tuple, 1));
			if (py_quantile == NULL || !PyFloat_Check(py_quantile) || py_error == NULL || !PyFloat_Check(py_error)) {
				continue;
			}

			double quantile = PyFloat_AsDouble(py_quantile);
			double error = PyFloat_AsDouble(py_error);
			quantiles.push_back(std::make_pair(quantile, error));
		}
	}

	// Create the Summary
	Summary* native_summary = self->metric_registry->MakeSummary(name.c_str(), labels, quantiles, window_size_seconds, window_partitions);
	return Py_BuildValue("O", Summary::CreatePythonObject(native_summary));
}

static PyObject* MetricRegistry_Serve(MetricRegistryPyObject* self, PyObject* args, PyObject* keywords) {
	char* bind_address = NULL;
	static char* keyword_list[] = {"bind_address", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywords, "s", keyword_list, &bind_address) || bind_address == NULL) {
		Py_RETURN_FALSE;
	}

	if (!self->metric_registry->Serve(bind_address)) {
		Py_RETURN_FALSE;
	}

	Py_RETURN_TRUE;
}

static PyObject* MetricRegistry_StopServing(MetricRegistryPyObject* self) {
	self->metric_registry->StopServing();
	Py_RETURN_TRUE;
}

static PyObject* MetricRegistry_GetCapsule(MetricRegistryPyObject* self) {
	MetricRegistryInterface* registry_interface = (MetricRegistryInterface*)self->metric_registry.get();
	PyObject* capsule = PyCapsule_New((void*)registry_interface, NULL, NULL);
	return capsule;
}

static PyMethodDef MetricRegistryPyMethods[] = {
	{"MakeCounter", (PyCFunction)MetricRegistry_MakeCounter, METH_VARARGS | METH_KEYWORDS, "Creates and returns a new prometheus_module.Counter metric"},
	{"MakeGauge", (PyCFunction)MetricRegistry_MakeGauge, METH_VARARGS | METH_KEYWORDS, "Creates and returns a new prometheus_module.Gauge metric"},
	{"MakeHistogram", (PyCFunction)MetricRegistry_MakeHistogram, METH_VARARGS | METH_KEYWORDS, "Creates and returns a new prometheus_module.Histogram metric"},
	{"MakeSummary", (PyCFunction)MetricRegistry_MakeSummary, METH_VARARGS | METH_KEYWORDS, "Creates and returns a new prometheus_module.Summary metric"},

	{"Serve", (PyCFunction)MetricRegistry_Serve, METH_VARARGS | METH_KEYWORDS, "Start serving metrics at the specified [ip:]port. To serve multiple ports, use comma separation: [ip:]port,[ip:]port[,...]. Returns True on success, False on failure."},
	{"StopServing", (PyCFunction)MetricRegistry_StopServing, METH_NOARGS, "Stop serving metrics"},

	{"GetCapsule", (PyCFunction)MetricRegistry_GetCapsule, METH_NOARGS, "Returns a capsule containing a pointer to the native MetricRegistryInterface for this instance"},

	{NULL}  /* Sentinel */
};

static PyTypeObject MetricRegistryPyType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"prometheus_module.MetricRegistry",             /* tp_name */
	sizeof(MetricRegistryPyObject),  /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)MetricRegistry_dealloc,        /* tp_dealloc */
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
	"Metric registry, factory, and http handler",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	MetricRegistryPyMethods,   /* tp_methods */
	0,						   /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)MetricRegistry_init, /* tp_init */
	0,                         /* tp_alloc */
	0,                         /* tp_new */
};

void MetricRegistry::RegisterPythonObject(PyObject* module) {
	MetricRegistryPyType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&MetricRegistryPyType) < 0)
		return;

	Py_INCREF(&MetricRegistryPyType);
	PyModule_AddObject(module, "MetricRegistry", (PyObject *)&MetricRegistryPyType);
}

