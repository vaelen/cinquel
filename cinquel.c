/*
 * Cinquel Helper Program
 * ANSI C89 compatible
 *
 * Copyright (c) 2026 Andrew C. Young
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "words.c"

#define MAX_INPUT 27  /* 26 letters + null terminator */
#define MAX_SUGGESTIONS 5
#define MAX_MATCHES 10

/* Game state */
typedef struct {
    int guess_number;
    unsigned int incorrect;
    unsigned int correct;
    unsigned int wrong_position[26];  /* Bitmask per letter, bits 0-4 = positions */
    char pattern[MAX_INPUT];
    char guess[6];
    /* Computed results */
    char suggestions[MAX_SUGGESTIONS][6];
    int suggestion_scores[MAX_SUGGESTIONS];
    int suggestions_count;
    char matches[MAX_MATCHES][6];
    int match_scores[MAX_MATCHES];
    int matches_count;
    char next_guess[6];
} GameState;

/* Check if a string is empty */
int is_empty(const char *str)
{
    return str[0] == '\0';
}

/* Check if a character is in a string */
int char_in_str(char c, const char *str)
{
    while (*str) {
        if (*str == c) {
            return 1;
        }
        str++;
    }
    return 0;
}

/* Add a letter to a bitmask */
void add_letter(unsigned int *mask, char c)
{
    if (c >= 'a' && c <= 'z') {
        *mask |= (1U << (c - 'a'));
    }
}

/* Remove a letter from a bitmask */
void remove_letter(unsigned int *mask, char c)
{
    if (c >= 'a' && c <= 'z') {
        *mask &= ~(1U << (c - 'a'));
    }
}

/* Check if a letter is in a bitmask */
int contains_letter(unsigned int mask, char c)
{
    if (c >= 'a' && c <= 'z') {
        return (mask >> (c - 'a')) & 1;
    }
    return 0;
}

/* Add all letters from a string to a bitmask */
void add_letters_from_string(unsigned int *mask, const char *str)
{
    while (*str) {
        add_letter(mask, *str);
        str++;
    }
}

/* Print letters in a bitmask */
void print_letters(unsigned int mask)
{
    char c;
    int any = 0;
    for (c = 'a'; c <= 'z'; c++) {
        if (contains_letter(mask, c)) {
            putchar(c);
            any = 1;
        }
    }
    if (!any) {
        printf("(none)");
    }
}

/* Check if word matches the known pattern (. = wildcard) */
int matches_pattern(const char *word, const char *pattern)
{
    int i;
    for (i = 0; i < 5; i++) {
        if (pattern[i] != '.' && pattern[i] != word[i]) {
            return 0;
        }
    }
    return 1;
}

/* Check if word contains all required letters (bitmask) */
int has_all_letters(const char *word, unsigned int mask)
{
    char c;
    for (c = 'a'; c <= 'z'; c++) {
        if (contains_letter(mask, c) && !char_in_str(c, word)) {
            return 0;
        }
    }
    return 1;
}

/* Check if word contains any forbidden letters (bitmask) */
int has_any_letters(const char *word, unsigned int mask)
{
    while (*word) {
        if (contains_letter(mask, *word)) {
            return 1;
        }
        word++;
    }
    return 0;
}

/* Add a wrong position for a letter */
void add_wrong_position(unsigned int *wrong_pos, char c, int position)
{
    if (c >= 'a' && c <= 'z' && position >= 0 && position < 5) {
        wrong_pos[c - 'a'] |= (1U << position);
    }
}

/* Check if a letter has a known wrong position */
int has_wrong_position(unsigned int *wrong_pos, char c, int position)
{
    if (c >= 'a' && c <= 'z' && position >= 0 && position < 5) {
        return (wrong_pos[c - 'a'] >> position) & 1;
    }
    return 0;
}

/* Check if word violates any wrong position constraints */
int violates_wrong_positions(const char *word, unsigned int *wrong_pos)
{
    int i;
    for (i = 0; i < 5; i++) {
        if (has_wrong_position(wrong_pos, word[i], i)) {
            return 1;
        }
    }
    return 0;
}

