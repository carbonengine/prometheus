import time

import prometheus_module

metric_registry = prometheus_module.MetricRegistry()

raw_input('Press enter to begin hosting at localhost:20800')

metric_registry.Serve("20800")

metric_registry.MakeCounter("Counter with name only")
counter = metric_registry.MakeCounter("Counter with name and labels", {"label1":"value1","label2":"value2"})

metric_registry.MakeGauge("Gauge with name only")
gauge = metric_registry.MakeGauge("Gauge with name and labels", {"label1":"value1","label2":"value2"})
gauge.Set(999.0)

metric_registry.MakeSummary("Summary with name only")
summary = metric_registry.MakeSummary("Summary with name and labels and quantiles", labels={"label1":"value1","label2":"value2"}, quantiles=[(0.1,0.05),(0.5,0.05),(0.9,0.05)])
summary.Observe(0)

metric_registry.MakeHistogram("Histogram with name only")
histogram = metric_registry.MakeHistogram("Histogram with name and labels and buckets", labels={"label1":"value1","label2":"value2"}, boundaries=[1.0,3.0,7.0])
histogram.Observe(0)

print 'Modifying the metrics for 10 seconds...'
for x in range(10):
    time.sleep(1)
    counter.Increment()
    gauge.Decrement(x)
    summary.Observe(x/10.0)
    histogram.Observe(x+1)
    print x+1,
print ''

raw_input('Press enter to stop hosting')
metric_registry.StopServing()

raw_input('press enter to quit')

