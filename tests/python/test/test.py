import gzip
import math
import string
from io import StringIO, BytesIO
import random
import time
import unittest
from urllib.error import URLError
from urllib.request import Request, urlopen
import prometheus_module
import test_native


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
        r = urlopen(url, timeout=0.5)
        result = r.read()
        return result.decode()

    def FetchLine(self, substr, port=''):
        for line in self.Fetch(port).split('\n'):
            if (substr in line) and not ('#' in line):
                return line.strip()
        return ''

    def FetchLines(self, substr, port=''):
        result = []
        for line in self.Fetch(port).split('\n'):
            if (substr in line) and not ('#' in line):
                result.append(line)
        return result

    def FetchLinesWithComments(self, substr, port=''):
        result = []
        for line in self.Fetch(port).split('\n'):
            if (substr in line):
                result.append(line)
        return result

    def IsServerListening(self, port=''):
        try:
            content = self.Fetch(port)
            has_valid_content = 'exposer' in content
            return has_valid_content
        except URLError:
            return False

    def RandomString(self, length=6):
        return ''.join(random.choice(string.ascii_uppercase) for _ in range(length))

#
# Server
#
class TestServing(TestBase):
    def ExpectServeSuccess(self, port):
        self.assertFalse(self.IsServerListening(port))
        r = prometheus_module.MetricRegistry()
        self.assertTrue(r.Serve(port))
        r.StopServing()
        self.assertFalse(self.IsServerListening(port))
        r = None

    def ExpectServeFailure(self, port):
        r = prometheus_module.MetricRegistry()
        self.assertFalse(r.Serve(port))
        r.StopServing()
        r = None

    def test_server_start_stop(self):
        self.assertFalse(self.IsServerListening(), 'Server must not listen until Serve is called')
        self.assertTrue(self.registry.Serve(self.port))
        self.assertTrue(self.IsServerListening(), 'Server must listen after Serve is called')
        self.registry.StopServing()
        self.assertFalse(self.IsServerListening(), 'Server must stop listening after StopServing is called')

    def test_serve_port_in_use(self):
        self.assertFalse(self.IsServerListening('20800'))
        self.assertTrue(self.registry.Serve('20800'))
        self.assertTrue(self.IsServerListening('20800'))

        registry2 = prometheus_module.MetricRegistry()
        self.assertFalse(registry2.Serve('20800'), 'Serve() must return False if the requested port is already in use).')

        self.registry.StopServing()

    def test_gzip_supported(self):
        self.assertTrue(self.registry.Serve(self.port))
        url = 'http://localhost:' + self.port
        request = Request(url)
        request.add_header('Accept-encoding', 'gzip')
        response = urlopen(request)
        encoding = response.info().get('Content-Encoding')
        self.assertEqual(encoding, 'gzip', 'Exposer must provide gzipped data when requested')
        buf = BytesIO(response.read())
        f = gzip.GzipFile(fileobj=buf)
        content = f.read().decode()
        self.assertTrue('exposer' in content, 'Returned content must unzip correctly and contain metrics')
        self.registry.StopServing()


