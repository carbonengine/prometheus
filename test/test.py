import time

import prometheus_module

raw_input('Press enter to begin hosting the /metrics endpoint')
test_object = prometheus_module.MetricTest()

print 'Incrementing the counter for 10 seconds...'
for x in range(10):
    time.sleep(1)
    test_object.increment()
    print x+1

raw_input('Press enter to terminate the /metrics endpoint')
test_object = None

raw_input('press enter to quit')
