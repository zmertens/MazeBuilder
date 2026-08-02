# MazeBuilder HTTP Example

A single executable (`mazebuilderhttp`) that runs as either a **local HTTP server**
or an **SFML visualisation client**.  The server generates mazes on demand via a
RESTful-style GET endpoint; the client fetches a maze from the server and renders
it in an interactive SFML window.

---

## Quick-start (two terminals)

```bash
# Terminal 1 � start the server
./mazebuilderhttp --server

# Terminal 2 � open the SFML client
./mazebuilderhttp --client
```

### Server mode

```bash
mazebuilderhttp --server              # listens on localhost:8080
mazebuilderhttp --server --port 9090  # custom port
```

### Client mode

`mazebuilderhttp --client [--host <host>] [--port <N>]`

`mazebuilderhttp --client --host 192.168.1.5 --port 9090`

---

## Server endpoint

| Method | Path     | Description                          |
|--------|----------|--------------------------------------|
| GET    | `/mazes` | Generate and return a maze as text   |

### Query parameters

| Parameter | Type   | Default       | Description                              |
|-----------|--------|---------------|------------------------------------------|
| `rows`    | uint   | `10`          | Number of rows (clamped to 1:100)        |
| `columns` | uint   | `10`          | Number of columns (clamped to 1:100)     |
| `algo`    | string | `binary_tree` | Algorithm: `binary_tree` `sidewinder` `dfs` |

### Example requests

```bash
# Default 10x10 binary_tree maze
curl http://localhost:8080/mazes

# 15-row x 12-column DFS maze
curl "http://localhost:8080/mazes?rows=15&columns=12&algo=dfs"

# Sidewinder with non-square grid
curl "http://localhost:8080/mazes?rows=8&columns=20&algo=sidewinder"
```

### Example response

```
rows=10 columns=10 algo=dfs
+--+--+--+--+--+--+--+--+--+--+
|                               |
+  +--+--+--+--+--+--+--+--+  +
|  |                          |  |
...
```

The first line is a metadata header (`rows=N columns=M algo=X`).
All subsequent lines are the ASCII maze.

---

## Client controls

| Key        | Action                                      |
|------------|---------------------------------------------|
| G / Enter  | Fetch a new maze from the server            |
| Tab        | Cycle algorithm (binary_tree ? sidewinder ? dfs) |
| + / =      | Grow maze by 2 rows and columns             |
| - / _      | Shrink maze by 2 rows and columns           |
| Q / Esc    | Quit                                        |

The window title shows the current algorithm and server address.
The status bar shows the metadata line returned by the last request.

---
