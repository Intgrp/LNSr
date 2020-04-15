import os
import subprocess
import time
from itertools import product
from glob import glob

runs = 1024
ksize_list = [-1]
# init_list = ['rcrs', 'td']
init_list = ['rcrs']
or_opt_list = [5]
two_exchange_list = [5]
elo_list = [50]
seeds = [42]
tmax = {5   :  5,
        10  : 15,
        25  : 15,
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
    maxParalism = 14

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
                           "--or_opt %d --two_exchange %d --random_removal --related_removal "
                           "--regret_insertion --greedy_insertion "
                           "--elo %d --random_seed %d --time %d --runs %d > %s") %\
                            (solver, ins, config[0], config[1], config[2], config[3], config[4],
                             config[5], t, runs, out_file)
                    print(cmd)
                    processSet.add(subprocess.Popen(cmd, shell=True))
                    # pid = subprocess.Popen(cmd, shell=True)
                    # pid.communicate()
                    runningTask = len(processSet)
                    break

    # check if subprocess all exits
    while processSet:
        time.sleep(5)
        print('Still %d sub process not exits' % len(processSet))
        finished = [pid for pid in processSet if pid.poll() is not None]
        processSet -= set(finished)

    # extract results
    f = open('summary', 'w+')
    ins_names = ["rcdp1001", "rcdp1004", "rcdp1007", "rcdp2501", "rcdp2504", "rcdp2507",\
                 "rcdp5001", "rcdp5004", "rcdp5007", "rdp101", "rdp102", "rdp103", "rdp104",\
                 "rdp105", "rdp106", "rdp107", "rdp108", "rdp109", "rdp110", "rdp111", "rdp112",\
                 "cdp101", "cdp102", "cdp103", "cdp104", "cdp105", "cdp106", "cdp107", "cdp108",\
                 "cdp109", "rcdp101", "rcdp102", \
                 "rcdp103", "rcdp104", "rcdp105", "rcdp106", "rcdp107", "rcdp108", "rdp201", \
                 "rdp202", "rdp203", "rdp204", "rdp205", "rdp206", "rdp207", "rdp208", \
                 "rdp209", "rdp210", "rdp211", "cdp201", "cdp202", "cdp203", "cdp204", \
                 "cdp205", "cdp206", "cdp207", "cdp208", "rcdp201", "rcdp202", \
                 "rcdp203", "rcdp204", "rcdp205", "rcdp206", "rcdp207", "rcdp208"]
    for ins in ins_names:
        out_file = glob("results/%s*" % ins)
        if len(out_file) != 1:
            f.write('Exists not 1 outfie for %s\n' % out_file)
            continue
        out_file = out_file[0]
        lines = open(out_file, 'r').read().strip().split('\n')
        for line in lines:
            if 'DISPATCHINGCOST' in line:
                dispatch = float(line.split(':')[-1].strip())
                break
        for line in reversed(lines):
            if 'Total cost' in line:
                total_cost = float(line.split(':')[-1].strip('. '))
                break
        for line in reversed(lines):
            if 'vehicle (route) number' in line:
                v_number = int(line.split(':')[-1].strip('. '))
                break
        total_cost = total_cost - dispatch * v_number
        f.write('%d %.4f\n' % (v_number, total_cost))
    f.close()

if __name__ == "__main__":
    os.chdir('../../')
    test()
