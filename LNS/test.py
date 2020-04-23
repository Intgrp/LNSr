import os
import sys
import math
import decimal
import subprocess
import time
from itertools import product
from glob import glob

runs = 1024
latin_list = ['on', 'off']
ksize_list = ['sqrt', 0.1, 0.3, 0.5, 0.75, 1]
elo_list = list(range(1, 51, 2))
# init_list = ['rcrs', 'td']
init_list = ['rcrs']
or_opt_list = [5]
two_exchange_list = [5]
seeds = [42]
tmax = {5   :  5,
        10  : 15,
        25  : 15,
        50  : 15,
        100 : 30,
        200 : 60}
grids = list(product(latin_list, ksize_list, elo_list))

def extract(L):
    # extract results
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
    for ins_id in ins_names:
        out_file = "results/%s_%s" % (ins_id, config_str_1(L))
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
        total_cost = float(round(decimal.Decimal(total_cost), 2))
        print(ins_id, v_number, total_cost)

def summary():
    baselines = {"rcdp1001": [3, 348.98],
                 "rcdp1004": [2, 216.69],
                 "rcdp1007": [3, 243.17],
                 "rcdp2501": [5, 551.05],
                 "rcdp2504": [4, 473.46],
                 "rcdp2507": [5, 540.87],
                 "rcdp5001": [9, 994.18],
                 "rcdp5004": [6, 733.21],
                 "rcdp5007": [7, 809.71],
                 "rdp101": [19, 1653.53],
                 "rdp102": [17, 1493.26],
                 "rdp103": [14, 1225.44],
                 "rdp104": [10, 994.41],
                 "rdp105": [15, 1364.07],
                 "rdp106": [13, 1240.47],
                 "rdp107": [11, 1102.87],
                 "rdp108": [10, 961.56],
                 "rdp109": [12, 1174.8],
                 "rdp110": [12, 1101.55],
                 "rdp111": [11, 1073.36],
                 "rdp112": [11, 981.75],
                 "cdp101": [11, 990.09],
                 "cdp102": [10, 941.49],
                 "cdp103": [10, 907.4],
                 "cdp104": [10, 881.24],
                 "cdp105": [11, 980.55],
                 "cdp106": [11, 878.29],
                 "cdp107": [11, 909.23],
                 "cdp108": [11, 920.3],
                 "cdp109": [10, 948.17],
                 "rcdp101": [15, 1665.55],
                 "rcdp102": [13, 1587.36],
                 "rcdp103": [12, 1340.24],
                 "rcdp104": [10, 1231.78],
                 "rcdp105": [15, 1563.75],
                 "rcdp106": [13, 1414.65],
                 "rcdp107": [12, 1273.22],
                 "rcdp108": [11, 1171.59],
                 "rdp201": [4, 1264.47],
                 "rdp202": [4, 1099.07],
                 "rdp203": [3, 953.47],
                 "rdp204": [3, 750.55],
                 "rdp205": [3, 1027.4],
                 "rdp206": [3, 968.14],
                 "rdp207": [3, 837.08],
                 "rdp208": [3, 746.31],
                 "rdp209": [3, 961.01],
                 "rdp210": [3, 988.39],
                 "rdp211": [3, 788.62],
                 "cdp201": [3, 591.56],
                 "cdp202": [3, 591.56],
                 "cdp203": [3, 591.17],
                 "cdp204": [3, 598.24],
                 "cdp205": [3, 588.88],
                 "cdp206": [3, 599.49],
                 "cdp207": [3, 588.29],
                 "cdp208": [3, 588.32],
                 "rcdp201": [4, 1464.97],
                 "rcdp202": [4, 1182.84],
                 "rcdp203": [4, 963.76],
                 "rcdp204": [3, 838.19],
                 "rcdp205": [4, 1335.68],
                 "rcdp206": [4, 1100.27],
                 "rcdp207": [3, 1161.29],
                 "rcdp208": [3, 844.19]}
    context = decimal.getcontext()
    context.rounding = decimal.ROUND_HALF_UP
    f = open('summary', 'w+')
    f.write('config, [v_better, v_worse, c_better, c_worse, equal], [total better,worse,equal]\n')
    for config in grids:
        v_better = 0
        v_worse = 0
        c_better = 0
        c_worse = 0
        equal = 0
        for ins in insts:
            ins_id = ins[ins.rfind('_')+1:ins.find('.')]
            if ins_id not in baselines:
                continue
            out_file = "results/%s_%s" % (ins_id, config_str(config))
            if not os.path.isfile(out_file):
                f.write('Erro: %s not exists\n' % out_file)

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
            total_cost = float(round(decimal.Decimal(total_cost), 2))
            if v_number < baselines[ins_id][0]:
                v_better += 1
            elif v_number > baselines[ins_id][0]:
                v_worse += 1
            elif total_cost < baselines[ins_id][1]:
                c_better += 1
            elif total_cost > baselines[ins_id][1]:
                c_worse += 1
            else:
                equal += 1
        f.write('%s, [%d, %d, %d, %d, %d], [%d, %d, %d]\n' %
                (str(config), v_better, v_worse, c_better, c_worse, equal,
                 v_better+c_better, v_worse+c_worse, equal))
    f.close()

# ('on', 0.75, 9)
def config_str_1(L):
    return "latin=%s_ksize=%s_elo=%s" %\
           (L[0], L[1], L[2])

def config_str(config):
    return "latin=%s_ksize=%s_elo=%d" %\
           (config[0], str(config[1]), config[2])

def test():
    solver = "LNSr/LNS/bin/LNS"
    maxParalism = 14

    runningTask = 0
    processSet = set()
    for config in grids:
        print('--------------------Testing %s ----------------------' % str(config))
        for i, ins in enumerate(insts):
            print('------------------Testing instance %d---------------' % i)
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
                        cus_num = int(f.read().strip().split('\n')[2].split(':')[1].strip())-1
                        t = tmax[cus_num]
                    out_file = "results/%s_%s" % (ins_id, config_str(config))
                    cmd = ("%s --problem %s %s --pruning --O_1_eval --two_opt --two_opt_star "
                           "--ksize %d --init %s "
                           "--or_opt %d --two_exchange %d --random_removal --related_removal "
                           "--regret_insertion --greedy_insertion "
                           "--elo %d --random_seed %d --time %d --runs %d > %s") %\
                            (solver, ins, '--latin' if config[0] == 'on' else '',
                             math.floor(math.sqrt(cus_num)) if config[1] == 'sqrt' else\
                             math.floor(config[1] * cus_num),
                             init_list[0], or_opt_list[0], two_exchange_list[0], config[2],
                             seeds[0], t, runs, out_file)
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

if __name__ == "__main__":
    os.chdir('../../')
    insts = glob("instances/Wang_Chen/explicit_*.vrpsdptw")
    if sys.argv[1] == 'test':
        test()
    elif sys.argv[1] == 'summary':
        summary()
    elif sys.argv[1] == 'extract':
        extract(sys.argv[2:])
    os.chdir('LNSr/LNS/')
