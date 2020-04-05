"""
Generate VRPSPDTW benchmarks based on jdata instances
5 200-dim instances <- 1201
5 400-dim instances <- 1201
5 600-dim instances <- 1301
5 800-dim instances <- 1401
5 1000-dim instances <- 1501
5 1200-dim instances <- 1601
"""
import random as rd
import sys
import pandas as pd

def mintosec(min_str):
    hour, minute = min_str.split(":")
    hour = int(hour.strip())
    minute = int(minute.strip())
    if hour == 0 and minute == 0:
        return 960.00
    return (hour-8)*60.0 + minute

def generate(cus, ins, base):
    vehicle_base_file = "examples/data/input_vehicle_type.csv"
    node_base_file = "examples/data/inputnode_%d.csv" % (base+1)
    edge_base_file = "examples/data/inputdistancetime_%d.csv" % (base+1)
    vehicle_df = pd.read_csv(vehicle_base_file)
    node_df = pd.read_csv(node_base_file)
    edge_df = pd.read_csv(edge_base_file)

    for i in range(ins):
        f = open('examples/%d_%d.vrpsdptw' % (cus, i+1), 'w+')
        f.write("NAME : %d_%d\n" % (cus, i+1))
        f.write("TYPE : VRPSDPTW\n")
        f.write("DIMENSION : %d\n" % (cus+1))
        f.write("VEHICLES : %d\n" % vehicle_df["max_vehicle_cnt"][1])
        f.write("DISPATCHINGCOST : %d\n" % vehicle_df["vehicle_cost"][1])
        f.write("UNITCOST : %.3f\n" % (0.001*vehicle_df["unit_trans_cost"][1]))
        f.write("CAPACITY : %.1f\n" % vehicle_df["max_weight"][1])
        f.write("EDGE_WEIGHT_TYPE : EXPLICIT\n")
        f.write("NODE_SECTION\n")
        # node,delivery,pickup,start_tm,end_tm,service_time
        node_index = 0
        f.write("%d,0,0,%d,%d,0\n" %\
                (node_index,
                 mintosec(node_df[node_df['type'] == 1]['first_receive_tm'][0]),
                 mintosec(node_df[node_df['type'] == 1]['last_receive_tm'][0])))
        L = list(range(1, node_df[node_df['type'] == 2].shape[0]+1))
        selected_1 = rd.sample(L, cus)
        selected_1 = sorted(selected_1)
        L = list(range(node_df[node_df['type'] == 2].shape[0]+1,\
                node_df[node_df['type'] == 2].shape[0]+1+node_df[node_df['type'] == 3].shape[0]))
        if len(L) > cus:
            selected_2 = rd.sample(L, cus)
        else:
            selected_2 = L
        rd.shuffle(selected_2)

        double_demand = rd.sample(list(range(len(selected_1))), len(selected_2))

        node_index += 1
        index2 = 0
        while node_index <= cus:
            if node_index-1 in double_demand:
                print(node_index, selected_1[node_index-1], selected_2[index2])
                pickup_weight = float(node_df.loc[selected_2[index2], 'pack_total_weight'])
                index2 += 1
            else:
                print(node_index, selected_1[node_index-1], '-')
                pickup_weight = 0.0
            line = "%d,%f,%f,%d,%d,30\n" %\
                    (node_index,
                     float(node_df.loc[selected_1[node_index-1], 'pack_total_weight']),
                     pickup_weight,
                     mintosec(node_df.loc[selected_1[node_index-1], 'first_receive_tm']),
                     mintosec(node_df.loc[selected_1[node_index-1], 'last_receive_tm']))
            f.write(line)
            node_index += 1

        f.write("DISTANCETIME_SECTION\n")
        dc = 0
        all_nodes = [node_df.loc[selected_1[node_index-1], 'ID'] for node_index in range(1, cus+1)]
        all_nodes.insert(0, dc)

        for index_1, from_node in enumerate(all_nodes):
            print(index_1, from_node)
            for index_2, to_node in enumerate(all_nodes):
                if index_1 == index_2:
                    continue
                dist = edge_df.loc[(edge_df["from_node"] == from_node) &\
                                   (edge_df["to_node"] == to_node), 'distance']
                time = edge_df.loc[(edge_df["from_node"] == from_node) &\
                                   (edge_df["to_node"] == to_node), 'spend_tm']
                line = '%d,%d,%d,%d\n' % (index_1, index_2, dist, time)
                f.write(line)

        f.write("DEPOT_SECTION\n%d\n" % dc) 
        f.close()

if __name__ == "__main__":
    # ./generate_benchmark 1200 5
    cus_num = int(sys.argv[1])
    ins_num = int(sys.argv[2])
    base_num = 0
    if cus_num == 200 or cus_num == 400:
        base_num = 1200
    elif cus_num == 600:
        base_num = 1300
    elif cus_num == 800:
        base_num = 1400
    elif cus_num == 1000:
        base_num = 1500
    elif cus_num == 1200:
        base_num = 1600
    rd.seed(42)
    generate(cus_num, ins_num, base_num)