#
# Counter
#
class TestCounter(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        self.registry.Serve(self.port)

    def tearDown(self):
        self.registry.StopServing()
        TestBase.tearDown(self)

    def FetchCounter(self, name):
        line = self.FetchLine(name)
        if not line:
            return 0
        byte_value = line.split(' ')[-1]
        return float(byte_value)


    def test_MakeCounter(self):
        n = self.RandomString()
        self.assertFalse(self.FetchLine(n))
        metric_family = self.registry.MakeCounter(n)
        self.assertIsNotNone(metric_family)
        self.assertFalse(self.FetchLine(n)) # Lazy


    def test_MakeCounter_with_labels(self):
        n = 'name' + self.RandomString()
        label_name = 'label_name' + self.RandomString()
        label_value = 'label_value' + self.RandomString()
        label_name2 = self.RandomString()
        label_value2 = self.RandomString()

        counter_family = self.registry.MakeCounter(n, [label_name, label_name2])
        counter = counter_family.WithLabelValues({label_name:label_value, label_name2:label_value2})

        line = self.FetchLine(label_value)
        self.assertTrue(label_name in line)
        self.assertTrue(label_value in line)
        self.assertTrue(label_name2 in line)
        self.assertTrue(label_value2 in line)

    def test_MakeCounter_lazy_instantiates(self):
        n = 'name' + self.RandomString()
        label_name = 'label_name' + self.RandomString()

        counter_family = self.registry.MakeCounter(n, [label_name])
        line = self.FetchLine(n)
        self.assertEqual(line, '', 'Counter with empty label values must not be published unless modified')

        counter_family.Increment()
        self.assertEqual(self.FetchCounter(n), 1, 'Counter with empty label values must be published after being modified')

        line = self.FetchLine(n)
        self.assertTrue(label_name in line)

    def test_MakeCounter_preserves_label_order(self):
        n = 'name' + self.RandomString()
        label_name_a = 'label_name_a' + self.RandomString()
        label_value_a = 'label_value_a' + self.RandomString()
        label_name_b = 'label_name_b' + self.RandomString()
        label_value_b = 'label_value_b' + self.RandomString()
        label_name_c = 'label_name_c' + self.RandomString()
        label_value_c = 'label_value_c' + self.RandomString()

        # Define the counter with labels in non-alphabetical order
        counter_family = self.registry.MakeCounter(n, [label_name_b, label_name_a, label_name_c])

        # Make sure WithLabelValues maps the keys to values regardless of order
        counter = counter_family.WithLabelValues({label_name_a:label_value_a, label_name_c:label_value_c, label_name_b:label_value_b})

        line = self.FetchLine(label_value_a)
        # label_name_aXYVIYD="label_value_aLSBDZH",label_name_bOOHOMG="label_value_bHJLADD"
        label_string_a = '{}="{}"'.format(label_name_a, label_value_a)
        label_string_b = '{}="{}"'.format(label_name_b, label_value_b)
        label_string_c = '{}="{}"'.format(label_name_c, label_value_c)
        self.assertTrue(label_string_a in line)
        self.assertTrue(label_string_b in line)
        self.assertTrue(label_string_c in line)

    def test_counter_increment(self):
        n = self.RandomString()
        label_name = 'label_name' + self.RandomString()
        label_value = 'label_value' + self.RandomString()
        f = self.registry.MakeCounter(n, [label_name])
        c = f.WithLabelValues({label_name:label_value})

        self.assertEqual(self.FetchCounter(n), 0, 'Counter must start at zero')
        c.Increment()
        self.assertEqual(self.FetchCounter(n), 1, 'Counter must increment by one by default')
        c.Increment(10)
        self.assertEqual(self.FetchCounter(n), 11, 'Counter must increment by parameter value')

    def test_counter_increment_with_labels(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_name2 = self.RandomString()
        # label_name2 exists to show that only providing label_name (omitting label_name2) in WithLabelValues still works
        # WithLabelValues({label_name:whatever}) (omitting label_name2) is the same as WithLabelValues({label_name:whatever,label_name2:''})
        f = self.registry.MakeCounter(n, [label_name, label_name2])

        label_value = self.RandomString()
        label_value2 = self.RandomString()
        c = f.WithLabelValues({label_name:label_value})
        c2 = f.WithLabelValues({label_name:label_value2})

        self.assertEqual(self.FetchCounter(label_value), 0, 'Counter with labels must start at zero')
        c.Increment()
        self.assertEqual(self.FetchCounter(label_value), 1, 'Counter with labels must increment by one by default')

        f.WithLabelValues({label_name:label_value}).Increment(10)
        self.assertEqual(self.FetchCounter(label_value), 11, 'Counter with labels must increment by parameter value')

        f.WithLabelValues({label_name:label_value2}).Increment(1)
        self.assertEqual(self.FetchCounter(label_value), 11, 'Counters with distinct label values must represent their own time series')
        self.assertEqual(self.FetchCounter(label_value2), 1, 'Counters with distinct label values must represent their own time series')

    def test_counter_with_labels_reuses_objects(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_value2 = self.RandomString()

        f = self.registry.MakeCounter(n, [label_name])
        c = f.WithLabelValues({label_name:label_value})
        c_same = f.WithLabelValues({label_name:label_value})
        c_different = f.WithLabelValues({label_name:label_value2})

        self.assertTrue(c is c_same, 'Metrics with identical name and label values must re-use the object')
        self.assertFalse(c is c_different, 'Metrics with different name or label values must use distinct objects')

    def test_counter_decrement_fails(self):
        n = self.RandomString()
        c = self.registry.MakeCounter(n)
        c.Increment(-1)
        self.assertEqual(self.FetchCounter(n), 0, 'Counter must not decrement')

    def test_counter_increment_by_zero_increments_by_one(self):
        # This sounds like a conflict, but is a technical necessity because the underlying native module
        # cannot differentiate between the absence of a value and the actual zero value.
        # In any case, the API only allows calling Increment with positive values.  Calling Increment(0) is technically an error.
        n = self.RandomString()
        c = self.registry.MakeCounter(n)
        c.Increment(0)
        self.assertEqual(self.FetchCounter(n), 1, 'Incrementing counter by zero must increment by one')

    def test_counters_with_different_label_values_share_one_type_definition(self):
        # The page pulled by prometheus contains a TYPE definition for each metric like this:
        #   '# TYPE my_counter_name counter'
        # When there are multiple metrics with the same name but different label values, they should share one definition
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_value2 = label_value + '-2'
        family = self.registry.MakeCounter(n, [label_name])
        counter1 = family.WithLabelValues({label_name: label_value})
        counter2 = family.WithLabelValues({label_name: label_value2})
        lines = self.FetchLinesWithComments('TYPE ' + n)
        self.assertEqual(len(lines), 1)


#
# Gauge
#
class TestGauge(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        self.registry.Serve(self.port)

    def tearDown(self):
        self.registry.StopServing()
        TestBase.tearDown(self)

    def FetchGauge(self, name):
        line = self.FetchLine(name)
        if not line:
            return 0
        string_value = line.split(' ')[-1]
        return float(string_value)


    def test_MakeGauge(self):
        n = self.RandomString()
        self.assertFalse(self.FetchLine(n))
        metric_family = self.registry.MakeGauge(n)
        self.assertIsNotNone(metric_family)
        self.assertFalse(self.FetchLine(n)) # Lazy

    def test_MakeGauge_with_labels(self):
        n = 'name' + self.RandomString()
        label_name = 'label_name' + self.RandomString()
        label_value = 'label_value' + self.RandomString()
        label_name2 = self.RandomString()
        label_value2 = self.RandomString()

        gauge_family = self.registry.MakeGauge(n, [label_name, label_name2])
        gauge = gauge_family.WithLabelValues({label_name:label_value, label_name2:label_value2})

        line = self.FetchLine(label_value)
        self.assertTrue(label_name in line)
        self.assertTrue(label_value in line)
        self.assertTrue(label_name2 in line)
        self.assertTrue(label_value2 in line)

    def test_MakeGauge_lazy_instantiates(self):
        n = 'name' + self.RandomString()
        label_name = 'label_name' + self.RandomString()

        gauge_family = self.registry.MakeGauge(n, [label_name])
        line = self.FetchLine(n)
        self.assertEqual(line, '', 'Gauge with empty label values must not be published unless modified')

        gauge_family.Set(1)
        self.assertEqual(self.FetchGauge(n), 1, 'Gauge with empty label values must be published after being modified')

        line = self.FetchLine(n)
        self.assertTrue(label_name in line)

    def test_MakeGauge_preserves_label_order(self):
        n = 'name' + self.RandomString()
        label_name_a = 'label_name_a' + self.RandomString()
        label_value_a = 'label_value_a' + self.RandomString()
        label_name_b = 'label_name_b' + self.RandomString()
        label_value_b = 'label_value_b' + self.RandomString()
        label_name_c = 'label_name_c' + self.RandomString()
        label_value_c = 'label_value_c' + self.RandomString()

        # Define the counter with labels in non-alphabetical order
        family = self.registry.MakeGauge(n, [label_name_b, label_name_a, label_name_c])

        # Make sure WithLabelValues maps the keys to values regardless of order
        m = family.WithLabelValues({label_name_a:label_value_a, label_name_c:label_value_c, label_name_b:label_value_b})

        line = self.FetchLine(label_value_a)
        # label_name_aXYVIYD="label_value_aLSBDZH",label_name_bOOHOMG="label_value_bHJLADD"
        label_string_a = '{}="{}"'.format(label_name_a, label_value_a)
        label_string_b = '{}="{}"'.format(label_name_b, label_value_b)
        label_string_c = '{}="{}"'.format(label_name_c, label_value_c)
        self.assertTrue(label_string_a in line)
        self.assertTrue(label_string_b in line)
        self.assertTrue(label_string_c in line)

    def test_gauge_increment(self):
        n = self.RandomString()
        g = self.registry.MakeGauge(n)
        self.assertEqual(self.FetchGauge(n), 0, 'Gauge must start at zero')
        g.Increment()
        self.assertEqual(self.FetchGauge(n), 1, 'Gauge must increment by one by default')
        g.Increment(10)
        self.assertEqual(self.FetchGauge(n), 11, 'Gauge must increment by parameter value')
        g.Increment(0)
        self.assertEqual(self.FetchGauge(n), 12, 'Increment by zero must increment by one')

    def test_gauge_decrement(self):
        n = self.RandomString()
        g = self.registry.MakeGauge(n)
        self.assertEqual(self.FetchGauge(n), 0, 'Gauge must start at zero')
        g.Decrement()
        self.assertEqual(self.FetchGauge(n), -1, 'Gauge must decrement by one by default')
        g.Decrement(10)
        self.assertEqual(self.FetchGauge(n), -11, 'Gauge must decrement by parameter value')
        g.Decrement(0)
        self.assertEqual(self.FetchGauge(n), -12, 'Decrement by zero must decrement by one')

    def test_gauge_set(self):
        n = self.RandomString()
        m = self.registry.MakeGauge(n)
        self.assertEqual(self.FetchGauge(n), 0, 'Gauge must start at zero')
        m.Set(999)
        self.assertEqual(self.FetchGauge(n), 999, 'Gauge must set value to parameter value')

    def test_gauge_set_with_labels(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_name2 = self.RandomString()
        # label_name2 exists to show that only providing label_name (omitting label_name2) in WithLabelValues still works
        # WithLabelValues({label_name:whatever}) (omitting label_name2) is the same as WithLabelValues({label_name:whatever,label_name2:''})
        f = self.registry.MakeGauge(n, [label_name, label_name2])

        label_value = self.RandomString()
        label_value2 = self.RandomString()
        m = f.WithLabelValues({label_name:label_value})
        m2 = f.WithLabelValues({label_name:label_value2})

        self.assertEqual(self.FetchGauge(label_value), 0, 'Gauge with labels must start at zero')
        m.Set(999)
        self.assertEqual(self.FetchGauge(label_value), 999, 'Gauge must set value to parameter value')

        f.WithLabelValues({label_name:label_value}).Set(10)
        self.assertEqual(self.FetchGauge(label_value), 10, 'Gauge with labels must set value to parameter value')

        f.WithLabelValues({label_name:label_value2}).Set(1)
        self.assertEqual(self.FetchGauge(label_value), 10, 'Metrics with distinct label values must represent their own time series')
        self.assertEqual(self.FetchGauge(label_value2), 1, 'Metrics with distinct label values must represent their own time series')

    def test_gauge_with_labels_reuses_objects(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_value2 = self.RandomString()

        f = self.registry.MakeGauge(n, [label_name])
        m = f.WithLabelValues({label_name:label_value})
        m_same = f.WithLabelValues({label_name:label_value})
        m_different = f.WithLabelValues({label_name:label_value2})

        self.assertTrue(m is m_same, 'Metrics with identical name and label values must re-use the object')
        self.assertFalse(m is m_different, 'Metrics with different name or label values must use distinct objects')

    def test_gauges_with_different_label_values_share_one_type_definition(self):
        # The page pulled by prometheus contains a TYPE definition for each metric like this:
        #   '# TYPE my_metric_name gauge'
        # When there are multiple metrics with the same name but different label values, they should share one definition
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_value2 = label_value + '-2'
        family = self.registry.MakeGauge(n, [label_name])
        gauge1 = family.WithLabelValues({label_name:label_value})
        gauge2 = family.WithLabelValues({label_name:label_value2})
        lines = self.FetchLinesWithComments('TYPE ' + n)
        self.assertEqual(len(lines), 1)


#
# Histogram
#
class TestHistogram(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        self.registry.Serve(self.port)

    def tearDown(self):
        self.registry.StopServing()
        TestBase.tearDown(self)

    def FetchHistogram(self, name):
        lines = self.FetchLines(name)
        if len(lines) == 0:
            return 0

        count = 0
        sum = 0.0
        buckets = []
        for line in lines:
            if '_count' in line:
                count = int(line.split(' ')[-1])
            if '_sum' in line:
                sum = float(line.split(' ')[-1])
            if '_bucket' in line:
                buckets.append(int(line.split(' ')[-1]))

        result = {}
        result['count'] = count
        result['sum'] = sum
        result['buckets'] = buckets
        return result

    def test_MakeHistogram(self):
        n = self.RandomString()
        self.assertFalse(self.FetchLine(n))
        metric_family = self.registry.MakeHistogram(n)
        self.assertIsNotNone(metric_family)
        self.assertFalse(self.FetchLine(n)) # Lazy

    def test_MakeHistogram_with_labels(self):
        n = 'name' + self.RandomString()
        label_name = 'label_name' + self.RandomString()
        label_value = 'label_value' + self.RandomString()
        label_name2 = self.RandomString()
        label_value2 = self.RandomString()

        f = self.registry.MakeHistogram(n, labels=[label_name, label_name2])
        f.WithLabelValues({label_name:label_value, label_name2:label_value2})

        line = self.FetchLine(label_value)
        self.assertTrue(label_name in line)
        self.assertTrue(label_value in line)
        self.assertTrue(label_name2 in line)
        self.assertTrue(label_value2 in line)

    def test_MakeHistogram_lazy_instantiates(self):
        n = 'name' + self.RandomString()
        label_name = 'label_name' + self.RandomString()

        histogram_family = self.registry.MakeHistogram(n, [label_name])
        line = self.FetchLine(n)
        self.assertEqual(line, '', 'Histogram with empty label values must not be published unless modified')

        histogram_family.Observe(1)
        values = self.FetchHistogram(n)
        self.assertEqual(values['count'], 1, 'Histogram with empty label values must be published after being modified')

    def test_MakeHistogram_preserves_label_order(self):
        n = 'name' + self.RandomString()
        label_name_a = 'label_name_a' + self.RandomString()
        label_value_a = 'label_value_a' + self.RandomString()
        label_name_b = 'label_name_b' + self.RandomString()
        label_value_b = 'label_value_b' + self.RandomString()
        label_name_c = 'label_name_c' + self.RandomString()
        label_value_c = 'label_value_c' + self.RandomString()

        # Define the metric with labels in non-alphabetical order
        family = self.registry.MakeHistogram(n, [label_name_b, label_name_a, label_name_c])

        # Make sure WithLabelValues maps the keys to values regardless of order
        m = family.WithLabelValues({label_name_a:label_value_a, label_name_c:label_value_c, label_name_b:label_value_b})

        line = self.FetchLine(label_value_a)
        # label_name_aXYVIYD="label_value_aLSBDZH",label_name_bOOHOMG="label_value_bHJLADD"
        label_string_a = '{}="{}"'.format(label_name_a, label_value_a)
        label_string_b = '{}="{}"'.format(label_name_b, label_value_b)
        label_string_c = '{}="{}"'.format(label_name_c, label_value_c)
        self.assertTrue(label_string_a in line)
        self.assertTrue(label_string_b in line)
        self.assertTrue(label_string_c in line)

    def test_MakeHistogram_with_boundaries(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        b = [10,100,1000]
        f = self.registry.MakeHistogram(n, labels=[label_name], boundaries=b)
        h = f.WithLabelValues({label_name:label_value})
        values = self.FetchHistogram(n)
        self.assertEqual(len(values['buckets']), len(b) + 1, 'Number of buckets must equal number of boundaries plus one)')

    def test_histogram_observe(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        b = [10,100,1000]
        f = self.registry.MakeHistogram(n, labels=[label_name], boundaries=b)
        h = f.WithLabelValues({label_name:label_value})

        values = self.FetchHistogram(n)
        self.assertEqual(values['count'], 0, 'Histogram must start with zero observations')
        self.assertEqual(values['sum'], 0.0, 'Histogram must start with zero observations')

        h.Observe(1)
        h.Observe(10)
        h.Observe(100)
        h.Observe(1000)
        h.Observe(10000)
        values = self.FetchHistogram(n)
        self.assertEqual(values['count'], 5, 'Histogram_count must increment with observations')
        self.assertEqual(values['sum'], 1.0 + 10.0 + 100.0 + 1000.0 + 10000.0, 'Histogram_sum must sum the observations')
        buckets = values['buckets']
        self.assertEqual(buckets[0], 2, 'Observed values must be recorded in their corresponding buckets')
        self.assertEqual(buckets[1], 3, 'Observed values must be recorded in their corresponding buckets')
        self.assertEqual(buckets[2], 4, 'Observed values must be recorded in their corresponding buckets')
        self.assertEqual(buckets[3], 5, 'Observed values must be recorded in their corresponding buckets')

    def test_histogram_observe_with_labels(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_name2 = self.RandomString()
        # label_name2 exists to show that only providing label_name (omitting label_name2) in WithLabelValues still works
        # WithLabelValues({label_name:whatever}) (omitting label_name2) is the same as WithLabelValues({label_name:whatever,label_name2:''})
        f = self.registry.MakeHistogram(n, [label_name, label_name2])

        label_value = self.RandomString()
        label_value2 = self.RandomString()
        m = f.WithLabelValues({label_name:label_value})
        m2 = f.WithLabelValues({label_name:label_value2})

        values = self.FetchHistogram(label_value)
        self.assertEqual(values['count'], 0, 'Histogram must start with zero observations')

        m.Observe(1)
        values = self.FetchHistogram(label_value)
        self.assertEqual(values['count'], 1, 'Histogram_count must increment with observations')

        f.WithLabelValues({label_name:label_value}).Observe(10)
        values = self.FetchHistogram(label_value)
        self.assertEqual(values['count'], 2, 'Histogram_count must increment with observations')

        f.WithLabelValues({label_name:label_value2}).Observe(1)
        values = self.FetchHistogram(label_value)
        values2 = self.FetchHistogram(label_value2)
        self.assertEqual(values['count'], 2, 'Metrics with distinct label values must represent their own time series')
        self.assertEqual(values2['count'], 1, 'Metrics with distinct label values must represent their own time series')

    def test_histogram_with_labels_reuses_objects(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_value2 = self.RandomString()

        f = self.registry.MakeHistogram(n, [label_name])
        m = f.WithLabelValues({label_name:label_value})
        m_same = f.WithLabelValues({label_name:label_value})
        m_different = f.WithLabelValues({label_name:label_value2})

        self.assertTrue(m is m_same, 'Metrics with identical name and label values must re-use the object')
        self.assertFalse(m is m_different, 'Metrics with different name or label values must use distinct objects')

    def test_histograms_with_different_label_values_share_one_type_definition(self):
        # The page pulled by prometheus contains a TYPE definition for each metric like this:
        #   '# TYPE my_metric_name histogram'
        # When there are multiple metrics with the same name but different label values, they should share one definition
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_value2 = label_value + '-2'
        family = self.registry.MakeHistogram(n, [label_name])
        metric1 = family.WithLabelValues({label_name:label_value})
        metric2 = family.WithLabelValues({label_name:label_value2})
        lines = self.FetchLinesWithComments('TYPE ' + n)
        self.assertEqual(len(lines), 1)


#
# Summary
#
class TestSummary(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        self.registry.Serve(self.port)

    def tearDown(self):
        self.registry.StopServing()
        TestBase.tearDown(self)

    def FetchSummary(self, name):
        lines = self.FetchLines(name)
        if len(lines) == 0:
            return 0

        count = 0
        sum = 0.0
        quantiles = []
        for line in lines:
            if '_count' in line:
                count = int(line.split(' ')[-1])
            if '_sum' in line:
                sum = float(line.split(' ')[-1])
            if 'quantile' in line:
                quantiles.append(float(line.split(' ')[-1]))

        result = {}
        result['count'] = count
        result['sum'] = sum
        result['quantiles'] = quantiles
        return result

    def test_MakeSummary(self):
        n = self.RandomString()
        self.assertFalse(self.FetchLine(n))
        metric_family = self.registry.MakeSummary(n)
        self.assertIsNotNone(metric_family)
        self.assertFalse(self.FetchLine(n)) # Lazy

    def test_MakeSummary_with_labels(self):
        n = 'name' + self.RandomString()
        label_name = 'label_name' + self.RandomString()
        label_value = 'label_value' + self.RandomString()
        label_name2 = self.RandomString()
        label_value2 = self.RandomString()

        f = self.registry.MakeSummary(n, labels=[label_name, label_name2])
        f.WithLabelValues({label_name:label_value, label_name2:label_value2})

        line = self.FetchLine(label_value)
        self.assertTrue(label_name in line)
        self.assertTrue(label_value in line)
        self.assertTrue(label_name2 in line)
        self.assertTrue(label_value2 in line)

    def test_MakeSummary_lazy_instantiates(self):
        n = 'name' + self.RandomString()
        label_name = 'label_name' + self.RandomString()

        summary_family = self.registry.MakeSummary(n, [label_name])
        line = self.FetchLine(n)
        self.assertEqual(line, '', 'Summary with empty label values must not be published unless modified')

        summary_family.Observe(1)
        values = self.FetchSummary(n)
        self.assertEqual(values['count'], 1, 'Summary with empty label values must be published after being modified')

    def test_MakeSummary_preserves_label_order(self):
        n = 'name' + self.RandomString()
        label_name_a = 'label_name_a' + self.RandomString()
        label_value_a = 'label_value_a' + self.RandomString()
        label_name_b = 'label_name_b' + self.RandomString()
        label_value_b = 'label_value_b' + self.RandomString()
        label_name_c = 'label_name_c' + self.RandomString()
        label_value_c = 'label_value_c' + self.RandomString()

        # Define the metric with labels in non-alphabetical order
        family = self.registry.MakeSummary(n, [label_name_b, label_name_a, label_name_c])

        # Make sure WithLabelValues maps the keys to values regardless of order
        m = family.WithLabelValues({label_name_a:label_value_a, label_name_c:label_value_c, label_name_b:label_value_b})

        line = self.FetchLine(label_value_a)
        # label_name_aXYVIYD="label_value_aLSBDZH",label_name_bOOHOMG="label_value_bHJLADD"
        label_string_a = '{}="{}"'.format(label_name_a, label_value_a)
        label_string_b = '{}="{}"'.format(label_name_b, label_value_b)
        label_string_c = '{}="{}"'.format(label_name_c, label_value_c)
        self.assertTrue(label_string_a in line)
        self.assertTrue(label_string_b in line)
        self.assertTrue(label_string_c in line)

    def test_MakeSummary_with_quantiles(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        q = [(0.1,0.05),(0.5,0.05),(0.9,0.05)]
        f = self.registry.MakeSummary(n, labels=[label_name], quantiles=q)
        s = f.WithLabelValues({label_name:label_value})

        values = self.FetchSummary(n)
        self.assertEqual(len(values['quantiles']), len(q), 'Number of quantiles must match')

    def test_summary_observe(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        tolerance = 0.05
        f = self.registry.MakeSummary(n, labels=[label_name], quantiles=[(0.1,tolerance),(0.5,tolerance),(0.9,tolerance)])
        s = f.WithLabelValues({label_name:label_value})

        values = self.FetchSummary(n)
        self.assertEqual(values['count'], 0, 'Summary must start with zero observations')
        self.assertEqual(values['sum'], 0.0, 'Summary must start with zero observations')
        self.assertEqual(len(values['quantiles']), 3, 'Summary must start with correct number of quantiles')

        s.Observe(1)
        s.Observe(10)
        s.Observe(100)
        s.Observe(1000)
        s.Observe(10000)

        values = self.FetchSummary(n)
        self.assertEqual(values['count'], 5, 'Summary_count must increment with observations')
        self.assertEqual(values['sum'], 1.0 + 10.0 + 100.0 + 1000.0 + 10000.0, 'Summary_sum must sum the observations')
        quantiles = values['quantiles']
        self.assertEqual(quantiles[0], 1.0, 'Observed values must be recorded in their corresponding quantiles')
        self.assertEqual(quantiles[1], 10.0, 'Observed values must be recorded in their corresponding quantiles')
        self.assertEqual(quantiles[2], 100.0, 'Observed values must be recorded in their corresponding quantiles')

    def test_summary_window(self):
        n = self.RandomString()

        # Make an 8-second window, split into 2 partitions, giving us 4 seconds per partition
        s = self.registry.MakeSummary(n, quantiles=[(0.5,0.0)], window_size_seconds=8, window_partitions=2)

        # Observe a sample, this is time t
        s.Observe(1)

        # Now, at t+0s, the sample should be there
        quantiles = self.FetchSummary(n)['quantiles']
        self.assertEqual(quantiles[0], 1, 'Sample must be present at t+0s')

        # Wait a second
        time.sleep(1)

        # Now, at t+1s, the sample should still be there
        quantiles = self.FetchSummary(n)['quantiles']
        self.assertEqual(quantiles[0], 1, 'Sample must be present at t+1s')

        # Wait until the 4-second mark
        time.sleep(3)

        # Now, at t+4s, the sample should be gone
        quantiles = self.FetchSummary(n)['quantiles']
        self.assertTrue(math.isnan(quantiles[0]), 'Sample must be absent at t+4s')

    def test_summary_observe_with_labels(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_name2 = self.RandomString()
        # label_name2 exists to show that only providing label_name (omitting label_name2) in WithLabelValues still works
        # WithLabelValues({label_name:whatever}) (omitting label_name2) is the same as WithLabelValues({label_name:whatever,label_name2:''})
        f = self.registry.MakeSummary(n, [label_name, label_name2])

        label_value = self.RandomString()
        label_value2 = self.RandomString()
        m = f.WithLabelValues({label_name:label_value})
        m2 = f.WithLabelValues({label_name:label_value2})

        values = self.FetchSummary(label_value)
        self.assertEqual(values['count'], 0, 'Summary must start with zero observations')

        m.Observe(1)
        values = self.FetchSummary(label_value)
        self.assertEqual(values['count'], 1, 'Summary_count must increment with observations')

        f.WithLabelValues({label_name:label_value}).Observe(10)
        values = self.FetchSummary(label_value)
        self.assertEqual(values['count'], 2, 'Summary_count must increment with observations')

        f.WithLabelValues({label_name:label_value2}).Observe(1)
        values = self.FetchSummary(label_value)
        values2 = self.FetchSummary(label_value2)
        self.assertEqual(values['count'], 2, 'Metrics with distinct label values must represent their own time series')
        self.assertEqual(values2['count'], 1, 'Metrics with distinct label values must represent their own time series')

    def test_summary_with_labels_reuses_objects(self):
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_value2 = self.RandomString()

        f = self.registry.MakeSummary(n, [label_name])
        m = f.WithLabelValues({label_name:label_value})
        m_same = f.WithLabelValues({label_name:label_value})
        m_different = f.WithLabelValues({label_name:label_value2})

        self.assertTrue(m is m_same, 'Metrics with identical name and label values must re-use the object')
        self.assertFalse(m is m_different, 'Metrics with different name or label values must use distinct objects')

    def test_summaries_with_different_label_values_share_one_type_definition(self):
        # The page pulled by prometheus contains a TYPE definition for each metric like this:
        #   '# TYPE my_metric_name summary'
        # When there are multiple metrics with the same name but different label values, they should share one definition
        n = self.RandomString()
        label_name = self.RandomString()
        label_value = self.RandomString()
        label_value2 = label_value + '-2'
        family = self.registry.MakeSummary(n, [label_name])
        metric1 = family.WithLabelValues({label_name:label_value})
        metric2 = family.WithLabelValues({label_name:label_value2})
        lines = self.FetchLinesWithComments('TYPE ' + n)
        self.assertEqual(len(lines), 1)


#
# Native
#
class TestNative(TestBase):
    def test_native_tests(self):
        self.assertTrue(test_native.RunTests(self.registry.GetCapsule()), 'Native tests must pass')


#
# Main
#
if __name__ == '__main__':
    unittest.main()
