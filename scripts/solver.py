'''
Working through different maze algorithms from the book "Mazes for Programmers" by Jamis Buck
https://www.amazon.com/Mazes-Programmers-Twisty-Little-Passages/dp/1680500554?ref_=ast_sto_dp

This script analyzes pre-existing maze images for 'solveability'

pip install pillow numpy networkx
'''

from PIL import Image
import numpy as np
import networkx as nx
import glob


def read_image(file_path):
    image = Image.open(file_path)
    image = image.convert('L')
    return np.array(image)

def preprocess_image(image_array):
    threshold = 128
    binary_image = (image_array > threshold).astype(np.uint8)    
    return binary_image

def create_graph(binary_image):
    rows, cols = binary_image.shape
    graph = nx.Graph()
    for r in range(rows):
        for c in range(cols):
            # Found a path
            if binary_image[r, c] == 1:
                graph.add_node((r, c))
                if r > 0 and binary_image[r - 1, c] == 1:
                    graph.add_edge((r, c), (r - 1, c))
                if c > 0 and binary_image[r, c - 1] == 1:
                    graph.add_edge((r, c), (r, c - 1))
    return graph

def solve_maze(graph, start, end):
    try:
        path = nx.shortest_path(graph, source=start, target=end)
        return path
    except nx.NetworkXNoPath:
        return None

if __name__ == '__main__':
    png_images = glob.glob('*.png')
    for png in png_images:
        image_array = read_image(png)
        binary_image = preprocess_image(image_array)
        graph = create_graph(binary_image)

        # Look for solveability
        start = (1, 1)
        end = (binary_image.shape[0] - 2, binary_image.shape[1] - 2)
        path = solve_maze(graph, start, end)

        if path:
            print(f'Path found in file: {png}')
        else:
            print(f'No path found in file: {png}')