/* Process a guess word: derive incorrect letters and wrong positions */
void process_guess(const char *guess, const char *correct_str, const char *pat,
                   unsigned int *incorrect, unsigned int *correct,
                   unsigned int *wrong_pos)
{
    int i;

    for (i = 0; i < 5; i++) {
        char c = guess[i];
        if (char_in_str(c, correct_str)) {
            /* Letter is correct (yellow or green) */
            add_letter(correct, c);
            /* If pattern doesn't have this letter at this position, it's a wrong position */
            if (pat[i] != c) {
                add_wrong_position(wrong_pos, c, i);
            }
        } else {
            /* Letter is incorrect (gray) */
            add_letter(incorrect, c);
        }
    }
}

/* Print wrong positions for debugging */
void print_wrong_positions(unsigned int *wrong_pos)
{
    char c;
    int i;
    int any = 0;
    for (c = 'a'; c <= 'z'; c++) {
        if (wrong_pos[c - 'a'] != 0) {
            if (any) printf(", ");
            printf("%c:", c);
            for (i = 0; i < 5; i++) {
                if (has_wrong_position(wrong_pos, c, i)) {
                    printf("%d", i + 1);
                }
            }
            any = 1;
        }
    }
    if (!any) {
        printf("(none)");
    }
}

/* Get frequency score for a letter (higher = more common) */
int letter_score(char c)
{
    int i;
    for (i = 0; letter_frequency[i]; i++) {
        if (letter_frequency[i] == c) {
            return 26 - i;  /* e=26, t=25, ..., z=1 */
        }
    }
    return 0;
}

/* Score word based on unique unused letters weighted by frequency (bitmask) */
int score_word(const char *word, unsigned int used)
{
    int score = 0;
    unsigned int seen = 0;
    int i;

    for (i = 0; word[i]; i++) {
        char c = word[i];
        /* Skip if letter is in used set */
        if (contains_letter(used, c)) {
            continue;
        }
        /* Skip if we've already counted this letter */
        if (contains_letter(seen, c)) {
            continue;
        }
        /* Add to seen and add frequency score */
        add_letter(&seen, c);
        score += letter_score(c);
    }
    return score;
}

/* Convert string to lowercase in place */
void to_lower(char *str)
{
    while (*str) {
        *str = tolower((unsigned char)*str);
        str++;
    }
}

/* Strip newline from end of string */
void strip_newline(char *str)
{
    int len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[--len] = '\0';
    }
}

/* Reset game state for a new game */
void reset_game(GameState *game)
{
    int i;
    game->guess_number = 1;
    game->incorrect = 0;
    game->correct = 0;
    strcpy(game->pattern, ".....");
    for (i = 0; i < 26; i++) {
        game->wrong_position[i] = 0;
    }
    memset(game->suggestions, 0, sizeof(game->suggestions));
    memset(game->suggestion_scores, 0, sizeof(game->suggestion_scores));
    game->suggestions_count = 0;
    memset(game->matches, 0, sizeof(game->matches));
    memset(game->match_scores, 0, sizeof(game->match_scores));
    game->matches_count = 0;
    game->next_guess[0] = '\0';
    game->guess[0] = '\0';
}

/* Get guess input from user */
void get_guess(GameState *game, char *guess, int len)
{
    char buf[MAX_INPUT];

    do {
        printf("%sEnter%s guess word (%d letters, or press Enter to quit): ",
               game->guess_number == 1 ? "" : "\n",
               game->guess_number == 1 ? "" : " new",
               len - 1);
        fflush(stdout);
        if (!fgets(buf, MAX_INPUT, stdin)) {
            buf[0] = '\0';
            break;
        }
        strip_newline(buf);
        to_lower(buf);
        if (is_empty(buf)) {
            break;  /* Allow empty input */
        }
    } while (strlen(buf) != (size_t)(len - 1));
    memcpy(guess, buf, len);
}

/* Get correct letters from user */
void get_correct_letters(char *correct_str)
{
    printf("Enter correct letters from guess (or press Enter for none): ");
    fflush(stdout);
    if (!fgets(correct_str, MAX_INPUT, stdin)) {
        correct_str[0] = '\0';
    } else {
        strip_newline(correct_str);
        to_lower(correct_str);
    }
}

