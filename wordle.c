/*
 * Wordle Helper Program
 * ANSI C89 compatible
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "words.c"

#define MAX_INPUT 64
#define MAX_SUGGESTIONS 5

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

/* Check if word contains all required letters */
int has_all_letters(const char *word, const char *letters)
{
    while (*letters) {
        if (!char_in_str(*letters, word)) {
            return 0;
        }
        letters++;
    }
    return 1;
}

/* Check if word contains any forbidden letters */
int has_any_letters(const char *word, const char *letters)
{
    while (*word) {
        if (char_in_str(*word, letters)) {
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

/* Score word based on unique unused letters weighted by frequency */
int score_word(const char *word, const char *used)
{
    int score = 0;
    char seen[27];
    int seen_count = 0;
    int i;

    for (i = 0; word[i]; i++) {
        char c = word[i];
        /* Skip if letter is in used set */
        if (char_in_str(c, used)) {
            continue;
        }
        /* Skip if we've already counted this letter */
        if (char_in_str(c, seen)) {
            continue;
        }
        /* Add to seen and add frequency score */
        seen[seen_count++] = c;
        seen[seen_count] = '\0';
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
    char incorrect[MAX_INPUT];
    char correct[MAX_INPUT];
    char pattern[MAX_INPUT];
    char used[MAX_INPUT * 2];

    /* For tracking top suggestions */
    int top_scores[MAX_SUGGESTIONS];
    int top_indices[MAX_SUGGESTIONS];

    int i, j;
    int match_count = 0;

    /* Initialize */
    incorrect[0] = '\0';
    correct[0] = '\0';
    strcpy(pattern, ".....");

    /* Get inputs */
    printf("Enter incorrect letters (or press Enter for none): ");
    fflush(stdout);
    if (fgets(incorrect, MAX_INPUT, stdin)) {
        strip_newline(incorrect);
        to_lower(incorrect);
    }

    printf("Enter correct letters (or press Enter for none): ");
    fflush(stdout);
    if (fgets(correct, MAX_INPUT, stdin)) {
        strip_newline(correct);
        to_lower(correct);
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
    strcpy(used, incorrect);
    strcat(used, correct);

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

    /* Find and print matching words */
    printf("\n=== MATCHING WORDS ===\n");
    for (i = 0; i < WORD_COUNT; i++) {
        const char *word = words[i];

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
        if (match_count <= 10) {
            printf("%s\n", word);
        }
    }

    printf("\nTotal: %d matching words\n", match_count);

    return 0;
}
