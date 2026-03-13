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

#include "cinquel.h"
#include "ui.h"

#include "words.c"

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
            copy_string(game->suggestions[i], words[top_indices[i]], sizeof(game->suggestions[i]));
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
            copy_string(game->matches[i], words[match_indices[i]], sizeof(game->matches[i]));
            game->match_scores[i] = match_scores[i];
        } else {
            game->matches[i][0] = '\0';
            game->match_scores[i] = 0;
        }
    }

    /* Determine next guess */
    game->next_guess[0] = '\0';
    if (game->matches_count <= 5 && game->matches[0][0] != '\0') {
        copy_string(game->next_guess, game->matches[0], sizeof(game->next_guess));
    } else if (game->guess_number == 1) {
        strcpy(game->next_guess, "atone");
    } else if (game->guess_number == 2) {
        strcpy(game->next_guess, "lurid");
    } else {
        int rand_idx = rand() % MAX_SUGGESTIONS;
        if (game->suggestions[rand_idx][0] != '\0') {
            copy_string(game->next_guess, game->suggestions[rand_idx], sizeof(game->next_guess));
        } else if (game->suggestions[0][0] != '\0') {
            copy_string(game->next_guess, game->suggestions[0], sizeof(game->next_guess));
        }
    }
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

    srand((unsigned int)time(NULL));
    reset_game(&game);

    /* Mode selection */
    printf("Would you like to (p)lay or (s)olve? ");
    fflush(stdout);
    if (!fgets(input, MAX_INPUT, stdin)) {
        return 0;
    }

    if (tolower((unsigned char)input[0]) == 'p') {
        play_mode(&game);
    } else {
        solve_mode(&game);
    }

    return 0;
}
