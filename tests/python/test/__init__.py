# Copyright © 2025 CCP ehf.

import os
import sys

# Import flavoured prometheus_module for test execution
flavor = os.environ.get("BUILDFLAVOR", "release")
if flavor == 'release':
    import prometheus_module as mod
elif flavor == 'debug':
    import prometheus_module_debug as mod
elif flavor == 'trinitydev':
    import prometheus_module_trinitydev as mod
elif flavor == 'internal':
    import prometheus_module_internal as mod
else:
    raise RuntimeError("Unknown build flavor: {}".format(flavor))

sys.modules["prometheus_module"] = mod
