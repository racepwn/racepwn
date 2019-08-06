import os
from sys import platform as _platform

# If you want to use local build, change this path to build folder
#
# Example:
# RACE_INC_DIR = "~/github/racepwn/lib/include/"
# RACE_LIB_DIR = "~/github/racepwn/build/lib/"
# RACE_LIB = "~/github/racepwn/build/lib/librace.so (or .dylib for OSX)
# RACE_HEADERS = "~/github/racepwn/lib/include/race.h"

def detect_lib():
    if _platform == "linux" or _platform == "linux2":
        return ".so"
    elif _platform == "darwin":
        return ".dylib"

RACE_INC_DIR = "/usr/local/include/race/"
RACE_LIB_DIR = "/usr/local/lib/"
RACE_LIB = "librace"+detect_lib()
RACE_HEADERS = "/usr/local/include/race/race.h"
RACE_RAW_HEADERS = "/usr/local/include/race/race_raw.h"

# WARNING!!!!
# Enum broken in ctypesgen.
# https://github.com/davidjamesca/ctypesgen/issues/60
#
# My pull still pending.
# So use my fork https://github.com/loonydev/ctypesgen

# WARNING2!!!!
# If you use python3 you may have problem with librace after generation
#
# Known issues:
# unicode - just delete, in python3 str is equesl to unicode

# Set location of ctypesgen.py installed or copied from
# https://github.com/davidjamesca/ctypesgen
#
# Example:
# ctypesgen_path = '~/github/ctypesgen/ctypesgen.py'  # - local
# ctypesgen_path = 'ctypesgen.py'  # - installed
#
# For Python3 use this fork https://github.com/Sillern/ctypesgen.git
ctypesgen_path = '~/github/ctypesgen_python3/ctypesgen/ctypesgen.py'  # - local
# ctypesgen_path = 'ctypesgen.py'  # - installed

wrapper_filename = 'librace.py'
cmd = "LD_LIBRARY_PATH={} {} -I {} -L {} -l {} {} {} -o {} ".format(
RACE_LIB_DIR, ctypesgen_path, RACE_INC_DIR, RACE_LIB_DIR, RACE_LIB,
RACE_HEADERS,RACE_RAW_HEADERS, wrapper_filename)

print(cmd)
os.system(cmd)
