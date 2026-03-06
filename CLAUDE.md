# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

An interactive command-line tool that helps you solve five-letter word puzzles. It suggests optimal guesses based on letter frequency analysis and narrows down possible answers as you provide feedback from each round.

## Building and Running

```bash
make          # Compile the program
./cinquel     # Run the cinquel helper
make clean    # Remove compiled binary
```

The program is compiled with strict C89 compliance (`-std=c89 -pedantic -Wall -Wextra`).

## Code Structure

- **cinquel.c** - Main program logic (interactive loop, user input, word filtering, scoring)
- **words.c** - Word database (5757 five-letter words) and letter frequency data

Note: `cinquel.c` includes `words.c` directly via `#include`, not as a separate compilation unit.

## Core Architecture

### Game State

Game state is encapsulated in a `GameState` struct (allocated on the stack in `main()` and passed by pointer to functions that need it):
- `guess_number` - Current guess number (1-based)
- `incorrect` - Bitmask of incorrect letters
- `correct` - Bitmask of correct letters
- `wrong_position[26]` - Per-letter position constraints (5 bits each for positions 0-4)
- `pattern` - The pattern being built (e.g., "..a..")

Call `reset_game(&game)` to reset all state for a new game.

### Data Representation

The program uses **bitmasks** extensively for efficient letter tracking:
- `unsigned int` bitmasks track sets of letters (26 bits, one per letter a-z)
- Position constraints use 5 bits per letter (one bit for each of the 5 positions)

### Two-Stage Word Suggestion

1. **Suggestion Words** - Maximize coverage of unused letters weighted by frequency
   - Scored by `score_word()` which counts unique unused letters × frequency rank
   - Used for exploratory guesses (1st-3rd guesses follow a strategy: neato → lurid → random from top 5)

2. **Matching Words** - Satisfy all known constraints (pattern, correct letters, wrong positions)
   - Filtered through pattern matching, letter requirements, and position exclusions
   - When ≤5 matches remain, suggests the highest-scored match

### User Interaction Flow

The program operates in a loop where:
- User enters a guess and feedback (correct letters, positions)
- **Empty guess** triggers the continue/new/quit prompt
- **Valid guess** automatically continues to the next round
- All processing (asking for correct letters, positions, finding matches) is skipped if guess is empty

### Key Helper Functions

**State Management:**
- `reset_game()` - Resets all global game state for a new game
- `is_empty()` - Checks if a string is empty

**User Input:**
- `get_guess()` - Gets guess input from user (accesses global `guess_number`)
- `get_correct_letters()` - Gets correct letters from user feedback
- `get_positions()` - Gets position input and updates pattern

**Word Processing:**
- `init_score_arrays()` - Initializes score/index arrays to -1
- `find_top_words()` - Finds top N suggestion words by score
- `find_top_matches()` - Finds top N matching words that satisfy all constraints
- `process_guess()` - Derives incorrect letters and wrong positions from user feedback

**Filtering:**
- `matches_pattern()`, `has_all_letters()`, `violates_wrong_positions()` - Word filtering predicates
- `score_word()` - Scores words based on unique unused letters weighted by frequency

**Constants:**
- `letter_frequency` - Ordered string "etaoinsrhldcumfpgwybvkxjqz" for frequency-based scoring
