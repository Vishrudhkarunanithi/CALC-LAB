
// Name: Vishrudh Pattukkottai Karunanithi
// Student ID : 251ADB035
// https://github.com/Vishrudhkarunanithi/CALC-LAB
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: <every AI tool, website and book you used; details in comments where you used them>
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#define STUDENT_NAME "Student"
#define STUDENT_LASTNAME "User"
#define STUDENT_ID "0000"


typedef struct {
    const char *text;
    size_t pos, length, error_pos, offset;
    int has_error;
} Parser;

typedef struct {
    double value;
    int is_integer;
} Value;

static void skip_spaces(Parser *p)
{
    while (p->pos < p->length &&
           (p->text[p->pos] == ' ' ||
            p->text[p->pos] == '\t' ||
            p->text[p->pos] == '\r'))
        p->pos++;
}

static void set_error(Parser *p, size_t pos)
{
    if (!p->has_error) {
        p->has_error = 1;
        p->error_pos = p->offset + pos;
    }
}

static Value parse_expression(Parser *p);
static Value parse_term(Parser *p);
static Value parse_power(Parser *p);

static Value make_value(double n, int is_integer)
{
    Value v = { n, is_integer };
    return v;
}

static Value parse_primary(Parser *p)
{
    skip_spaces(p);

    if (p->pos >= p->length) {
        set_error(p, p->pos);
        return make_value(0, 0);
    }

    if (p->text[p->pos] == '(') {
        size_t start = p->pos++;
        Value v = parse_expression(p);
        skip_spaces(p);

        if (p->pos >= p->length || p->text[p->pos] != ')') {
            set_error(p, p->pos);
            return make_value(0, 0);
        }

        p->pos++;
        (void)start;
        return v;
    }

    size_t start = p->pos;
    int digits = 0;
    int is_integer = 1;

    while (p->pos < p->length &&
           isdigit((unsigned char)p->text[p->pos])) {
        p->pos++;
        digits++;
    }

    if (p->pos < p->length && p->text[p->pos] == '.') {
        is_integer = 0;
        size_t dot = p->pos++;
        size_t fraction_start = p->pos;

        while (p->pos < p->length &&
               isdigit((unsigned char)p->text[p->pos]))
            p->pos++;

        if (digits == 0 || p->pos == fraction_start) {
            set_error(p, dot);
            return make_value(0, 0);
        }
    }

    if (digits == 0) {
        set_error(p, start);
        return make_value(0, 0);
    }

    size_t count = p->pos - start;
    char *number = malloc(count + 1);

    if (!number) {
        set_error(p, start);
        return make_value(0, 0);
    }

    memcpy(number, p->text + start, count);
    number[count] = '\0';

    char *end;
    errno = 0;
    double value = strtod(number, &end);

    if (errno == ERANGE || *end != '\0' || !isfinite(value))
        set_error(p, start);

    free(number);
    return make_value(value, is_integer);
}

static Value parse_power(Parser *p)
{
    Value left = parse_primary(p);
    if (p->has_error) return left;

    skip_spaces(p);

    if (p->pos + 1 < p->length &&
        p->text[p->pos] == '*' &&
        p->text[p->pos + 1] == '*') {
        size_t op_pos = p->pos;
        p->pos += 2;
        skip_spaces(p);

        size_t right_start = p->pos;
        Value right = parse_power(p);

        if (!p->has_error) {
            if (right.value < 0) {
                set_error(p, right_start);
                return make_value(0, 0);
            }

            left.value = pow(left.value, right.value);
            left.is_integer = left.is_integer && right.is_integer;

            if (!isfinite(left.value))
                 set_error(p, op_pos);
        }
    }

    return left;
}

