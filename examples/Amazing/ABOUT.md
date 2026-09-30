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
    - An "AI" driven demo: the app traces the grid by itself and narrates the rules, looping
      continuously at a gentle pace for as long as nobody steps in
    - Stage 1 introduces the hidden blue squares and the positive points they award
    - Stage 2 introduces the hidden red squares and their penalty
    - Stage 3 explains which shader effects unlock at which score threshold
    - The moment the player touches or clicks the board, the demo stops and endless mode begins

## Scoring and Competition

The objective of the game is to touch or point and hold a blue square,
and without lifting the pointer, drag a continuous line across the grid
towards the green square.

Hidden squares are revealed by the traced line from any direction:

  * A hidden blue square awards `hidden_blue_reward` points (5 by default)
  * A hidden red square costs `hidden_red_penalty` points (2 by default)

The real objective, though, is collecting **yellow squares**. Reaching the
green square turns it yellow, awards 10 points, and immediately spawns a new
green square elsewhere in the grid so play never resets. The player "travels"
across the board as yellow squares accumulate, chasing green squares one
after another for continuous, rapid gameplay.

### Expanding the Level

`apply_interval_per_points` (in `amazing_mazes.json`) controls how many net
points need to accumulate before the board expands. The magnitude is what
matters, so the value may be written as positive or negative in the config.
With the default of 50, roughly five yellow squares (10 points each) trigger
an expansion. Each expansion:

  * Grows the grid by a couple of rows and columns - new blocks to color and
    draw paths towards green squares
  * Keeps every existing square (blue, yellow, hidden blue/red) exactly where
    it was; only the grid grows around it
  * Seeds a fresh set of hidden blue/red squares and a new hidden path into
    the newly opened space

The board never zooms or rescales to fit the bigger grid - pinch-zoom is still
a manual, explicit gesture. Instead, the camera pans to reveal the new space,
the same way it does for ordinary panning (see below).

Mobile support mode is enabled through the `mobile_support` flag in
`amazing_mazes.json`. With it enabled, touch began/moved/ended events drive the
exact same press, drag and release code path as the mouse, which keeps the
gameplay portable to an Android build later on.

## Moving the Camera

The board is drawn into an off-screen render texture, and an `sf::View` is
applied to that texture only. The window itself keeps a 1:1 pixel view, so the
post-process sprite, the fog spotlight and the HUD stay screen aligned no
matter how the board is panned, zoomed or rotated.

  * **Zoom** - mouse wheel, or a two-finger pinch. Zoom is anchored at the
    cursor or the finger midpoint, so the square under the pointer stays put.
  * **Rotate** - `Q` / `E`, `Ctrl` + wheel, or a two-finger twist. The twist
    gesture uses a dead zone so an ordinary pinch does not leave the board
    crooked.
  * **Pan** - arrow keys, a right- or middle-button drag, or two-finger
    translation.
  * **Reset** - `R` returns the camera to centred, unzoomed and unrotated.

During endless play, the board also pans itself: whenever the pointer or an
active touch sits near a window edge, the camera keeps sliding that direction,
like an infinite side-scroller, using the exact same pan routine as a manual
drag. This is automatic and continuous - no swipe gesture is required - and it
stacks with, rather than replaces, the explicit pan controls above.

When a second finger lands, any in-progress trace is cancelled without scoring
and the gesture takes over. Drawing does not resume until every finger lifts,
so a released pinch cannot turn into a stray scoring line.

Camera limits are configured in `amazing_mazes.json`:

  * `zoom_min` / `zoom_max` - cumulative zoom clamp
  * `zoom_wheel_step` - zoom factor applied per wheel notch
  * `rotation_enabled` - master switch for all rotation input
  * `rotation_dead_zone_degrees` - twist threshold before rotation latches

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