/* Get positions and update pattern */
void get_positions(char *pattern, const char *guess, const char *correct_str)
{
    char input[MAX_INPUT];
    int i;

    if (!is_empty(correct_str)) {
        printf("Enter correct positions (1-5, e.g., '14' for positions 1 and 4, or Enter for none): ");
        fflush(stdout);
        if (fgets(input, MAX_INPUT, stdin)) {
            strip_newline(input);
            /* Update pattern based on position digits */
            for (i = 0; input[i]; i++) {
                if (input[i] >= '1' && input[i] <= '5') {
                    int pos = input[i] - '1';
                    pattern[pos] = guess[pos];
                }
            }
        }
        /* EOF is treated as no positions */
    }
}

/* Initialize score arrays to -1 */
void init_score_arrays(int *scores, int *indices, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        scores[i] = -1;
        indices[i] = -1;
    }
}

/* Find top N words by score */
void find_top_words(int *scores, int *indices, int max_count, unsigned int used)
{
    int i, j, k;

    for (i = 0; i < WORD_COUNT; i++) {
        int score = score_word(words[i], used);

        /* Check if this score makes it into top N */
        for (j = 0; j < max_count; j++) {
            if (score > scores[j]) {
                /* Shift lower scores down */
                for (k = max_count - 1; k > j; k--) {
                    scores[k] = scores[k-1];
                    indices[k] = indices[k-1];
                }
                scores[j] = score;
                indices[j] = i;
                break;
            }
        }
    }
}

/* Find top N matching words by score */
int find_top_matches(int *scores, int *indices, int max_count,
                     const char *pattern, unsigned int correct,
                     unsigned int incorrect, unsigned int *wrong_pos,
                     unsigned int used)
{
    int i, j, k;
    int match_count = 0;

    for (i = 0; i < WORD_COUNT; i++) {
        const char *word = words[i];
        int score;

        /* Check all conditions */
        if (!matches_pattern(word, pattern)) {
            continue;
        }
        if (!has_all_letters(word, correct)) {
            continue;
        }
        if (has_any_letters(word, incorrect)) {
            continue;
        }
        if (violates_wrong_positions(word, wrong_pos)) {
            continue;
        }

        match_count++;
        score = score_word(word, used);

        /* Check if this score makes it into top matches */
        for (j = 0; j < max_count; j++) {
            if (score > scores[j]) {
                /* Shift lower scores down */
                for (k = max_count - 1; k > j; k--) {
                    scores[k] = scores[k-1];
                    indices[k] = indices[k-1];
                }
                scores[j] = score;
                indices[j] = i;
                break;
            }
        }
    }

    return match_count;
}

/* Compute suggestions, matches, and next guess */
void compute_suggestions(GameState *game)
{
    unsigned int used = game->incorrect | game->correct;
    int top_scores[MAX_SUGGESTIONS];
    int top_indices[MAX_SUGGESTIONS];
    int match_scores[MAX_MATCHES];
    int match_indices[MAX_MATCHES];
    int i;

    /* Find top suggestion words */
    init_score_arrays(top_scores, top_indices, MAX_SUGGESTIONS);
    find_top_words(top_scores, top_indices, MAX_SUGGESTIONS, used);
    game->suggestions_count = 0;
    for (i = 0; i < MAX_SUGGESTIONS; i++) {
        if (top_indices[i] >= 0) {
            strncpy(game->suggestions[i], words[top_indices[i]], 5);
            game->suggestions[i][5] = '\0';
            game->suggestion_scores[i] = top_scores[i];
            game->suggestions_count++;
        } else {
            game->suggestions[i][0] = '\0';
            game->suggestion_scores[i] = 0;
        }
    }

    /* Find top matching words */
    init_score_arrays(match_scores, match_indices, MAX_MATCHES);
    game->matches_count = find_top_matches(match_scores, match_indices, MAX_MATCHES,
                                           game->pattern, game->correct, game->incorrect,
                                           game->wrong_position, used);
    for (i = 0; i < MAX_MATCHES; i++) {
        if (match_indices[i] >= 0) {
            strncpy(game->matches[i], words[match_indices[i]], 5);
            game->matches[i][5] = '\0';
            game->match_scores[i] = match_scores[i];
        } else {
            game->matches[i][0] = '\0';
            game->match_scores[i] = 0;
        }
    }

    /* Determine next guess */
    game->next_guess[0] = '\0';
    if (game->matches_count <= 5 && game->matches[0][0] != '\0') {
        strncpy(game->next_guess, game->matches[0], 5);
        game->next_guess[5] = '\0';
    } else if (game->guess_number == 1) {
        strcpy(game->next_guess, "neato");
    } else if (game->guess_number == 2) {
        strcpy(game->next_guess, "lurid");
    } else {
        int rand_idx = rand() % MAX_SUGGESTIONS;
        if (game->suggestions[rand_idx][0] != '\0') {
            strncpy(game->next_guess, game->suggestions[rand_idx], 5);
            game->next_guess[5] = '\0';
        } else if (game->suggestions[0][0] != '\0') {
            strncpy(game->next_guess, game->suggestions[0], 5);
            game->next_guess[5] = '\0';
        }
    }
}

