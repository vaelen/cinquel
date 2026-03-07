/*
 * Cinquel Helper Program
 * ANSI C89 compatible
 *
 * Copyright (c) 2026 Andrew C. Young
 * Licensed under the MIT License. See LICENSE file for details.
 */

#ifndef CINQUEL_H
#define CINQUEL_H

#include "util.h"

#define MAX_INPUT 27  /* 26 letters + null terminator */
#define MAX_SUGGESTIONS 5
#define MAX_MATCHES 10
#define WORD_COUNT 5757

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

/* Word data (defined in words.c, included by cinquel.c) */
extern const char *letter_frequency;
extern const char *words[];

/* Engine functions */
void add_letter(unsigned int *mask, char c);
void remove_letter(unsigned int *mask, char c);
int contains_letter(unsigned int mask, char c);
void add_letters_from_string(unsigned int *mask, const char *str);
int matches_pattern(const char *word, const char *pattern);
int has_all_letters(const char *word, unsigned int mask);
int has_any_letters(const char *word, unsigned int mask);
void add_wrong_position(unsigned int *wrong_pos, char c, int position);
int has_wrong_position(unsigned int *wrong_pos, char c, int position);
int violates_wrong_positions(const char *word, unsigned int *wrong_pos);
void process_guess(const char *guess, const char *correct_str, const char *pat,
                   unsigned int *incorrect, unsigned int *correct,
                   unsigned int *wrong_pos);
int letter_score(char c);
int score_word(const char *word, unsigned int used);
void reset_game(GameState *game);
void init_score_arrays(int *scores, int *indices, int count);
void find_top_words(int *scores, int *indices, int max_count, unsigned int used);
int find_top_matches(int *scores, int *indices, int max_count,
                     const char *pattern, unsigned int correct,
                     unsigned int incorrect, unsigned int *wrong_pos,
                     unsigned int used);
void compute_suggestions(GameState *game);
int find_word(const char *word);
int pattern_solved(const char *pattern);

#endif
