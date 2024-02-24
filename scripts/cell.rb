# Represents a cell in a maze

require 'distances'

class Cell
    attr_reader :row, :column
    attr_accessor :north, :south, :east, :west

    def initialize(row, column)
        @row, @column = row, column
        @links = {}
    end

    # Helper methods for linking and unlinking cells in mazes
    def link(cell, bidi=true)
        @links[cell] = true
        cell.link(self, false) if bidi
        self
    end

    def unlink(cell, bidi=true)
        @links.delete(cell)
        cell.unlink(self, false) if bidi
        self
    end

    def links
        @links.keys
    end

    def linked?(cell)
        @links.key?(cell)
    end

    def neighbors
        list = []
        list << north if north
        list << south if south
        list << east if east
        list << west if west
        list
    end

    # def to_s
    #     "row: #{@row} , column: #{@column}"
    # end

    # Compute distances between cell starting points
    # using Dijkstra's algo
    def distances
        distances = Distances.new(self)
        frontier = [self]
        while frontier.any?
            new_frontier = []
            frontier.each do |cell|
                cell.links.each do |linked|
                    next if distances[linked]
                    distances[linked] = distances[cell] + 1
                    new_frontier << linked
                end
            end
            frontier = new_frontier
        end
        distances
    end
end
