# C & C++ Http movie client

**Copyright © Springer Robert Stefan, 2025**

This project is an application that interacts with a REST server using
command-line input. The client connects via sockets, sends HTTP requests, and
displays responses received from the server.

## Dependencies

- g++
- nlohmann/json library (included as a single header)
- Linux (uses POSIX socket API)

## Project Structure

- `client.cpp` contains the main logic and command interpreter for stdin input
- `helpers.c` includes functions for connection, sending, and receiving messages
- `requests.c` manually builds GET and POST HTTP requests
- `buffer.c` manages a dynamic buffer for large server responses
- `json.hpp` is an external library used for JSON parsing

## Functionality

The client accepts the following commands:

- `login_admin` - authenticate as admin
- `logout_admin` - logout as admin
- `add_user` - add a normal user
- `get_users` - display all users
- `delete_user` - delete a user

- `login` - authenticate as a normal user
- `logout` - logout normal user
- `get_access` - obtain access token

- `add_movie` - add a movie
- `get_movies` - list all movies
- `get_movie` - display movie details
- `update_movie` - update a movie
- `delete_movie` - delete a movie

- `add_collection` - add a movie collection
- `get_collections` - list all collections
- `get_collection` - display collection details
- `delete_collection` - delete a collection
- `add_movie_to_collection` - add movie to collection
- `delete_movie_from_collection` - remove movie from collection

- `exit` - exit the application

## Application Flow

1. The user types a command (e.g., `login`, `add_movie`, etc.)
2. Required fields are read from stdin (e.g., username, password, title)
3. The HTTP payload is built (if needed) using `nlohmann::json`
4. The HTTP request is sent over a TCP socket
5. The response is received and processed:
   - if it's JSON, it is parsed using `nlohmann::json`
   - relevant fields are extracted and printed

## Compilation Instructions

```bash
g++ client.cpp buffer.c helpers.c -o client -std=c++1
