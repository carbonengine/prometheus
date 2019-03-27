import string
import random
import unittest
import urllib2

import prometheus_module


#
# Base
#

class TestBase(unittest.TestCase):
    def setUp(self):
        self.port = '20800'
        self.registry = prometheus_module.MetricRegistry()

    def Fetch(self, port=''):
        if not port:
            port = self.port
        url = 'http://localhost:' + port
        return urllib2.urlopen(url).read()

    def FetchLine(self, substr, port=''):
        for line in self.Fetch(port).split('\n'):
            if (substr in line) and not ('#' in line):
                return line.strip()
        return ''

    def IsServerListening(self, port=''):
        try:
            content = self.Fetch(port)
            has_valid_content = 'exposer' in content
            return has_valid_content
        except urllib2.URLError:
            return False

    def RandomString(self, length=6):
        return ''.join(random.choice(string.ascii_uppercase) for _ in range(length))


#
# Server
#

class TestServing(TestBase):
    def test_server_start_stop(self):
        self.assertFalse(self.IsServerListening(), 'Server must not listen until Serve is called')
        self.registry.Serve(self.port)
        self.assertTrue(self.IsServerListening(), 'Server must listen after Serve is called')
        self.registry.StopServing()
        self.assertFalse(self.IsServerListening(), 'Server must stop listening after StopServing is called')

    def test_serve_port_in_use(self):
        #todo. this crashes
        self.assertTrue(False, 'todo')
        #self.assertFalse(self.IsServerListening('20800'))
        #self.registry.Serve('20800')
        #registry2 = prometheus_module.MetricRegistry()
        #registry2.Serve('20800')
        
    def test_bad_port_formats(self):
        # todo. these currently crash the program. not exceptions. just fire and burning.
        self.assertTrue(False, 'todo')
        #self.registry.Serve('http://localhost:20800')
        #self.registry.Serve(':20800')
        #self.registry.Serve('')
        #self.registry.Serve()

    def test_good_port_formats(self):
        self.assertTrue(False, 'todo')
        # todo


#
# Counter
#

class TestCounter(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        self.registry.Serve(self.port)

    def tearDown(self):
        TestBase.tearDown(self)
        self.registry.StopServing()

    def FetchCounter(self, name):
        line = self.FetchLine(name)
        if not line:
            return 0
        string_value = line.split(' ')[-1]
        return float(string_value)


    def test_MakeCounter(self):
        n = self.RandomString()
        self.assertFalse(self.FetchLine(n))
        self.registry.MakeCounter(n)
        self.assertTrue(self.FetchLine(n))

    def test_MakeCounter_with_labels(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_name2 = self.RandomString()
        label_value2 = self.RandomString()

        self.registry.MakeCounter(n, {label_name:label_value, label_name2:label_value2})

        line = self.FetchLine(n)
        self.assertTrue(label_name in line)
        self.assertTrue(label_value in line)
        self.assertTrue(label_name2 in line)
        self.assertTrue(label_value2 in line)

    def test_MakeCounter_with_label_with_leading_number_fails(self):
        #todo. this succeeds if NDEBUG is not set, but asserts if NDEBUG is set.
        self.assertTrue(False, 'todo')
        #self.registry.MakeCounter(self.RandomString(), {'123name':'value'})

    def test_counter_increment(self):
        n = self.RandomString()
        c = self.registry.MakeCounter(n)
        self.assertEqual(self.FetchCounter(n), 0, 'Counter must start at zero')
        c.Increment()
        self.assertEqual(self.FetchCounter(n), 1, 'Counter must increment by one by default')
        c.Increment(10)
        self.assertEqual(self.FetchCounter(n), 11, 'Counter must increment by parameter value')

    def test_counter_decrement_fails(self):
        n = self.RandomString()
        c = self.registry.MakeCounter(n)
        c.Increment(-1)
        self.assertEqual(self.FetchCounter(n), 0, 'Counter must not decrement')


#
# Gauge
#

class TestGauge(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        self.registry.Serve(self.port)

    def tearDown(self):
        TestBase.tearDown(self)
        self.registry.StopServing()

    def FetchGauge(self, name):
        line = self.FetchLine(name)
        if not line:
            return 0
        string_value = line.split(' ')[-1]
        return float(string_value)


    def test_MakeGauge(self):
        n = self.RandomString()
        self.assertFalse(self.FetchLine(n))
        self.registry.MakeGauge(n)
        self.assertTrue(self.FetchLine(n))

    def test_MakeGauge_with_labels(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_name2 = self.RandomString()
        label_value2 = self.RandomString()

        self.registry.MakeGauge(n, {label_name:label_value, label_name2:label_value2})

        line = self.FetchLine(n)
        self.assertTrue(label_name in line)
        self.assertTrue(label_value in line)
        self.assertTrue(label_name2 in line)
        self.assertTrue(label_value2 in line)

    def test_gauge_increment(self):
        n = self.RandomString()
        g = self.registry.MakeGauge(n)
        self.assertEqual(self.FetchGauge(n), 0, 'Gauge must start at zero')
        g.Increment()
        self.assertEqual(self.FetchGauge(n), 1, 'Gauge must increment by one by default')
        g.Increment(10)
        self.assertEqual(self.FetchGauge(n), 11, 'Gauge must increment by parameter value')

    def test_gauge_decrement(self):
        n = self.RandomString()
        g = self.registry.MakeGauge(n)
        self.assertEqual(self.FetchGauge(n), 0, 'Gauge must start at zero')
        g.Decrement()
        self.assertEqual(self.FetchGauge(n), -1, 'Gauge must decrement by one by default')
        g.Decrement(10)
        self.assertEqual(self.FetchGauge(n), -11, 'Gauge must decrement by parameter value')

    def test_gauge_set(self):
        n = self.RandomString()
        c = self.registry.MakeGauge(n)
        self.assertEqual(self.FetchGauge(n), 0, 'Gauge must start at zero')
        c.Set(999)
        self.assertEqual(self.FetchGauge(n), 999, 'Gauge must set value to parameter value')


#
# Main
#

if __name__ == '__main__':
    unittest.main()
