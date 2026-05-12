# Copyright © 2025 CCP ehf.

import sys
import unittest

# Mock built modules for test discovery
sys.modules["prometheus_module"] = sys
sys.modules["test_native"] = sys

def print_suite(suite):
    if hasattr(suite, '_exception'):
        print(suite._exception)
        sys.exit(1)
    else:
        if not hasattr(suite, '__iter__'):
            print(suite.id())
        else:
            [print_suite(suite) for suite in suite]


if __name__ == '__main__':
    suite = unittest.defaultTestLoader.discover('.')
    print_suite(suite)
