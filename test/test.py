import time

import prometheus_module

metric_registry = prometheus_module.MetricRegistry()

raw_input('Press enter to begin hosting at localhost:20800')

metric_registry.Serve("20801")

#metric_registry.MakeCounter() # Error. MakeCounter requires a name parameter now
metric_registry.MakeCounter("Counter with name only")
c = metric_registry.MakeCounter("Counter with name and labels", {"label1":"value1","label2":"value2"})

print 'Incrementing the counter for 10 seconds...'
for x in range(10):
    time.sleep(1)
    c.Increment()
    print x+1

raw_input('Press enter to stop hosting')
metric_registry.StopServing()

raw_input('press enter to quit')

