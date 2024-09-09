'''
Working through different maze algorithms from the book "Mazes for Programmers" by Jamis Buck
https://www.amazon.com/Mazes-Programmers-Twisty-Little-Passages/dp/1680500554?ref_=ast_sto_dp

Outputs to terminal or writes a PNG file representing the maze
'''

from PIL import Image, ImageDraw
import random

class Distances:

    def __init__(self, root):
        self.root = root
        self.cells = {}
        # Distance from root to itself is 0
        self.cells[self.root] = 0

    def __index__(self, cell):
        return self.cells[cell]
    
    def __index__(self, cell, distance):
        self.cells[cell] = distance

    def get_cells(self):
        return self.cells.keys()

class Cell:
    
    def __init__(self, row, column):
        self.north = None
        self.south = None
        self.east = None
        self.west = None
        self.row = row
        self.column = column
        self._links = {}
        
    def link(self, cell, bidi=True):
        self._links[cell] = True
        if bidi == True:
            cell.link(self, False)
    
    def unlink(self, cell, bidi=True):
        del self._links[cell]
        if bidi == True:
            cell.unlink(self, False)
    
    def links(self):
        self._links
        
    def is_linked(self, cell):
        if cell in self._links:
            return True
        else:
            return False
    
    def get_neighbors(self):
        neighbors = []
        if self.north:
            neighbors.append(self.north)
        if self.south:
            neighbors.append(self.south)
        if self.east:
            neighbors.append(self.east)
        if self.west:
            neighbors.append(self.west)
    
    def distances(self):
        d = Distances(self)
        frontier = [self]
        while frontier:
            new_frontier = []
            for cell in frontier:
                for linked in cell.links():
                    if d[linked]:
                        d[linked] = d[cell] + 1
                        new_frontier.append(linked)
            frontier = new_frontier
        return d
            
class Grid:
    
    def __init__(self, rows, columns):
        self._rows = rows
        self._columns = columns
        self.prepare_grid()
        
    def prepare_grid(self):
        self._grid = []
        for row in range(self._rows):
            self._grid.append([])
            for col in range(self._columns):
                self._grid[row].append(Cell(row, col))
        self.configure_cells()
        
    # Configure 2-dimensional grid starting with the top-leftmost cell,
    #   and count left-right and then downward (similar to an SQL table!)
    def configure_cells(self):
        for cell in self.each_cell():
            row, col = cell.row, cell.column
            cell.north = self.at(row - 1, col)
            cell.south = self.at(row + 1, col)
            cell.west = self.at(row, col - 1)
            cell.east = self.at(row, col + 1)

    def at(self, row, column):
        if row < 0 or row >= self._rows:
            return None
        elif column < 0 or column >= len(self._grid[row]):
            return None
        return self._grid[row][column]
    
    def random_cell(self):
        random_row = random.randint(0, self._rows)
        random_col = random.randint(0, len(self._grid[random_row - 1]))
        return random.choice(self._grid)
    
    def size(self):
        return self._rows * self._columns
    
    def each_row(self):
        for row in range(len(self._grid)):
            yield self._grid[row]
    
    def each_cell(self):
        for grid_row in self.each_row():
            for cell in grid_row:
                if cell != None:
                    yield cell
                
    def __str__(self):
        output = "+" + "---+" * self._columns + "\n"
        for row in self.each_row():
            # Top and Bottom of Cell Wall
            top = "|"
            bottom = "+"
            for cell in row:
                if cell is None:
                    cell = Cell(-1, -1)
            
                body = "   "
                if cell.is_linked(cell.east) is True:
                    east_boundary = " "
                else:
                    east_boundary = "|"
                top += body + east_boundary
                
                if cell.is_linked(cell.south) is True:
                    south_boundary = "   "
                else:
                    south_boundary = "---"
           
                corner = "+"
                bottom += south_boundary + corner

            output += top + "\n"
            output += bottom + "\n"
        return output

class BinaryTree:
    
    # Count left-right, bottom-top
    @staticmethod
    def on(grid):
        for cell in grid.each_cell():
            neighbors = []
            if cell.north is not None:
                neighbors.append(cell.north)
            if cell.east is not None:
                neighbors.append(cell.east)
            
            if len(neighbors) != 0:
                random_index = random.randrange(len(neighbors))
                neighbor = neighbors[random_index]
                if neighbor is not None:
                    cell.link(neighbor)

# class DFS:
#     @staticmethod
#     def on(grid):
#         for cell in grid.each_cell():
#             maze_stack = [cell]
#             while maze_stack:
#                 current = maze_stack.pop()
#                 # Find all unvisited neighbors of current cell
#                 neighbors = [neighbor for neighbor in current if not neighbor.links()]
#                 if neighbors:
#                     # Push unvisited cells with respcet to current
#                     maze_stack.append(current)
#                     neighbor = random.choice(neighbors)
#                     current.link(neighbor)
#                     maze_stack.append(neighbor)

'''
Utility function to draw the maze using PIL
'''
def draw_maze(grid, cell_size=10):
    img_width = grid._columns * cell_size
    img_height = grid._rows * cell_size
    background = (255, 255, 255)
    wall = (0, 0, 0)

    img = Image.new("RGB", (img_width + 1, img_height + 1), background)
    draw = ImageDraw.Draw(img)

    for cell in grid.each_cell():
        x1 = cell.column * cell_size
        y1 = cell.row * cell_size
        x2 = (cell.column + 1) * cell_size
        y2 = (cell.row + 1) * cell_size

        if cell.north is None:
            draw.line((x1, y1, x2, y1), fill=wall)
        if cell.west is None:
            draw.line((x1, y1, x1, y2), fill=wall)
        if not cell.is_linked(cell.east):
            draw.line((x2, y1, x2, y2), fill=wall)
        if not cell.is_linked(cell.south):
            draw.line((x1, y2, x2, y2), fill=wall)

    return img

if __name__ == '__main__':
    
    CELL_SIZE = 4
    grid = Grid(10, 10)
    DFS.on(grid)
    
    # print(str(grid))
    img = draw_maze(grid, cell_size=CELL_SIZE)
    img.save("dfs.png")
    
