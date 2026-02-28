/*
 * Wordle Helper Program
 * ANSI C89 compatible
 * Copyright (c) 2025, Andrew C. Young
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

/* Global game state */
static int guess_number = 1;
static unsigned int incorrect = 0;
static unsigned int correct = 0;
static unsigned int wrong_position[26];  /* Bitmask per letter, bits 0-4 = positions */
static char pattern[MAX_INPUT] = ".....";

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

/* Print suggestion words */
void print_suggestions(int *top_scores, int *top_indices)
{
    int i;
    printf("\n=== SUGGESTION WORDS (maximize letter frequency) ===\n");
    for (i = 0; i < MAX_SUGGESTIONS; i++) {
        if (top_indices[i] >= 0) {
            printf("%d. %s (score: %d)\n",
                   i + 1, words[top_indices[i]], top_scores[i]);
        }
    }
}

/* Print matching words */
void print_matches(int *match_scores, int *match_indices, int match_count)
{
    int i;
    printf("\n=== MATCHING WORDS ===\n");
    for (i = 0; i < MAX_MATCHES; i++) {
        if (match_indices[i] >= 0) {
            printf("%s (score: %d)\n", words[match_indices[i]], match_scores[i]);
        }
    }
    printf("\nTotal: %d matching words\n", match_count);
}

/* Print suggested next guess */
void print_next_guess(int match_count, int *match_indices, int *top_indices,
                      int next_guess_number)
{
    printf("\n");
    if (match_count <= 5 && match_indices != NULL) {
        /* 5 or fewer matches: guess highest scored match */
        if (match_indices[0] >= 0) {
            printf("My next guess is %s.\n", words[match_indices[0]]);
        }
    } else if (next_guess_number == 1) {
        /* First guess: always 'neato' */
        printf("My next guess is neato.\n");
    } else if (next_guess_number == 2) {
        /* Second guess: always 'lurid' */
        printf("My next guess is lurid.\n");
    } else {
        /* Third guess onwards: random from top 5 suggestions */
        int rand_idx = rand() % MAX_SUGGESTIONS;
        if (top_indices[rand_idx] >= 0) {
            printf("My next guess is %s.\n", words[top_indices[rand_idx]]);
        } else if (top_indices[0] >= 0) {
            printf("My next guess is %s.\n", words[top_indices[0]]);
        }
    }
}

/* Reset game state for a new game */
void reset_game(void)
{
    int i;
    guess_number = 1;
    incorrect = 0;
    correct = 0;
    strcpy(pattern, ".....");
    for (i = 0; i < 26; i++) {
        wrong_position[i] = 0;
    }
}

/* Get guess input from user */
void get_guess(char *guess)
{
    do {
        printf("%sEnter%s guess word (5 letters, or press Enter to quit): ",
               guess_number == 1 ? "" : "\n",  /* Add newline prefix if not first guess */
               guess_number == 1 ? "" : " new");  /* Add "new" if not first guess */
        fflush(stdout);
        if (fgets(guess, MAX_INPUT, stdin)) {
            strip_newline(guess);
            to_lower(guess);
        }
        if (is_empty(guess)) {
            break;  /* Allow empty input */
        }
    } while (strlen(guess) != 5);
}

/* Get correct letters from user */
void get_correct_letters(char *correct_str)
{
    printf("Enter correct letters from guess (or press Enter for none): ");
    fflush(stdout);
    if (fgets(correct_str, MAX_INPUT, stdin)) {
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

int main(void)
{
    unsigned int used;
    char input[MAX_INPUT];
    char guess[MAX_INPUT];
    char choice;

    /* For tracking top suggestions */
    int top_scores[MAX_SUGGESTIONS];
    int top_indices[MAX_SUGGESTIONS];

    /* For tracking top matches */
    int match_scores[MAX_MATCHES];
    int match_indices[MAX_MATCHES];

    int match_count;

    /* Initialize */
    srand((unsigned int)time(NULL));
    reset_game();

    while (1) {
        /* Show initial suggestions on first guess */
        if (guess_number == 1) {
            used = incorrect | correct;
            init_score_arrays(top_scores, top_indices, MAX_SUGGESTIONS);
            find_top_words(top_scores, top_indices, MAX_SUGGESTIONS, used);
            print_suggestions(top_scores, top_indices);
            print_next_guess(WORD_COUNT, NULL, top_indices, guess_number);
            printf("\n");
        } else {
            /* Show current state for subsequent guesses */
            printf("\n=== CURRENT STATE ===\n");
            printf("Incorrect letters: ");
            print_letters(incorrect);
            printf("\n");
            printf("Correct letters: ");
            print_letters(correct);
            printf("\n");
            printf("Wrong positions: ");
            print_wrong_positions(wrong_position);
            printf("\n");
            printf("Pattern: %s\n", pattern);
        }

        /* Get guess input */
        get_guess(guess);

        /* Only continue processing if guess is not empty */
        if (!is_empty(guess)) {
            /* Get correct letters and positions */
            get_correct_letters(input);
            get_positions(pattern, guess, input);

            /* Process the guess to derive incorrect letters and wrong positions */
            process_guess(guess, input, pattern, &incorrect, &correct, wrong_position);

            /* Build used letters set */
            used = incorrect | correct;

            /* Find and print suggestion words */
            init_score_arrays(top_scores, top_indices, MAX_SUGGESTIONS);
            find_top_words(top_scores, top_indices, MAX_SUGGESTIONS, used);
            print_suggestions(top_scores, top_indices);

            /* Find and print matching words */
            init_score_arrays(match_scores, match_indices, MAX_MATCHES);
            match_count = find_top_matches(match_scores, match_indices, MAX_MATCHES,
                                          pattern, correct, incorrect, wrong_position, used);
            print_matches(match_scores, match_indices, match_count);

            /* Print suggested next guess */
            print_next_guess(match_count, match_indices, top_indices, guess_number + 1);
        }

        /* Check if guess was empty */
        if (is_empty(guess)) {
            /* Empty guess: prompt for next action */
            printf("\nWhat would you like to do? (c)ontinue the game, start a (n)ew game, or (q)uit? ");
            fflush(stdout);
            if (fgets(input, MAX_INPUT, stdin)) {
                choice = tolower((unsigned char)input[0]);
                if (choice == 'q') {
                    break;
                } else if (choice == 'n') {
                    /* Reset for new game */
                    reset_game();
                    printf("\n=== STARTING NEW GAME ===\n\n");
                } else if (choice == 'c') {
                    guess_number++;
                } else {
                    /* Default to continue */
                    guess_number++;
                }
            } else {
                break;  /* EOF */
            }
        } else {
            /* Non-empty guess: automatically continue */
            guess_number++;
        }
    }

    return 0;
}
