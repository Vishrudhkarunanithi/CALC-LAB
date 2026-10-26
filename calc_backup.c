#define _POSIX_C_SOURCE 200809L


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

typedef struct {
    const char *text;
    size_t pos;
    size_t length;
    size_t error_pos;
    int has_error;
} Parser;

/* Skip spaces and tabs between tokens. */
static void skip_spaces(Parser *p)
{
    while (p->pos < p->length &&
           (p->text[p->pos] == ' ' ||
            p->text[p->pos] == '\t' ||
            p->text[p->pos] == '\r')) {
        p->pos++;
    }
}

/* Record the first parsing error. */
static void set_error(Parser *p, size_t pos)
{
    if (!p->has_error) {
        p->has_error = 1;
        p->error_pos = pos;
    }
}

/* Read an integer number. */
static long long parse_number(Parser *p)
{
    skip_spaces(p);

    if (p->pos >= p->length ||
        !isdigit((unsigned char)p->text[p->pos])) {
        set_error(p, p->pos);
        return 0;
    }

    long long value = 0;

    while (p->pos < p->length &&
           isdigit((unsigned char)p->text[p->pos])) {
        int digit = p->text[p->pos] - '0';

        if (value > (LLONG_MAX - digit) / 10) {
            set_error(p, p->pos);
            return 0;
        }

        value = value * 10 + digit;
        p->pos++;
    }

    return value;
}

/* Parse addition and subtraction. */
static long long parse_expression(Parser *p)
{
    long long value = parse_number(p);

    while (!p->has_error) {
        skip_spaces(p);

        if (p->pos >= p->length) {
            break;
        }

        char op = p->text[p->pos];

        if (op != '+' && op != '-') {
            if (isdigit((unsigned char)op)) {
                set_error(p, p->pos);
            }
            break;
        }

        size_t operator_pos = p->pos;
        p->pos++;

        skip_spaces(p);

        if (p->pos >= p->length ||
            p->text[p->pos] == '+' ||
            p->text[p->pos] == '-') {
            set_error(p, p->pos);
            return 0;
        }

        long long right = parse_number(p);

        if (p->has_error) {
            return 0;
        }

        if (op == '+') {
            if (__builtin_add_overflow(value, right, &value)) {
                set_error(p, operator_pos);
                return 0;
            }
        } else {
            if (__builtin_sub_overflow(value, right, &value)) {
                set_error(p, operator_pos);
                return 0;
            }
        }
    }

    return value;
}

/* Evaluate one expression. */
static void evaluate_line(const char *line, size_t length)
{
    Parser p = {
        .text = line,
        .pos = 0,
        .length = length,
        .error_pos = 0,
        .has_error = 0
    };

    skip_spaces(&p);

    if (p.pos == p.length ||
        p.text[p.pos] == '#') {
        return;
    }

    long long result = parse_expression(&p);
    skip_spaces(&p);

    if (!p.has_error && p.pos < p.length) {
        set_error(&p, p.pos);
    }

    if (p.has_error) {
        printf("ERROR:%zu\n", p.error_pos + 1);
    } else {
        printf("%lld\n", result);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
        fprintf(stderr, "Cannot open input file: %s\n", argv[1]);
        return 1;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    while ((length = getline(&line, &capacity, file)) != -1) {
        size_t n = (size_t)length;

        if (n > 0 && line[n - 1] == '\n') {
            n--;
        }

        evaluate_line(line, n);
    }

    free(line);
    fclose(file);

    return 0;
}