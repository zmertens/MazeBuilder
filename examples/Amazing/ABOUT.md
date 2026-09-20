# Amazing

An example that explores procedurally generated levels via Maze Builder.
This "amazing" app is an arcade-like incremental numbers game with modern tooling (box2d, c++20, SFML).
It uses Maze Builder as a utility library for procedurally generating levels and scoring.

Modern graphics shaders are used to give particle and bloom effects.

Other features include audio and sound effects.

@TODO: Network additions

## Into the Features and Gameplay

The game follows a simple state pattern:
  * MENU
    - Accepts user input and can start/close the game
    - Provides options for game configuration
    - High scores and rules
    - Provides an async class with "just-in-time" resource loading
  * PLAYING
    - Interactive and continuous level exploration
    - Provides scoring and rules are applied to all players
    - Endless vertical and horizontal side-scrolling
  * TRANSITION
    - Provides ambience and pause
    - Resets and restarts the game
    - Signifies configuration events to select a level
  * TUTORIAL
    - Provides guided messaging with respect to gameplay progression
    - Disables scoring system, persistence of scores

## Scoring and Competition

Gameplay happens by pointing at a cell on screen,
and clicking the mouse, joystick button, or finger down on that point,
and swiping to another cell.
Once the pointer is released then that swiped or drawn portion of cells is rendered
in color and revealed.

Numbers are added/subtracted by swiping parts of a randomly-select path.
Natural goal-orientation towards positive (green) numbers
gives the appearance of progress. Subtracting numbers (red)
will mean slower progress; there isn't a notion of "negative progress".

Receiving a configurable number less than zero puts the game in a tutorial mode.
The player can choose to skip and receive a lump sum of positive numbers, or play tutorial mode.

Tutorial mode will, by default, clearly shown and mark the randomly-generated path,
and instruct the user where to swipe. The tutorial will show and explain briefly positive and negative numbers.

Once the player feels comfortable with the basic mechanics they can move back into PLAYING mode.

## Modern Graphics and Effects with Shaders

Chunk-based rendering system with continuous level generation via Maze Builder.

Particles to give a central focal point to the player's current pointer, 
and represent movement between start/stop positions.

Bloom filter for ambience, and blur filter as a "fog of war" effect.

Transitions thru gameplay with grayscale on scene creation, as a means of hidding the best path out, and the pointer movement has the effect of interpolating into shaders like pixelation and wave.
