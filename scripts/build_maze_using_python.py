import random

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
        return self._grid[random_row][random_col]
    
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

if __name__ == '__main__':
    
    CELL_SIZE = 4
    grid = Grid(10, 10)
    BinaryTree.on(grid)
    
    print(str(grid))
    
