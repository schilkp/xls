# -*- Python -*-

import os
import platform
import re
import subprocess
import tempfile

import lit.formats
import lit.util

# Configuration file for the 'lit' test runner.

# name: The name of this test suite.
config.name = "MLIR_TRANSLATE"

config.test_format = lit.formats.ShTest(True)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = [".ir"]

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.dirname(__file__)

# # excludes: A list of directories to exclude from the testsuite.
config.excludes = []
