import time

import prometheus_module

metric_registry = prometheus_module.MetricRegistry()

raw_input('Press enter to begin hosting at localhost:20800')

metric_registry.Serve("20800")

counter = metric_registry.MakeCounter("demo_counter", {"label1":"value1","label2":"value2"})
gauge = metric_registry.MakeGauge("demo_gauge", {"label1":"value1","label2":"value2"})
summary = metric_registry.MakeSummary("demo_summary", labels={"label1":"value1","label2":"value2"}, quantiles=[(0.1,0.05),(0.5,0.05),(0.9,0.05)])
histogram = metric_registry.MakeHistogram("demo_histogram", labels={"label1":"value1","label2":"value2"}, boundaries=[1.0,3.0,7.0])

print 'Modifying the metrics for 10 seconds...'
gauge.Set(999.0)
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

