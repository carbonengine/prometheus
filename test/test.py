import time

import prometheus_module

metric_registry = prometheus_module.MetricRegistry()

raw_input('Press enter to begin hosting at localhost:20800')

#print 'Incrementing the counter for 10 seconds...'
#for x in range(10):
#    time.sleep(1)
#    test_object.increment()
#    print x+1

metric_registry.Serve("127.0.0.1:20801")

raw_input('Press enter to stop hosting')
metric_registry.StopServing()

raw_input('press enter to quit')

