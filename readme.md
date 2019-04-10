# eve-monolith-prometheus

Prometheus module for monolith.

This wraps the native [prometheus-cpp](https://github.com/jupp0r/prometheus-cpp) client and exposes it to Python as a native module (PYD).

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
counter = registry.MakeCounter('MyCounter', labels={'my_label':'my_value', 'another_label':'another_value'})

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
counter = registry.MakeCounter('MyCounter', labels={'my_label':'my_value', 'another_label':'another_value'})

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

Gauges are for values that can increase, decrease, or be set to an arbitrary value.  Generally, they are used to record observations of a variable like queue size or memory utilization.

```python
# Name is required
# labels are optional
gauge = registry.MakeGauge('MyGauge', labels={'my_label','my_value', 'another_label':'another_value'})

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

## Building

### Docker

* Install [Docker for Windows](https://docs.docker.com/docker-for-windows/install/)
* Run `build.bat` (or `build.bat -test` if you want to run tests as well)
* Resulting artifacts will appear in `.\export`

### Manual

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

* After building the solution,
* `cd test`
* `copy ..\x64\release\*.pyd`
* `copy ..\import\vcpkg\x64-windows\bin\*.dll`
* `python test.py`