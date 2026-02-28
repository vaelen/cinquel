# Wordle Helper

An interactive command-line tool that helps you solve [Wordle](https://www.nytimes.com/games/wordle/index.html) puzzles. It suggests optimal guesses based on letter frequency analysis and narrows down possible answers as you provide feedback from each round.

Written in ANSI C89.

## Building and Running

```bash
make          # Compile the program
./wordle      # Run the wordle helper
make clean    # Remove compiled binary
```

Requires `gcc` (or any C compiler that supports `-std=c89`).

## How It Works

The program maintains a database of 5,757 five-letter words and uses a two-stage strategy:

1. **Suggestion words** - Maximizes coverage of untried letters, weighted by English letter frequency (e, t, a, o, ... are worth more). Used for early exploratory guesses.
2. **Matching words** - Filters the full word list against all known constraints (correct positions, required letters, eliminated letters, wrong-position constraints). Once 5 or fewer matches remain, it suggests the highest-scored match directly.

The default opening strategy is **neato** followed by **lurid**, which together cover 10 of the most common letters.

## Usage

Each round, the program shows suggestions and asks you to:

1. **Enter your guess** - The 5-letter word you played in Wordle.
2. **Enter correct letters** - All letters that were yellow or green (present in the answer).
3. **Enter correct positions** - Which positions (1-5) were green (exact match).

The program then updates its state and shows:
- Top suggestion words for letter discovery
- All matching words that satisfy every known constraint
- A recommended next guess

Press **Enter** with an empty guess to pause the game, where you can continue, start a new game, or quit.

### Example Session

```
=== SUGGESTION WORDS (maximize letter frequency) ===
1. oater (score: 120)
2. orate (score: 120)
3. arose (score: 118)
4. stoae (score: 116)
5. toeas (score: 116)

My next guess is neato.

Enter guess word (5 letters, or press Enter to quit): neato
Enter correct letters from guess (or press Enter for none): te
Enter correct positions (1-5, e.g., '14' for positions 1 and 4, or Enter for none):

=== SUGGESTION WORDS (maximize letter frequency) ===
1. girls (score: 72)
2. grils (score: 72)
3. curls (score: 70)
4. hurls (score: 70)
5. sirly (score: 70)

=== MATCHING WORDS ===
steel (score: 14)
steep (score: 14)
steed (score: 14)
...

My next guess is lurid.
```

## License

MIT License. See [LICENSE](LICENSE) for details.

Copyright (c) 2026 Andrew C. Young