static Value parse_term(Parser *p)
{
    Value left = parse_power(p);

    while (!p->has_error) {
        skip_spaces(p);
        if (p->pos >= p->length) break;

        char op = p->text[p->pos];

        if (op != '*' && op != '/') break;

        if (p->pos + 1 < p->length &&
            p->text[p->pos + 1] == '*')
            break;

        p->pos++;
        skip_spaces(p);
        size_t right_start = p->pos;
        Value right = parse_power(p);

        if (p->has_error) return left;

        if (op == '/' && right.value == 0.0) {
            set_error(p, right_start);
            return make_value(0, 0);
        }

        if (op == '/' && left.is_integer && right.is_integer) {
            left.value = trunc(left.value / right.value);
            left.is_integer = 1;
        } else {
            left.value = op == '*' ?
                left.value * right.value :
                left.value / right.value;
            left.is_integer = op == '*' &&
                left.is_integer && right.is_integer;
        }

        if (!isfinite(left.value)) {
            set_error(p, right_start);
            return make_value(0, 0);
        }
    }

    return left;
}

static Value parse_expression(Parser *p)
{
    Value left = parse_term(p);

    while (!p->has_error) {
        skip_spaces(p);
        if (p->pos >= p->length) break;

        char op = p->text[p->pos];
        if (op != '+' && op != '-') break;

        p->pos++;
        skip_spaces(p);
        size_t right_start = p->pos;

        if (p->pos < p->length &&
            (p->text[p->pos] == '+' ||
             p->text[p->pos] == '-')) {
            set_error(p, p->pos);
            return make_value(0, 0);
        }

        Value right = parse_term(p);
        if (p->has_error) return left;

        left.value = op == '+' ?
            left.value + right.value :
            left.value - right.value;

        left.is_integer = left.is_integer && right.is_integer;

        if (!isfinite(left.value)) {
            set_error(p, right_start);
            return make_value(0, 0);
        }
    }

     return left;
}


static int calculate_file(const char *input, const char *output)
{
    FILE *in = fopen(input, "r");

    if (!in) {
        fprintf(stderr, "Cannot open input file: %s\n", input);
        return -1;
    }

    FILE *out =fopen(output, "w");

    if (!out) {
        fprintf(stderr, "Cannot create output file: %s\n", output);
        fclose(in);
        return -1;
    }

    FILE *temp = tmpfile();

    if (!temp) {
        fprintf(stderr, "Cannot create temporary output\n");
        fclose(in);
        fclose(out);
        return -1;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    size_t offset = 0;
    size_t error_position = 0;
    int has_error = 0;

    while ((length = getline(&line, &capacity, in)) != -1) {
        size_t n = (size_t)length;
        size_t content_length = n;

        if (content_length > 0 && line[content_length - 1] == '\n')
            content_length--;

        Parser p = {
            .text = line,
            .pos = 0,
            .length = content_length,
            .error_pos = 0,
            .offset = offset,
            .has_error = 0
        };

        skip_spaces(&p);

        if (p.pos < p.length && p.text[p.pos] != '#') {
            Value result = parse_expression(&p);
            skip_spaces(&p);

            if (!p.has_error && p.pos < p.length)
                set_error(&p, p.pos);

            if (p.has_error) {
                has_error = 1;
                error_position = p.error_pos + 1;
                break;
            }

            fprintf(temp, "%.15g\n", result.value);
        }

        offset += n;
    }

    if (ferror(in)) {
        fprintf(stderr, "Error reading input file: %s\n", input);
        free(line);
        fclose(in);
        fclose(temp);
        fclose(out);
        return -1;
    }

    if (has_error) {
        fprintf(out, "ERROR:%zu\n", error_position);
    } else {
        rewind(temp);
        int ch;

        while ((ch = fgetc(temp)) != EOF)
            fputc(ch, out);
    }

    free(line);
    fclose(in);
    fclose(temp);

    if (fclose(out) != 0)
        return -1;

    return 0;
}


static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int is_directory(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int ensure_directory(const char *path)
{
    if (mkdir(path, 0777) == 0) return 0;

    if (errno == EEXIST && is_directory(path)) return 0;

    fprintf(stderr, "Cannot create output directory: %s\n", path);
    return -1;
}

static int make_output_path(char *buffer, size_t size,const char *directory, const char *base)
{
    int n = snprintf(buffer, size, "%s/%s_%s_%s_%s.txt",directory, base,
                     STUDENT_NAME, STUDENT_LASTNAME, STUDENT_ID);

    if (n < 0 || (size_t)n >= size) {
        fprintf(stderr, "Output path is too long\n");
        return -1;
    }

    return 0;
}

static int process_file(const char *input, const char *outdir)
{
    const char *filename = base_name(input);
    size_t len = strlen(filename);

    if (len <= 4 || strcmp(filename + len - 4, ".txt") != 0) {
        fprintf(stderr, "Input must be a .txt file: %s\n", input);
        return -1;
    }

    size_t base_len = len - 4;
    char *base = malloc(base_len + 1);

    if (!base) {
        fprintf(stderr, "Out of memory\n");
        return -1;
    }

    memcpy(base, filename, base_len);
    base[base_len] = '\0';

    char output[PATH_MAX];

    int status = make_output_path(output, sizeof output, outdir, base);

    free(base);

    if (status != 0) return -1;

    return calculate_file(input, output);
}

static int process_directory(const char *input_dir, const char *outdir)
{
    DIR *dir = opendir(input_dir);

    if (!dir) {
        fprintf(stderr, "Cannot open input directory: %s\n", input_dir);
        return -1;
    }

    struct dirent *entry;
    int failed = 0;

    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);

        if (len <= 4 || strcmp(name + len - 4, ".txt") != 0)
            continue;

        char input[PATH_MAX];

        int n = snprintf(input, sizeof input, "%s/%s", input_dir, name);

        if (n < 0 || (size_t)n >= sizeof input) {
            fprintf(stderr, "Input path is too long\n");
            failed = 1;
            continue;
        }

        struct stat st;

        if (stat(input, &st) != 0 || !S_ISREG(st.st_mode))
            continue;

        if (process_file(input, outdir) != 0)
            failed = 1;
    }

    closedir(dir);
    return failed ? -1 : 0;
}

