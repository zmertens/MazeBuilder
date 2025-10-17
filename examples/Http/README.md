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

1. **maze_client** - HTTP client for Corners maze building server
   ```bash
   maze_client --help
   maze_client --create -r 10 -c 10 -s 42 -a dfs -d [0:1]
   ```

2. **ls** - List available programs
   ```bash
   ls
   ```
   Output: `find  maze_client ls  help  exit`

3. **find** - Find programs matching pattern
   ```bash
   find maze
   ```
   Output: `maze_client`

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
builder123@mazes:~/http$ maze_client --create -r 15 -c 20 -s 123 -a binary_tree
HTTP Response Status: 201 (Created)
Response Body:
{"data":"base64_encoded_maze_data","createdAt":"2024-01-01T12:00:00Z","version_str":"v7.2.6"}
```


## Communication Protocols

### HTTP Communication
The client communicates with the Corners server using HTTP REST API:
- **POST /api/mazes/create** - Create a new maze

All HTTP responses are displayed as JSON in the terminal.

## Error Handling

### HTTP Errors
If there are HTTP connection issues or server errors, appropriate messages are displayed:
```bash
builder123@mazes:~/http$ maze_client --create -r 10 -c 10
HTTP Response Status: 500 (Internal Server Error)
Response Body:
{"error": "Database connection failed"}
```

## Getting Help

- Use `maze_client --help` for HTTP maze builder specific help
- Use `help` for general terminal commands
- Use `./mazebuilderhttp --help` (before starting) for usage information
