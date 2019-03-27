import unittest
import urllib2

import prometheus_module

# Helpers

class TestBase(unittest.TestCase):

    def setUp(self):
        self.port = '20800'
        self.registry = prometheus_module.MetricRegistry()

    def Fetch(self, port=''):
        if port == '':
            port = self.port
        url = 'http://localhost:' + port
        return urllib2.urlopen(url).read()

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
        print 'todo'

class TestCounter(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        self.registry.Serve(self.port)

    def tearDown(self):
        TestBase.tearDown(self)
        self.registry.StopServing()

    def test_counter_counts(self):
        self.assertTrue(True)


if __name__ == '__main__':
    unittest.main()
