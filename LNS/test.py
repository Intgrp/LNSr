import os
import subprocess
import time
from itertools import product
from glob import glob

runs = 1024
ksize_list = [1, 3, 5, 7, 9]
init_list = ['rcrs', 'td']
or_opt_list = [2, 3, 4]
two_exchange_list = [2, 3]
elo_list = [1, 2, 3, 4, 5]
seeds = [0, 42, 64]
tmax = {5   :  5,
        10  : 15,
        50  : 15,
        100 : 30,
        200 : 60}

grids = list(product(ksize_list, init_list, or_opt_list, two_exchange_list, elo_list, seeds))

def config_str(config, t):
    return "ksize=%d_init=%s_or-len=%d_ex-len=%d_elo=%d_seed=%d_t=%d" %\
           (config[0], config[1], config[2], config[3], config[4], config[5], t)

def test():
    solver = "LNSr/LNS/bin/LNS"
    insts = glob("instances/Wang_Chen/explicit_*.vrpsdptw")
    maxParalism = 36

    runningTask = 0
    processSet = set()
    for i, ins in enumerate(insts):
        print('------------------Testing instance %d---------------' % i)
        for config in grids:
            while True:
                if runningTask >= maxParalism:
                    time.sleep(0.1)
                    finished = [
                        pid for pid in processSet if pid.poll() is not None
                    ]
                    processSet -= set(finished)
                    runningTask = len(processSet)
                    continue
                else:
                    ins_id = ins[ins.rfind('_')+1:ins.find('.')]
                    with open(ins, 'r') as f:
                        t = tmax[int(f.read().strip().split('\n')[2].split(':')[1].strip())-1]
                    out_file = "results/%s_%s" % (ins_id, config_str(config, t))
                    cmd = ("%s --problem %s --pruning --O_1_eval --two_opt --two_opt_star "
                           "--ksize %d --init %s "
                           "--or_opt %d --two_exchange %d --random_removal --regret_insertion "
                           "--elo %d --random_seed %d --time %d --runs %d > %s") %\
                            (solver, ins, config[0], config[1], config[2], config[3], config[4],
                             config[5], t, runs, out_file)
                    print(cmd)
                    processSet.add(subprocess.Popen(cmd, shell=True))
                    runningTask = len(processSet)
                    break

    # check if subprocess all exits
    while processSet:
        time.sleep(5)
        print('Still %d sub process not exits' % len(processSet))
        finished = [pid for pid in processSet if pid.poll() is not None]
        processSet -= set(finished)


if __name__ == "__main__":
    os.chdir('../../')
    test()