/* Display round information */
void show_round_info(GameState *game)
{
    int i;

    if (game->guess_number > 1) {
        printf("\n=== CURRENT STATE ===\n");
        printf("Incorrect letters: ");
        print_letters(game->incorrect);
        printf("\n");
        printf("Correct letters: ");
        print_letters(game->correct);
        printf("\n");
        printf("Wrong positions: ");
        print_wrong_positions(game->wrong_position);
        printf("\n");
        printf("Pattern: %s\n", game->pattern);
    }

    printf("\n=== SUGGESTION WORDS (maximize letter frequency) ===\n");
    for (i = 0; i < MAX_SUGGESTIONS; i++) {
        if (game->suggestions[i][0] != '\0') {
            printf("%d. %s (score: %d)\n", i + 1, game->suggestions[i],
                   game->suggestion_scores[i]);
        }
    }

    if (game->guess_number > 1) {
        printf("\n=== MATCHING WORDS ===\n");
        for (i = 0; i < MAX_MATCHES; i++) {
            if (game->matches[i][0] != '\0') {
                printf("%s (score: %d)\n", game->matches[i],
                       game->match_scores[i]);
            }
        }
        printf("\nTotal: %d matching words\n", game->matches_count);
    }

    if (game->next_guess[0] != '\0') {
        printf("\nMy next guess is %s.\n", game->next_guess);
    }
    printf("\n");
}

/* Process a round: gather feedback and update state */
void process_round(GameState *game, const char *guess)
{
    char correct_str[MAX_INPUT];

    get_correct_letters(correct_str);
    get_positions(game->pattern, guess, correct_str);
    process_guess(guess, correct_str, game->pattern, &game->incorrect,
                  &game->correct, game->wrong_position);
}

/* Handle empty-guess menu. Returns 0 to quit, 1 to continue. */
int prompt_action(GameState *game)
{
    char input[MAX_INPUT];
    char choice;

    printf("\nWhat would you like to do? (c)ontinue the game, start a (n)ew game, or (q)uit? ");
    fflush(stdout);
    if (fgets(input, MAX_INPUT, stdin)) {
        choice = tolower((unsigned char)input[0]);
        if (choice == 'q') {
            return 0;
        } else if (choice == 'n') {
            reset_game(game);
            printf("\n=== STARTING NEW GAME ===\n\n");
        } else {
            game->guess_number++;
        }
        return 1;
    }
    return 0;  /* EOF */
}

/* Find a word in the word list, returns index or -1 */
int find_word(const char *word)
{
    int i;
    for (i = 0; i < WORD_COUNT; i++) {
        if (strcmp(words[i], word) == 0) {
            return i;
        }
    }
    return -1;
}

