""" transform existing EUC_2D instances to EXPLICIT instances """
import os
from glob import glob
import math
import numpy as np

def transform():
    insts = glob('instances/Wang_Chen/*.txt')
    for ins in insts:
        new_ins = "instances/Wang_Chen/explicit_" + ins[ins.rfind('/')+1:ins.find('.')] +\
                  ".vrpsdptw"
        new_f = open(new_ins, 'w+')
        with open(ins, 'r') as f:
            content = f.read().strip().split('\n')
            new_f.write(content[0].strip() + "\n")
            values = content[4].strip().split()
            cus_num = int(values[0])
            veh_num = int(values[1])
            capacity = float(values[2])
            nodes = np.zeros((cus_num+1, 8))

            index = 0
            for line in content[9:]:
                values = line.strip().split()
                nodes[index, 0] = index
                nodes[index, 1:] = list(map(float, values[1:])) 
                index += 1
            new_f.write("TYPE : VRPSDPTW\n")
            new_f.write("DIMENSION : %d\n" % (cus_num+1))
            new_f.write("VEHICLES : %d\n" % veh_num)
            new_f.write("DISPATCHINGCOST : %d\n" % 2000)
            new_f.write("UNITCOST : %.1f\n" % 1.0)
            new_f.write("CAPACITY : %.1f\n" % capacity)
            new_f.write("EDGE_WEIGHT_TYPE : EXPLICIT\n")
            new_f.write("NODE_SECTION\n")
            for i in range(cus_num+1):
                new_f.write("%d,%.1f,%.1f,%d,%d,%d\n" %\
                            (int(nodes[i, 0]), nodes[i, 3], nodes[i, 4],\
                             int(nodes[i, 5]), int(nodes[i, 6]), int(nodes[i, 7])))
            new_f.write("DISTANCETIME_SECTION\n")
            for i in range(cus_num+1):
                for j in range(cus_num+1):
                    if i == j:
                        continue
                    dist = math.sqrt((nodes[i, 1] - nodes[j, 1]) ** 2 +\
                                     (nodes[i, 2] - nodes[j, 2]) ** 2)
                    new_f.write("%d,%d,%.6f,%.6f\n" % (i, j, dist, dist))
            new_f.write("DEPOT_SECTION\n%d\n" % int(nodes[0, 0]))
            new_f.close()

        new_f.close()

if __name__ == "__main__":
    os.chdir('../../')
    transform()
