from dftracer.logger.env import *

import dftracer.logger.common
if DFTRACER_ENABLE:
    import pydftracer_dbg as profiler
    dftracer.logger.common.profiler = profiler

from dftracer.logger.common import *