static void usage(const char *program)
{
    fprintf(stderr,
        "Usage: %s [-o output_dir] input.txt\n"
        "       %s [-d input_dir] [-o output_dir]\n",
        program, program);
}

int main(int argc, char *argv[])
{
    const char *input = NULL;
    const char *outdir = NULL;
    int directory_mode = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 ||
            strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing output directory\n");
                usage(argv[0]);
                return 1;
            }
            outdir = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 ||
                   strcmp(argv[i], "--dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing input directory\n");
                usage(argv[0]);
                return 1;
            }
            input = argv[++i];
            directory_mode = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        } else {
            if (input != NULL) {
                fprintf(stderr, "Too many input arguments\n");
                usage(argv[0]);
                return 1;
            }
            input = argv[i];
        }
    }

    if (!input) {
        fprintf(stderr, "No input specified\n");
        usage(argv[0]);
        return 1;
    }

    if (directory_mode && !is_directory(input)) {
        fprintf(stderr, "Input directory does not exist: %s\n", input);
        return 1;
    }

    if (!directory_mode && is_directory(input)) {
        fprintf(stderr, "Expected an input file, got a directory: %s\n", input);
        return 1;
    }

    if (!directory_mode) {
        FILE *check = fopen(input, "r");

        if (!check) {
            fprintf(stderr, "Cannot open input file: %s\n", input);
            return 1;
        }

        fclose(check);
    }

    char default_dir[PATH_MAX];

    if (!outdir) {
        const char *base = base_name(input);
        size_t len = strlen(base);

        if (!directory_mode && len >= 4 &&
            strcmp(base + len - 4, ".txt") == 0)
            len -= 4;

        if (len == 0) {
            fprintf(stderr, "Invalid input name\n");
            return 1;
        }

        int n = snprintf(default_dir, sizeof default_dir,
                         "%.*s_%s_%s",
                         (int)len, base,
                         STUDENT_NAME, STUDENT_ID);

        if (n < 0 || (size_t)n >= sizeof default_dir) {
            fprintf(stderr, "Output directory name is too long\n");
            return 1;
        }

        outdir = default_dir;
    }

    if (ensure_directory(outdir) != 0)
        return 1;

    int result = directory_mode
        ? process_directory(input, outdir)
        : process_file(input, outdir);

    return result == 0 ? 0 : 1;
}
