# PlatformIO pre-script: runs tools/check_safety_sources.py before every
# host test run, and stops the build if the relay safety path is broken.
Import("env")  # noqa: F821 - provided by PlatformIO's SCons

import os
import subprocess
import sys

_script = os.path.join(env.subst("$PROJECT_DIR"), "tools",  # noqa: F821
                       "check_safety_sources.py")
if subprocess.run([sys.executable, _script], check=False).returncode != 0:
    env.Exit(1)  # noqa: F821
