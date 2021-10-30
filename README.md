# AH_Computing_Science_Project

My Advanced Higher Computer Science Project written in C++.

It is a simple multiplayer 2D game based on a mod for Dota 2. It uses Direct2D for graphics and Winsock for networking. It uses SQLite to keep a database of user accounts and previous games.

It features 3 separate programs:
* The Game Client is a client application for a user to play the game.
* The Game Server is a server application which runs the game logic. During a game players send their input along a TCP connection and the server sends the game state back over UDP.
* The Game Server is a server application. One copy of the Game Coordinator is run and it has the database of accounts and previous games. Game Clients and Game Servers connect to the Game Coordinator when they launch. Users must log in through the Client App and then can request to be given a game. The Game Coordinator will match users searching for games in pairs then send choose for them a Game Server.

I wrote this while I was still in secondary school (as mentioned for my advanced higher computer science qualification) and so there are mistakes I made in producing this project that I would not make in a new project. It also lacks features that I would have implemented if I continued to work on this.
* I did not plan much of the code before writing it so the code is quite messy in some places.
* The code should have been split more between classes and there are parts 
* The code is not robust, it deals poorly with connections being lost at inopportune times.
* Most of the networking I figured out myself, only reading the Winsock documentation. This means that I don't use an existing protocol which could simplify things and some things are done in non-standard ways e.g. measuring ping.
* It is not secure e.g. it doesn't use encryption for transferring passwords.
* It computes game state entirely on the server then sends it to the client. Most modern games will try to compute game state on the client as well so that the game reacts more quickly to the users inputs. The server is then used to correct any discrepencies between the gamestate of the client and the server.
* The graphics are poor but this was never a focus of the project.
