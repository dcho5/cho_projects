# search.py
# ---------------
# Licensing Information:  You are free to use or extend this projects for
# educational purposes provided that (1) you do not distribute or publish
# solutions, (2) you retain this notice, and (3) you provide clear
# attribution to the University of Illinois at Urbana-Champaign
#
# Created by Kelvin Ma (kelvinm2@illinois.edu) on 01/24/2021
from collections import deque

"""
This is the main entry point for MP1. You should only modify code
within this file -- the unrevised staff files will be used for all other
files and classes when code is run, so be careful to not modify anything else.
"""
# Search should return the path.
# The path should be a list of tuples in the form (row, col) that correspond
# to the positions of the path taken by your search algorithm.
# maze is a Maze object based on the maze from the file specified by input filename
# searchMethod is the search method specified by --method flag (bfs,dfs,astar,astar_multi,fast)


def bfs(maze):
    """
    Runs BFS for part 1 of the assignment.

    @param maze: The maze to execute the search on.

    @return path: a list of tuples containing the coordinates of each state in the computed path
    """

    # initialize node variables
    distance = 0
    parent = None
    node = (maze.start, parent)   # tuple(state, parent ID)

    # initialize queues
    explored = deque()
    explored.append(node)
    exp_cells = deque()
    exp_cells.append(maze.start)
    frontier = deque()
    frontier.append(node)

    while frontier:
        node = frontier.popleft()
        parent = node

        if node[0] == maze.waypoints[0]:
            break

        for neighbor in maze.neighbors(node[0][0], node[0][1]):
            if neighbor not in exp_cells:
                node = (neighbor, parent)
                frontier.append(node)
                explored.append(node)
                exp_cells.append(neighbor)

    path = deque()

    for item in explored:
        if item[0] == maze.waypoints[0]:
            node = item

    while node[1] != None:
        path.append(node[0])
        node = node[1]

    path.reverse()
    path.appendleft(maze.start)

    return path

def astar_single(maze):
    """
    Runs A star for part 2 of the assignment.

    @param maze: The maze to execute the search on.

    @return path: a list of tuples containing the coordinates of each state in the computed path
    """

    curr = maze.start
    path = []
    frontier = [maze.start]

    # {cell : parent cell}
    parent = {}

    g_n = {maze.start : 0}
    gh_n = {maze.start : 0 + manhattan(maze.start, maze.waypoints[0])}

    while frontier:
        ghn_min = float('inf')
        for cell in frontier:
            if gh_n.get(cell) < ghn_min:
                curr = cell
                ghn_min = gh_n.get(cell)

        if curr == maze.waypoints[0]:
            path = [curr]
            while curr in parent.keys():
                curr = parent.get(curr)
                path.insert(0, curr)

            return path

        frontier.remove(curr)

        for neighbor in maze.neighbors(curr[0], curr[1]):
            g_temp = g_n.get(curr) + 1
            if g_n.get(neighbor) is not None and g_temp < g_n.get(neighbor):
                parent[neighbor] = curr
                g_n[neighbor] = g_temp
                gh_n[neighbor] = g_n.get(neighbor) + manhattan(neighbor, maze.waypoints[0])
                if neighbor not in frontier:
                    frontier.append(neighbor)

            if g_n.get(neighbor) is None and g_temp < float('inf'):
                parent[neighbor] = curr
                g_n[neighbor] = g_temp
                gh_n[neighbor] = g_n.get(neighbor) + manhattan(neighbor, maze.waypoints[0])
                if neighbor not in frontier:
                    frontier.append(neighbor)

    return path

def astar_corner(maze):
    """
    Runs A star for part 3 of the assignment in the case where there are four corner objectives.

    @param maze: The maze to execute the search on.

    @return path: a list of tuples containing the coordinates of each state in the computed path
        """
    return astar_multiple(maze)

def astar_multiple(maze):
    """
    Runs A star for part 4 of the assignment in the case where there are
    multiple objectives.

    @param maze: The maze to execute the search on.

    @return path: a list of tuples containing the coordinates of each state in the computed path
    """

    path = []
    wp = listify(maze.waypoints)
    wp.sort()
    # {cell : remaining waypoints' MST length}
    mst_lens = {}
    remain = {maze.start : wp}

    g_n = {(maze.start, tuple(wp)) : 0}
    frontier = []
    frontier.append((0, (maze.start, None, 0, wp)))

    while frontier:
        node = frontier.pop(0)
        curr = node[1][0]
        waypt = node[1][3]
        g = node[1][2]

        if curr in waypt:
            waypt.remove(curr)

        if not waypt:
            while node is not None:
                path.append(node[1][0])
                node = node[1][1]
            break

        for neighbor in maze.neighbors(curr[0], curr[1]):
            g_temp = g + 1
            if (neighbor, tuple(waypt)) not in g_n:
                g_n[(neighbor, tuple(waypt))] = float('inf')

            #if (g_n.get(neighbor) is None) or (g_n.get(neighbor) is not None and g_temp < g_n[(curr, tuple(waypt))]:
            if g_temp < g_n[(neighbor, tuple(waypt))]:
                g_n[(neighbor, tuple(waypt))] = g_temp
                wp_copy = waypt.copy()

                if tuple(waypt) not in mst_lens:
                    mst_lens[tuple(waypt)] = kruskals(waypt)

                mst = mst_lens[tuple(waypt)]

                h_n = mst + manhattan(neighbor, nearest_wp(neighbor, wp_copy))
                frontier.append((g_temp + h_n, (neighbor, node, g_temp, wp_copy)))
                frontier.sort()

    path.reverse()
    return path

def kruskals(wp):

    waypoints = wp.copy()
    edges = {}
    mst = []
    mst_len = 0

    wp_temp = waypoints

    # find all the edges
    for fro in waypoints:
        wp_temp.remove(fro)
        for to in wp_temp:
            edges[manhattan(fro, to)] = (fro, to)

    sorted(edges.values())

    # Kruskal's algorithm
    for key in edges:
        if edges[key][0] not in mst or edges[key][1] not in mst:
            if edges[key][0] not in mst:
                mst.append(edges[key][0])
            if edges[key][1] not in mst:
                mst.append(edges[key][1])
            mst_len += key

    if mst_len > 100:
        mst_len /= 1.5

    return mst_len

def manhattan(fro, to):

    return abs(fro[0]-to[0]) + abs(fro[1]-to[1])

def nearest_wp(fro, waypoints):

    nearest = ()
    shortest = float('inf')

    for to in waypoints:
        temp = manhattan(fro, to)
        if temp < shortest:
            shortest = temp
            nearest = to

    return nearest

def listify(tuple):

    list = []
    for item in tuple:
        list.append(item)

    return list

def fast(maze):
    """
    Runs suboptimal search algorithm for part 5.

    @param maze: The maze to execute the search on.

    @return path: a list of tuples containing the coordinates of each state in the computed path
    """
    return []
