import time

import prometheus_module

metric_registry = prometheus_module.MetricRegistry()

raw_input('Press enter to begin hosting at localhost:20800')

metric_registry.Serve("20801")

c = metric_registry.MakeCounter("Hi, my name is Counter")

print 'Incrementing the counter for 10 seconds...'
for x in range(10):
    time.sleep(1)
    c.Increment()
    print x+1

raw_input('Press enter to stop hosting')
metric_registry.StopServing()

raw_input('press enter to quit')

