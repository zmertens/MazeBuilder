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
    - An "AI" driven demo: the app traces the grid by itself and narrates the rules
    - Stage 1 introduces the hidden blue squares and the positive points they award
    - Stage 2 introduces the hidden red squares and their penalty
    - Stage 3 explains which shader effects unlock at which score threshold
    - Hands control back to the player and transitions into PLAYING

## Scoring and Competition

The objective of the game is to touch or point and hold a blue square,
and without lifting the pointer, drag a continuous line across the grid
towards the green square.

Hidden squares are revealed by the traced line from any direction:

  * A hidden blue square awards `hidden_blue_reward` points (5 by default)
  * A hidden red square costs `hidden_red_penalty` points (2 by default)

Activating the green square generates a new blue square randomly in the grid,
so play continues without resetting the scene.

Mobile support mode is enabled through the `mobile_support` flag in
`amazing_mazes.json`. With it enabled, touch began/moved/ended events drive the
exact same press, drag and release code path as the mouse, which keeps the
gameplay portable to an Android build later on.

## Modern Graphics and Effects with Shaders

Graphical effects are unlocked by accumulating positive points. The thresholds
are configured in `amazing_mazes.json`:

  * `shader_threshold_fog` - radial fog "fog of war" spotlight
  * `shader_threshold_bloom` - bloom threshold plus blur glow pass
  * `shader_threshold_parallax` - animated parallax background

Chunk-based rendering system with continuous level generation via Maze Builder.

Particles to give a central focal point to the player's current pointer, 
and represent movement between start/stop positions.

Transitions thru gameplay with grayscale on scene creation, as a means of hidding the best path out, and the pointer movement has the effect of interpolating into shaders like pixelation and wave.
