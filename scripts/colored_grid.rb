
require 'grid'
require 'chunky_png'

class ColoredGrid < Grid
    def distances=(distances)
        @distances = distances
        farthest, @maximum = distances.max
    end

    def background_color_for(cell)
        if !@distances[cell]
            return nil
        end
        distance = @distances[cell]
        # or return nil?
        intensity = (@maximum - distance).to_f / @maximum
        dark = (255 * intensity).round
        bright = 128 + (127 * intensity).round
        ChunkyPNG::Color::rgb(dark, bright, dark)
    end
end
