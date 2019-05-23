# eve-monolith-prometheus

Prometheus module for monolith.

This wraps the native [prometheus-cpp](https://github.com/jupp0r/prometheus-cpp) client and exposes it to Python as a native module (PYD).

## Metric types

This library supports the Counter, Gauge, Histogram, and Summary metric types.  See the [prometheus docs](https://prometheus.io/docs/concepts/metric_types/) for a detailed explanation of each type.

## Python Usage

See `.\test\test.py` for detailed examples

### Basics

```python
import prometheus_module

# Create a metric registry. All metrics belong to a registry.
registry = prometheus_module.MetricRegistry()

# Serve the metrics on the specified port so Prometheus can pull them
registry.Serve('8080')

# At this point, localhost:8080 is live, but without any metric data.

# Create a Counter metric. name is required. labels are optional.
counter = registry.MakeCounter('MyCounter', labels=['my_label', 'another_label'])

# At this point, MyCounter should be visible on localhost:8080 with value 0

# Increment the counter
counter.Increment() # value is 1
counter.Increment() # value is 2
counter.Increment(10) # value is 12

# At this point, MyCounter should be visible on localhost:8080 with value 12
```

### Counter

Counters are for values that can only increase. Generally, they are used to record the number of times a given event happens like an error count or number of times a function is called.

```python
# Name is required.
# labels are optional.
counter = registry.MakeCounter('MyCounter', labels=['my_label', 'another_label'])

# Increments by one by default
counter.Increment()

# Optionally, can increment by any positive non-zero value
counter.Increment(10)

# Does not support 'incrementing' by zero. This will increment by one instead as it is the same as calling Increment() without a value.
#invalid: counter.Increment(0)

# Does not support negative values. This will have no effect.
#invalid: counter.Increment(-1)
```

### Gauge

Gauges are for values that can increase, decrease, or be set to an arbitrary value.  Generally, they are used to record observations of a variable like queue size, memory utilization, or number of processes.

```python
# Name is required
# labels are optional
gauge = registry.MakeGauge('MyGauge', labels=['my_label', 'another_label'])

# Can increment like a Counter
gauge.Increment() # Increments by one by default
gauge.Increment(10) # Can increment by an arbitrary value
#invalid: gauge.Increment(0) # This will increment by one instead as it is the same as calling Increment() without a value.
#invalid: gauge.Increment(-1) # Cannot increment by negative values. Use Decrement instead.

# Can decrement
gauge.Decrement() # Decrements by one by default
gauge.Decrement(10) # Can decrement by an arbitrary value
#invalid: gauge.Decrement(0) # This will decrement by one instead as it is the same as calling Decrement() without a value.
#invalid: gauge.Decrement(-1) # Cannot decrement by negative values. Use Increment instead.

# Can be set to an arbitrary value
gauge.Set(999)
```

### Histogram

Histograms are used to record samples of a value and count them in buckets.  Generally, these are used to record things like request sizes or durations.  The bucket sizes are configurable.

```python
# Name is required
# labels are optional
# boundaries specify the buckets. A bucket represents all values less than or equal to its boundary
histogram = registry.MakeHistogram('MyHistogram', labels=['my_label'], boundaries=[10,100,1000])

# Observe() records a sample, incrementing all buckets greater than or equal to the sample value.
# Given the boundaries [10,100,1000],
histogram.Observe(1) # Increments all buckets
histogram.Observe(11) # Increments the second and third buckets
histogram.Observe(101) # Increments the third bucket
histogram.Observe(1001) # Increments the (automatically-provided) infinity bucket
```

### Summary

See the [prometheus docs](https://prometheus.io/docs/practices/histograms/#quantiles) for a proper explanation of summaries.

Summaries are similar to Histograms, but they sort samples into φ-quantiles (basically percentiles) rather than buckets, and are used to represent a sliding window of time (as opposed to histograms which store their samples permanently).  Generally, these are used for the same types of data as histograms (request sizes and durations), but provide a different view due to their sliding window property, and are calculated on the client side (whereas histograms can be used to calculate quantiles on the server side).

```python
# Name is required
# labels are optional
# quantiles specify the percentile and error values of the quantile "buckets". The following example will calculate the 10th, 50th, and 90th percentile observations (first parameter of each tuple) with a 5% error tolerance (second parameter of each tuple).
# The sliding window size is currently fixed at 5 minutes.
summary = registry.MakeSummary('MySummary', labels=['my_label'], quantiles=[(0.1, 0.05), (0.5, 0.05), (0.9, 0.05)])

# Observe() records a sample value at the current time.
summary.Observe(1)
```

### Labels

See the [prometheus docs](https://prometheus.io/docs/practices/instrumentation/#use-labels) for a proper explanation of labels.

Labels allow you to group related metrics that differ along one or more dimensions.  Rather than programmatically generating metric names, you can instead use labels to differentiate them.  A common example is for http response codes.  Rather than making an http_response_200_total metric and an http_response_404 metric, you would instead make a single http_response_total metric with response_code as a label to differentiate the 200 and 404 response measurements.  Each distinct set of label values represents a standalone time-series.

```python
http_response_total = registry.MakeCounter('http_response_total', labels=['response_code'])

# Record a 200 result
http_response_total.WithLabelValues({'response_code':'200'}).Increment()

# Record a 404 result
http_response_total.WithLabelValues({'response_code':'404'}).Increment()

# For better performance, you can save a reference to the metric for a given set of labels
http_response_total_200 = http_response_total.WithLabelValues({'response_code':'200'})
http_response_total_200.Increment()

# Metrics are stored and can be looked up at will. If given the same label values, WithLabelValues() will return the
# same metric object
assert(http_response_total_200 is http_response_total.WithLabelValues({'response_code':'200'}))

# Which allows you to refer to the metric either way. If http_response_total_200 has a value of zero, then:
http_response_total_200.Increment()
http_response_total.WithLabelValues({'response_code':'200'}).Increment()
# Will result in http_response_total_200 having a value of 2

# If the metric returned by MakeCounter() is used directly, it uses empty strings for its label values
assert(http_response_total is http_response_total.WithLabelValues({'response_code':''}))
```

## Building

### via Docker

* Install [Docker for Windows](https://docs.docker.com/docker-for-windows/install/)
* Run `build.bat` (or `build.bat -test` if you want to run tests as well)
* Resulting artifacts will appear in `.\export`

### via Visual Studio

* Install VS2017 if you don't have it already
* Install [Windows 10 SDK 10.0.17763.132](https://go.microsoft.com/fwlink/p/?LinkID=2033908) or newer if you don't have it already
* Install dependencies. See build.Dockerfile for functional steps.
    * Create an import folder in the source root directory
    * Copy (or link) the [Stackless](http://www.stackless.com/binaries/python-2.7.15150.amd64-stackless.msi) Python27 folder into the import folder (`.\import\Python27`)
    * Install [vcpkg](https://github.com/Microsoft/vcpkg) into the import folder (`.\import\vcpkg`)
    * Open a command prompt inside the vcpkg folder. Use locally-installed packages instead of the global option in vcpkg.
    * Install [prometheus-cpp](https://github.com/jupp0r/prometheus-cpp): `vcpkg install prometheus-cpp:x64-windows`
    * Install [Googletest](https://github.com/google/googletest) (for tests only): `vcpkg install gtest:x64-windows`
    * Install [curl](https://github.com/curl/curl) (for tests only): `vcpkg install curl:x64-windows`
* Open prometheus_module.sln and build it.

## Running tests

### Docker

* Run `build.bat -test`

### Manual

* Build and run prometheus_module.sln with Visual Studio (press Ctrl+F5).
* Alternately, use the command line:
```DOS .bat
cd test
copy ..\x64\release\*.pyd
copy ..\import\vcpkg\x64-windows\bin\*.dll

python test.py
```
* For verbose output, use `python -m unittest discover -v -p test.py` in place of the last line above