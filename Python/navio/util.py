import subprocess as sub
import sys

def check_apm():
    ret = sub.call("ps -AT -o comm= | grep -q ^sched-timer", shell=True)
    if ret == 0:
        sys.exit("APM is running. Can't launch the example")
