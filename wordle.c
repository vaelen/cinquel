/*
 * Wordle Helper Program
 * ANSI C89 compatible
 * Copyright (c) 2025, Andrew C. Young
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "words.c"

#define MAX_INPUT 27  /* 26 letters + null terminator */
#define MAX_SUGGESTIONS 5
#define MAX_MATCHES 10

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

int main(void)
{
    unsigned int incorrect = 0;
    unsigned int correct = 0;
    unsigned int used;
    char pattern[MAX_INPUT];
    char input[MAX_INPUT];
    char choice;

    /* For tracking top suggestions */
    int top_scores[MAX_SUGGESTIONS];
    int top_indices[MAX_SUGGESTIONS];

    /* For tracking top matches */
    int match_scores[MAX_MATCHES];
    int match_indices[MAX_MATCHES];

    int i, j, k;
    int match_count;
    int first_round = 1;

    /* Initialize */
    strcpy(pattern, ".....");

    while (1) {
        match_count = 0;

        if (first_round) {
            /* Get initial inputs */
            printf("Enter incorrect letters (or press Enter for none): ");
            fflush(stdout);
            if (fgets(input, MAX_INPUT, stdin)) {
                strip_newline(input);
                to_lower(input);
                add_letters_from_string(&incorrect, input);
            }

            printf("Enter correct letters (or press Enter for none): ");
            fflush(stdout);
            if (fgets(input, MAX_INPUT, stdin)) {
                strip_newline(input);
                to_lower(input);
                add_letters_from_string(&correct, input);
            }
        } else {
            /* Show current state */
            printf("\n=== CURRENT STATE ===\n");
            printf("Incorrect letters: ");
            print_letters(incorrect);
            printf("\n");
            printf("Correct letters: ");
            print_letters(correct);
            printf("\n");

            /* Get additional inputs */
            printf("\nEnter additional incorrect letters (or press Enter for none): ");
            fflush(stdout);
            if (fgets(input, MAX_INPUT, stdin)) {
                strip_newline(input);
                to_lower(input);
                add_letters_from_string(&incorrect, input);
            }

            printf("Enter additional correct letters (or press Enter for none): ");
            fflush(stdout);
            if (fgets(input, MAX_INPUT, stdin)) {
                strip_newline(input);
                to_lower(input);
                add_letters_from_string(&correct, input);
            }
        }

        printf("Enter known pattern (use . for unknown, e.g., '.o..y'): ");
        fflush(stdout);
        if (fgets(pattern, MAX_INPUT, stdin)) {
            strip_newline(pattern);
            to_lower(pattern);
            /* Ensure pattern is exactly 5 characters */
            if (strlen(pattern) != 5) {
                printf("Warning: Pattern should be 5 characters. Using '.....' instead.\n");
                strcpy(pattern, ".....");
            }
        }

        /* Build used letters set */
        used = incorrect | correct;

        /* Initialize top scores */
        for (i = 0; i < MAX_SUGGESTIONS; i++) {
            top_scores[i] = -1;
            top_indices[i] = -1;
        }

        /* Find suggestion words (maximize unused letters weighted by frequency) */
        for (i = 0; i < WORD_COUNT; i++) {
            int score = score_word(words[i], used);

            /* Check if this score makes it into top suggestions */
            for (j = 0; j < MAX_SUGGESTIONS; j++) {
                if (score > top_scores[j]) {
                    int k;
                    /* Shift lower scores down */
                    for (k = MAX_SUGGESTIONS - 1; k > j; k--) {
                        top_scores[k] = top_scores[k-1];
                        top_indices[k] = top_indices[k-1];
                    }
                    top_scores[j] = score;
                    top_indices[j] = i;
                    break;
                }
            }
        }

        /* Print suggestion words */
        printf("\n=== SUGGESTION WORDS (maximize letter frequency) ===\n");
        for (i = 0; i < MAX_SUGGESTIONS; i++) {
            if (top_indices[i] >= 0) {
                printf("%d. %s (score: %d)\n",
                       i + 1, words[top_indices[i]], top_scores[i]);
            }
        }

        /* Initialize match tracking */
        for (i = 0; i < MAX_MATCHES; i++) {
            match_scores[i] = -1;
            match_indices[i] = -1;
        }

        /* Find matching words and track top by score */
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

            match_count++;
            score = score_word(word, used);

            /* Check if this score makes it into top matches */
            for (j = 0; j < MAX_MATCHES; j++) {
                if (score > match_scores[j]) {
                    /* Shift lower scores down */
                    for (k = MAX_MATCHES - 1; k > j; k--) {
                        match_scores[k] = match_scores[k-1];
                        match_indices[k] = match_indices[k-1];
                    }
                    match_scores[j] = score;
                    match_indices[j] = i;
                    break;
                }
            }
        }

        /* Print matching words sorted by score */
        printf("\n=== MATCHING WORDS ===\n");
        for (i = 0; i < MAX_MATCHES; i++) {
            if (match_indices[i] >= 0) {
                printf("%s (score: %d)\n", words[match_indices[i]], match_scores[i]);
            }
        }

        printf("\nTotal: %d matching words\n", match_count);

        /* Prompt for next action */
        printf("\nWhat would you like to do? (c)ontinue the game, start a (n)ew game, or (q)uit? ");
        fflush(stdout);
        if (fgets(input, MAX_INPUT, stdin)) {
            choice = tolower((unsigned char)input[0]);
            if (choice == 'q') {
                break;
            } else if (choice == 'n') {
                /* Reset for new game */
                incorrect = 0;
                correct = 0;
                strcpy(pattern, ".....");
                first_round = 1;
                printf("\n=== STARTING NEW GAME ===\n\n");
            } else if (choice == 'c') {
                first_round = 0;
            } else {
                /* Default to continue */
                first_round = 0;
            }
        } else {
            break;  /* EOF */
        }
    }

    return 0;
}
