/*
 * Cinquel Terminal UI
 * ANSI C89 compatible
 *
 * Copyright (c) 2026 Andrew C. Young
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cinquel.h"

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
    copy_string(guess, buf, len);
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

/* Print a guess with colored feedback against a target word */
void print_guess_result(const char *guess, const char *target)
{
    int i;
    int target_counts[26];
    int exact[5];
    int color[5]; /* 0=incorrect, 1=wrong position, 2=correct */

    /* Validate that all characters are lowercase a-z */
    for (i = 0; i < 5; i++) {
        if (guess[i] < 'a' || guess[i] > 'z' || target[i] < 'a' || target[i] > 'z') {
            printf("Error: invalid characters in guess or target\n");
            return;
        }
    }

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

/* Play mode loop */
void play_mode(GameState *game)
{
    char target[6];

    copy_string(target, words[rand() % WORD_COUNT], sizeof(target));

    while (1) {
        int round;
        int won = 0;

        for (round = 1; round <= 6; round++) {
            char guess[6];

            game->guess_number = round;
            get_guess(game, guess, sizeof(guess));

            if (is_empty(guess)) {
                if (!prompt_action(game)) {
                    return;
                }
                round = game->guess_number - 1;
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

        if (!prompt_new_or_quit(game)) {
            break;
        }
        copy_string(target, words[rand() % WORD_COUNT], sizeof(target));
    }
}

/* Solve mode loop */
void solve_mode(GameState *game)
{
    while (1) {
        char guess[6];

        compute_suggestions(game);
        show_round_info(game);

        if (game->matches_count == 1) {
            if (!prompt_action(game)) {
                break;
            }
            continue;
        }

        get_guess(game, guess, sizeof(guess));

        if (!is_empty(guess)) {
            copy_string(game->guess, guess, sizeof(game->guess));
            process_round(game, guess);
            game->guess_number++;

            /* Check if pattern is fully solved */
            if (pattern_solved(game->pattern)) {
                printf("\nSolved: %s\n", game->pattern);
                if (!prompt_new_or_quit(game)) {
                    break;
                }
            }
        } else {
            if (!prompt_action(game)) {
                break;
            }
        }
    }
}
