# MazeBuilder HTTP Terminal Interface

This is a terminal-based HTTP client for interacting with the Corners maze building server. It provides a UNIX-style terminal interface with various commands for creating, retrieving, and managing mazes.

## Building

The HTTP example is built with CMake using the configuration `-DMAZE_BUILDER_EXAMPLES=1` as part of the MazeBuilder project. The executable is named `mazebuilderhttp`.

## Usage

### Starting the Terminal

```bash
./mazebuilderhttp <server_url>
```

**Arguments:**
- `server_url`: URL of the Corners server
  - For development: `http://localhost:3000`
  - For production: `http://corners-app-9d22c3fdfd0c.herokuapp.com/`

**Examples:**
```bash
./mazebuilderhttp http://localhost:3000
./mazebuilderhttp http://corners-app-9d22c3fdfd0c.herokuapp.com/
```

### Terminal Commands

Once the terminal is running, you'll see a prompt like:
```
builder123@mazes:~/http$
```

#### Available Commands

1. **mazebuilderhttp** - HTTP client for Corners maze building server
   ```bash
   mazebuilderhttp --help
   mazebuilderhttp --create -r 10 -c 10 -s 42 -a dfs
   ```

2. **ls** - List available programs
   ```bash
   ls
   ```
   Output: `find  mazebuilderhttp  ls  help  exit`

3. **find** - Find programs matching pattern
   ```bash
   find maze
   ```
   Output: `mazebuilderhttp`

4. **help** - Show terminal help
   ```bash
   help
   ```

5. **exit** - Exit the terminal
   ```bash
   exit
   ```

#### MazeBuilder HTTP Options

- `-r, --rows <number>`: Number of rows (default: 10)
- `-c, --columns <number>`: Number of columns (default: 10)  
- `-s, --seed <number>`: Random seed (default: 42)
- `-a, --algorithm <name>`: Algorithm to use (default: dfs)
  - Available algorithms: `dfs`, `binary_tree`, `sidewinder`

## Examples

### Creating a Maze
```bash
builder123@mazes:~/http$ mazebuilderhttp --create -r 15 -c 20 -s 123 -a binary_tree
HTTP Response Status: 201 (Created)
Response Body:
{"id": 1, "rows": 15, "columns": 20, "seed": 123, "algorithm": "binary_tree", "created_at": "2024-01-01T12:00:00Z"}
```

## Server Communication

The client communicates with the Corners server using HTTP REST API:

- **POST /api/mazes/create - Create a new maze

All responses are displayed as JSON in the terminal.

## Error Handling

If there are connection issues or server errors, the client will display appropriate error messages:

```bash
builder123@mazes:~/http$ mazebuilderhttp --create -r 10 -c 10
HTTP Response Status: 500 (Internal Server Error)
Response Body:
{"error": "Database connection failed"}
```

## Getting Help

- Use `mazebuilderhttp --help` for maze builder specific help
- Use `help` for general terminal commands
- Use `./mazebuilderhttp --help` (before starting) for usage information
