import os
import subprocess
from glob import glob

def test():
    solver = "LNSr/LNS/bin/LNS"
    insts = glob("instances/Wang_Chen/explicit_*.vrpsdptw")
    for ins in insts:
        out_file = "results/%s" % ins[ins.rfind('_')+1:ins.find('.')]
        cmd = ("%s --problem %s --pruning --O_1_eval --two_opt --two_opt_star "
               "--or_opt --two_exchange --random_removal --regret_insertion "
               "--elo 3 > %s") % (solver, ins, out_file)
        print("%s\n" % cmd)
        pid = subprocess.Popen(cmd, shell=True)
        pid.communicate()

if __name__ == "__main__":
    os.chdir('../../')
    test()