/* Print a guess with colored feedback against a target word */
void print_guess_result(const char *guess, const char *target)
{
    int i;
    int target_counts[26];
    int exact[5];
    int color[5]; /* 0=incorrect, 1=wrong position, 2=correct */

    memset(target_counts, 0, sizeof(target_counts));
    memset(exact, 0, sizeof(exact));
    memset(color, 0, sizeof(color));

    /* Count letters in target */
    for (i = 0; i < 5; i++) {
        target_counts[target[i] - 'a']++;
    }

    /* First pass: mark exact matches */
    for (i = 0; i < 5; i++) {
        if (guess[i] == target[i]) {
            exact[i] = 1;
            color[i] = 2;
            target_counts[guess[i] - 'a']--;
        }
    }

    /* Second pass: mark wrong positions */
    for (i = 0; i < 5; i++) {
        if (!exact[i] && target_counts[guess[i] - 'a'] > 0) {
            color[i] = 1;
            target_counts[guess[i] - 'a']--;
        }
    }

    /* Print with colors */
    for (i = 0; i < 5; i++) {
        if (color[i] == 2) {
            printf("\033[97;44m%c\033[0m", guess[i]);
        } else if (color[i] == 1) {
            printf("\033[97;45m%c\033[0m", guess[i]);
        } else {
            printf("\033[90;40m%c\033[0m", guess[i]);
        }
    }
    printf("\n");
}

/* Prompt for new game or quit. Returns 0 to quit, 1 for new game. */
int prompt_new_or_quit(GameState *game)
{
    char input[MAX_INPUT];
    char choice;

    printf("\nStart a (n)ew game or (q)uit? ");
    fflush(stdout);
    if (fgets(input, MAX_INPUT, stdin)) {
        choice = tolower((unsigned char)input[0]);
        if (choice == 'q') {
            return 0;
        }
        reset_game(game);
        printf("\n=== STARTING NEW GAME ===\n\n");
        return 1;
    }
    return 0;
}

/* Check if pattern is fully solved (no dots) */
int pattern_solved(const char *pattern)
{
    int i;
    for (i = 0; i < 5; i++) {
        if (pattern[i] == '.') {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    GameState game;
    char input[MAX_INPUT];
    int mode; /* 0=play, 1=solve */
    char target[6];

    srand((unsigned int)time(NULL));
    reset_game(&game);

    /* Mode selection */
    printf("Would you like to (p)lay or (s)olve? ");
    fflush(stdout);
    mode = 1;
    if (!fgets(input, MAX_INPUT, stdin)) {
        return 0;
    }
    if (tolower((unsigned char)input[0]) == 'p') {
        mode = 0;
    }

    if (mode == 0) {
        /* Play mode */
        strncpy(target, words[rand() % WORD_COUNT], 5);
        target[5] = '\0';

        while (1) {
            int round;
            int won = 0;

            for (round = 1; round <= 6; round++) {
                char guess[6];

                game.guess_number = round;
                get_guess(&game, guess, sizeof(guess));

                if (is_empty(guess)) {
                    if (!prompt_action(&game)) {
                        return 0;
                    }
                    round = game.guess_number - 1;
                    continue;
                }

                if (find_word(guess) < 0) {
                    printf("Not in word list. Try again.\n");
                    round--;
                    continue;
                }

                print_guess_result(guess, target);

                if (strcmp(guess, target) == 0) {
                    printf("You win! Got it in %d guess%s.\n",
                           round, round == 1 ? "" : "es");
                    won = 1;
                    break;
                }
            }

            if (!won) {
                printf("You lose! The word was %s.\n", target);
            }

            if (!prompt_new_or_quit(&game)) {
                break;
            }
            strncpy(target, words[rand() % WORD_COUNT], 5);
            target[5] = '\0';
        }
    } else {
        /* Solve mode */
        while (1) {
            char guess[6];

            compute_suggestions(&game);
            show_round_info(&game);

            if (game.matches_count == 1) {
                if (!prompt_action(&game)) {
                    break;
                }
                continue;
            }

            get_guess(&game, guess, sizeof(guess));

            if (!is_empty(guess)) {
                strncpy(game.guess, guess, 5);
                game.guess[5] = '\0';
                process_round(&game, guess);
                game.guess_number++;

                /* Check if pattern is fully solved */
                if (pattern_solved(game.pattern)) {
                    printf("\nSolved: %s\n", game.pattern);
                    if (!prompt_new_or_quit(&game)) {
                        break;
                    }
                }
            } else {
                if (!prompt_action(&game)) {
                    break;
                }
            }
        }
    }

    return 0;
}
