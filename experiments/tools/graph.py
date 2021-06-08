import csv
import graph_tool.all as gt


file="./graph.csv"


layoutMap={1: (0.375, 0.55),
           2: (0.30, 0.6),
           3: (0.30, 0.55),
           #5: (0.2, 0.7), #(0.7, 0.55)
           5: (0.7, 0.55),
           6: (0.375, 0.7),
           7: (0.55, 0.57),
           8: (0.52, 0.67),
           9: (0.57, 0.67),
           10: (0.65, 0.67),
           11: (0.65, 0.75),
           12: (0.7, 0.77),
           13: (0.7, 0.875),
           14: (0.82, 0.9),
           15: (0.9, 0.875),
           16: (0.9, 0.375),
           17: (0.82, 0.4),
           18: (0.7, 0.38),
           19: (0.7, 0.2)}

g = gt.Graph(directed=False)
coords = g.new_vertex_property("vector<double>")
name = g.new_vertex_property("string")

vertexMap={}

for k,v in layoutMap.items():
    vertex = g.add_vertex()
    vertexMap["raspi-{:02d}".format(k)] = vertex
    coords[vertex] = (v[0]*100,(1-v[1])*100)
    name[vertex] = str(k)#"raspi-{:02d}".format(k)

maxLq1 = g.new_edge_property("float")
lastLq1 = g.new_edge_property("float")
maxLq2 = g.new_edge_property("float")
lastLq2 = g.new_edge_property("float")
becameBi = g.new_edge_property("bool")
lostBi = g.new_edge_property("bool")
lost = g.new_edge_property("bool")


with open(file, 'r') as f:
    reader = csv.DictReader(f, delimiter=";")
    for l in reader:
        n1 = l['node1']
        n2 = l['node2']
        e = g.add_edge(vertexMap[n1], vertexMap[n2])
        print(l)
        maxLq1[e] = float(l['max lq1'])
        lastLq1[e] = float(l['last lq1'])
        maxLq2[e] = float(l['max lq2'])
        lastLq2[e] = float(l['last lq2'])
        becameBi[e] = l['became bi'] == "True"
        lostBi[e] = l['lost bi'] == "True"
        lost[e] = l['lost'] == "True"


edge_color=g.new_edge_property("string")
for e in g.edges():
    if becameBi[e] and (not lostBi[e] and not lost[e]):
        edge_color[e] = "green"
    elif becameBi[e] and (lostBi[e] and not lost[e]):
        edge_color[e] = "yellow"
    elif becameBi[e] and lostBi[e] and lost[e]:
        edge_color[e] = "orange"
    else:
        edge_color[e] = "red"

vertex_color=g.new_vertex_property("string")
for v in g.vertices():
    vertex_color[v] = "red"
    for e in v.all_edges():
        if edge_color[e] == "green":
            vertex_color[v] = "black"
            break


view = gt.GraphView(g, vfilt=lambda v: name[v] != "14" and name[v] != "13"
                                       and name[v] != "8" and name[v] != "10" and name[v] != "12"
                    and name[v] != "6" and name[v] != "16" and name[v] != "3" and name[v] != "11")
gt.graph_draw(g, vertex_text=name, vertex_color=vertex_color, edge_color=edge_color, pos=coords, output_size=(1000,1000)) # , output="andreData/graph.png"
#gt.graph_draw(view, vertex_text=name, vertex_color=vertex_color, edge_color=edge_color, pos=coords, output_size=(1000,1000))

#13, 14
#8, 10, 12
#6, 16, 3, 11






