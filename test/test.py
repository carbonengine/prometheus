import unittest
import urllib2

import prometheus_module

# Helpers

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

class TestServing(TestBase):
    def test_server_start_stop(self):
        self.assertFalse(self.IsServerListening(), 'Server must not listen until Serve is called')
        self.registry.Serve(self.port)
        self.assertTrue(self.IsServerListening(), 'Server must listen after Serve is called')
        self.registry.StopServing()
        self.assertFalse(self.IsServerListening(), 'Server must stop listening after StopServing is called')
        
    def test_bad_port_formats(self):
        self.assertTrue(True)
        # todo. these currently crash the program. not exceptions. just fire and burning.
        #self.registry.Serve('http://localhost:20800')
        #self.registry.Serve(':20800')
        #self.registry.Serve('')
        #self.registry.Serve()

    def test_good_port_formats(self):
        self.assertTrue(True)
        # todo

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

    def test_counter_increment_succeeds(self):
        n = 'counter1'
        c = self.registry.MakeCounter(n)
        self.assertEqual(self.FetchCounter(n), 0, 'Counter must start at zero')
        c.Increment()
        self.assertEqual(self.FetchCounter(n), 1, 'Counter must increment by one as default')
        c.Increment(10)
        self.assertEqual(self.FetchCounter(n), 11, 'Counter must increment by parameter value')

    def test_counter_decrement_fails(self):
        n = 'counter1'
        c = self.registry.MakeCounter(n)
        self.assertEqual(self.FetchCounter(n), 0, 'Counter must start at zero')
        c.Increment()
        self.assertEqual(self.FetchCounter(n), 1, 'Counter must increment by one as default')
        c.Increment(-1)
        self.assertEqual(self.FetchCounter(n), 1, 'Counter must not decrement')


if __name__ == '__main__':
    unittest.main()
