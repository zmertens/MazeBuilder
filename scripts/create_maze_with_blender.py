import bpy
import bmesh
from bpy_extras.object_utils import AddObjectHelper

import random

class Cell:
    
    def __init__(self, row, column):
        self._row = row
        self._column = column
        self._links = {}
        
    def link(self, cell, bidi=True):
        pass
    
    def unlink(self, cell, bidi=True):
        pass
    
    def links(self):
        self._links
        
    def is_linked(self, cell):
        return self._links[cell]
    
    def get_neighbors(self):
        neighbors = []
        if self._north:
            neighbors.append(self._north)
        elif self._south:
            neighbors.append(self._south)
        elif self._east:
            neighbors.append(self._east)
        elif self._west:
            neighbors.append(self._west)
            
            
class Grid:
    
    def __init__(self, rows, columns):
        self._rows = rows
        self._columns = columns
        self.prepare_grid()
        
    def prepare_grid(self):
        self._grid = []
        for row in range(0, len(self._rows)):
            self._grid.append([])
            for col in range(0, len(self._columns)):
                self._grid[row].append(Cell(row, col))
        self.configure_cells()
        
    def configure_cells(self):
        pass
        
    def __index__(self, row, column):
        if row < 0 or row >= self._rows:
            return None
        elif column < 0 or column >= len(self._grid[row]):
            return None
        return self._grid[row][column]
    
    def random_cell(self):
        random_row = random.randint(0, self._rows)
        random_col = random.randint(0, len(self._grid[random_row]))
        return self._grid[random_row][random_col]
    
    def size(self):
        return self._rows * self._columns
    
    def each_row(self):
        for row in xrange(len(self._grid)):
            yield row
    
    def each_cell(self):
        for cell_list in self.each_row:
            for cell in cell_list:
                if cell:
                    yield cell   

class BinaryTree:
    
    # Count left-right, bottom-top
    def on(self, grid):
        for cell in grid.each_cell:
            neighbors = []
            if cell._north != None:
                neighbors.append(cell._north)
            elif cell._east != None:
                neighbors.append(cell._east)
            random_index = random.randint(0, len(neighbors))
            neighbor = neighbors[random_index]
            if neighbor != None:
                cell.link(neighbor)


if __name__ == '__main__':
    
    CELL_SIZE = 4
    
    grid = Grid(20, 20)
    
    BinaryTree.on(grid)
    
    for cell in grid.each_cell:
        bpy.ops.mesh.primitive_plane_add(size=CELL_SIZE, enter_editmode=False, align='WORLD', location=(0, 0, 0))
        bpy.ops.transform.translate(value=(0, 2.66474, 0), orient_type='GLOBAL', orient_matrix=((1, 0, 0), (0, 1, 0), (0, 0, 1)), orient_matrix_type='GLOBAL', constraint_axis=(False, True, False), mirror=True, use_proportional_edit=False, proportional_edit_falloff='SMOOTH', proportional_size=1, use_proportional_connected=False, use_proportional_projected=False)

    