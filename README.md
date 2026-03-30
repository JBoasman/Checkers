Java Checkers

I started this as my first project as a deep dive into game state logic, object-oriented design, and different levels of artificial intelligence. 
This was a significant challenge for me, especially on the debugging side of things.

🛠 Features & Implementation
1. The Game Engine & Rule Implementation

The game rules were fairy simple to implement. Here I enforce the standard rules of Checkers mostly using booleans.

    Validation: Implemented logic for legal moves, ensuring pieces only move diagonally and forward (unless they’re Kings!).

    Jump Logic: Handled the complexity of forced jumps and multi-jump sequences, which was arguably the trickiest part of the state management.

    Win Conditions: The game actively monitors the board to detect when a player is out of pieces or blocked from making any further moves.

2. The Display Window (JavaFX)

I wanted the interface to feel intuitive, so I used JavaFX to create a clean, responsive board.

    Visual Cues: The UI highlights available moves when you click a piece, making the game accessible for beginners.

    State Sync: The display stays perfectly in sync with the underlying data model, ensuring that "what you see is what the engine thinks."

3. The "Easy" Bot (Randomized Logic)

The Easy Bot is designed to be a "casual" opponent. 

    How it works: It scans the board for all possible legal moves and picks one at random.

    The Experience: It’s unpredictable and great for testing the game rules, but it doesn't plan ahead—making it a perfect warm-up for a new player.

4. The "Hard" Bot (Minimax Algorithm)

This is where things got serious. To create a challenging opponent, I implemented a Minimax algorithm.

    Lookahead: The bot "simulates" several turns into the future to see how the board might look based on your potential responses.

    Heuristics: It evaluates board positions based on piece count, king status, and board control.

    The Result: It plays a much more defensive and strategic game, actively trying to bait the player into traps.