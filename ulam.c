/**
 * Untyped lambda calculus interpreter
 *
 * @author Junru Shen
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>

#define C_LAMBDA '\\'

#define PARSE_ERROR(fmt, ...) \
do {\
    printf("ulam: Parse error: "fmt"\n", __VA_ARGS__); \
    abort(); \
} while (0)

#define RUNTIME_ERROR(fmt, ...) \
do {\
    printf("ulam: Runtime error: "fmt"\n", __VA_ARGS__); \
    abort(); \
} while (0)

enum token_t {
    T_VAR = 0,
    T_DOT,
    T_LAMBDA,
    T_LPAR,
    T_RPAR,
    T_UEOF
};
const char* str_token_t[] = {
        "Variable",
        "Dot",
        "Lambda",
        "(",
        ")",
        "EOF"
};
struct token {
    enum token_t type;
    const char* val;
    size_t len;
};
struct token token = {
        .type = EOF,
        .val = NULL
}, look_ahead = {
        .type = EOF,
        .val = NULL
};

enum ast_node_t {
    A_VAR,
    A_APP,
    A_ABS
};
struct ast_node {
    struct ast_node *l, *r;
    enum ast_node_t type;
    const char* var_name;
    size_t var_len;
};
#define MAKE_NODE(var, t) \
do { \
    var = malloc(sizeof(struct ast_node)); \
    var->l = var->r = NULL; \
    var->type = t; \
    var->var_name = NULL; \
    var->var_len = 0; \
} while (0)
#define BUILD_VAR(var, name, len) \
do { \
    MAKE_NODE(var, A_VAR); \
    var->var_name = name; \
    var->var_len = len; \
} while (0)
#define BUILD_APP(var, le, ri) \
do { \
    MAKE_NODE(var, A_APP); \
    var->l = le; \
    var->r = ri; \
} while (0)
#define BUILD_ABS(var, name, len, body) \
do { \
    MAKE_NODE(var, A_ABS); \
    var->var_name = name; \
    var->var_len = len; \
    var->l = body; \
} while (0)

const char *src_p = NULL, *src_beg = NULL, *src_end = NULL;

void load_src();
void next();
struct ast_node* parse_term();
struct ast_node* parse_application_term();
struct ast_node* parse_application_term_extra(struct ast_node* last);
struct ast_node* parse_atom();
void dump_ast(struct ast_node* node);

bool is_fv(struct ast_node* m, const char* name, size_t len);
void sub(struct ast_node* m, struct ast_node* n, const char* name, size_t len);
struct ast_node* dup_node(struct ast_node* m);
char* new_var_name(const char* name, size_t len, size_t* new_len);
bool eval_single_step(struct ast_node* m);
void eval(struct ast_node *m, bool show_steps);

struct ast_node* from_int(int val);
int to_int(struct ast_node* m);
bool to_bool(struct ast_node* m);

#define CODE_BUFFER_SIZE 10240
char code[CODE_BUFFER_SIZE];
size_t code_len = 0, store_len = 0;
char* store;

struct symbol {
    const char* name;
    size_t len;
    struct ast_node* node;

    struct symbol* next;
};
struct symbol* symbol_head = NULL;

struct source_stack_node {
    FILE* file;
    struct source_stack_node* next;
};
struct source_stack_node* source_stack_top = NULL;

int ulam_atoi(const char* str, size_t len);

int main(int argc, char** argv) {
    const char* path = NULL;
    int result;
    while ((result = getopt(argc, argv, "o:")) != -1) {
        switch (result) {
            case 'o':
                path = strdup(optarg);
                break;

            default:
                printf("Usage: ./lambda [-o source_code_path]\n");
                break;
        }
    }
    source_stack_top = malloc(sizeof(struct source_stack_node));
    source_stack_top->next = NULL;
    if (!path) {
        source_stack_top->file = stdin;
        printf("Untyped Lambda-Calculus Interpreter V0.1\n");
        printf("By Junru Shen, Hohai University\n");
        printf("Interactive Mode\n");
        printf("> ");
    } else {
        source_stack_top->file = fopen(path, "r");
    }
    store = NULL;
    char ch;
    bool skip = false;
    while (source_stack_top != NULL) {
        if (fscanf(source_stack_top->file, "%c", &ch) == EOF) {
            struct source_stack_node* tmp = source_stack_top;
            source_stack_top = source_stack_top->next;
            free(tmp);
            continue;
        }
        if (skip) {
            if (ch == '#') {
                skip = false;
            }
            continue;
        }
        if (ch == L'\n') {
            if (code_len >= 2 && code[code_len - 1] == L'\\') {
                code_len--;
            } else {
                if (code_len == 0) {
                    continue;
                }
                if (store != NULL && store_len) {
                    load_src(code, code_len);
                    struct ast_node* ast = parse_term();
                    if (store[0] != '$') {
                        struct symbol *sym = malloc(sizeof(struct symbol));
                        sym->next = symbol_head;
                        sym->node = dup_node(ast);
                        sym->name = store;
                        sym->len = store_len;
                        symbol_head = sym;
                        if (!path) {
                            printf("Saved %.*s\n", (int) store_len, store);
                        }
                    } else {
                        switch (store[1]) {
                            case 'i':
                                // Integer output
                                eval(ast, false);
                                printf("%d\n", to_int(ast));
                                break;

                            case 'b':
                                // Boolean output
                                eval(ast, false);
                                printf(to_bool(ast) ? "TRUE" : "FALSE");
                                break;

                            case 's':
                                // Output reduction steps
                                eval(ast, true);
                                break;

                            default:
                                PARSE_ERROR("Unexpected display modifier: $%c", store[1]);
                        }
                    }
                } else if (store != NULL && !store_len) {
                    struct source_stack_node* tmp = source_stack_top;
                    source_stack_top = malloc(sizeof(struct source_stack_node));
                    source_stack_top->next = tmp;
                    code[code_len] = '\0';
                    source_stack_top->file = fopen(code, "r");
                    if (errno) {
                        RUNTIME_ERROR("Error while loading external library %s: %s", code, strerror(errno));
                    }
                    if (!path) {
                        printf("Load external library %s OK\n", code);
                    }
                } else if (store == NULL) {
                    load_src(code, code_len);
                    struct ast_node* ast = parse_term();
                    eval(ast, false);
                    dump_ast(ast);
                    printf("\n");
                }
                store = NULL;
                code_len = 0;
                if (!path) {
                    printf("> ");
                }
                continue;
            }
        } else if (ch == ':') {
            store = malloc(code_len);
            memcpy(store, code, code_len);
            store_len = code_len;
            code_len = 0;
            continue;
        } else if (ch == '#') {
            skip = true;
            continue;
        }
        code[code_len++] = ch;
    }
}

void load_src(const char* src_code, size_t len) {
    src_p = src_beg = src_code;
    src_end = src_code + len;
    token.type = look_ahead.type = EOF;
    token.val = look_ahead.val = NULL;
    next();
}

#define OP(ch, t) \
do { \
    if (*src_p == ch) { \
        look_ahead.type = t; \
        look_ahead.val = src_p++; \
        look_ahead.len = 1; \
        return; \
    } \
} while (0)
#define EMPTY_S(ch) (ch == ' ' || ch == '\n' || ch == '\t')
#define IS_VAR_C(ch) (ch != C_LAMBDA && ch != '.' && ch != '(' && ch != ')' && !EMPTY_S(ch))

void next() {
    token = look_ahead;

    while (src_p != src_end && EMPTY_S(*src_p)) {
        src_p++;
    }
    if (src_p == src_end) {
        look_ahead.type = T_UEOF;
        return;
    }

    OP(C_LAMBDA, T_LAMBDA);
    OP('.', T_DOT);
    OP('(', T_LPAR);
    OP(')', T_RPAR);

    look_ahead.type = T_VAR;
    look_ahead.val = src_p;
    look_ahead.len = 0;
    while (src_p != src_end && IS_VAR_C(*src_p)) {
        src_p++;
        look_ahead.len++;
    }
}

#define IS_STR_EQ(a, b, a_len, b_len) ((a_len) == (b_len) && !memcmp((void*) a, (void*) b, (a_len)))
#define COPY_STR(dst, src, len) \
do { \
    dst = malloc((len)); \
    memcpy(dst, src, (len)); \
} while (0)

/*
 * Parsing
 */
#define EXPECT(t) \
do { \
    next(); \
    if (token.type != t) { \
        PARSE_ERROR("Expected %s, got %s", str_token_t[t], str_token_t[token.type]); \
    } \
} while (0)
// term
struct ast_node* parse_term() {
    switch (look_ahead.type) {
        // term ::== applicationTerm
        case T_VAR:
        case T_LPAR:
            return parse_application_term();

        // term ::== LAMBDA VAR DOT term
        case T_LAMBDA: {
            EXPECT(T_LAMBDA);
            EXPECT(T_VAR);
            struct token tmp = token;
            EXPECT(T_DOT);
            struct ast_node *node = NULL;
            BUILD_ABS(node, tmp.val, tmp.len, parse_term());
            return node;
        }

        // other: fail
        default:
            PARSE_ERROR("Unexpected token %s, expected %s, %s or %s",
                    str_token_t[look_ahead.type], str_token_t[T_VAR], str_token_t[T_LPAR], str_token_t[T_LAMBDA]);
    }
}
// applicationTerm
struct ast_node* parse_application_term() {
    switch (look_ahead.type) {
        // applicationTerm ::== atom applicationTermExtra
        case T_VAR:
        case T_LPAR:
            return parse_application_term_extra(parse_atom());

        // other: fail
        default:
            PARSE_ERROR("Unexpected token %s, expected %s or %s",
                        str_token_t[look_ahead.type], str_token_t[T_VAR], str_token_t[T_LPAR]);
    }
}
// atom
struct ast_node* parse_atom() {
    switch (look_ahead.type) {
        // atom ::== VAR
        case T_VAR: {
            EXPECT(T_VAR);
            if (token.val[0] == '$') {
                switch (token.val[1]) {
                    case 'i': {
                        int val = ulam_atoi(token.val + 2, token.len - 2);
                        return from_int(val);
                    }

                    default:
                        RUNTIME_ERROR("Wrong variable modifier", NULL);
                }
            }
            struct symbol* p_sym = symbol_head;
            while (p_sym != NULL) {
                if (IS_STR_EQ(p_sym->name, token.val, p_sym->len, token.len)) {
                    return dup_node(p_sym->node);
                }
                p_sym = p_sym->next;
            }
            struct ast_node* node;
            BUILD_VAR(node, token.val, token.len);
            return node;
        }

        // atom ::== ( term )
        case T_LPAR: {
            EXPECT(T_LPAR);
            struct ast_node* term = parse_term();
            EXPECT(T_RPAR);
            return term;
        }

        // other: fail
        default:
            PARSE_ERROR("Unexpected token %s, expected %s or %s",
                        str_token_t[look_ahead.type], str_token_t[T_VAR], str_token_t[T_LPAR]);
    }
}
// applicationTermExtra
struct ast_node* parse_application_term_extra(struct ast_node* last) {
    switch (look_ahead.type) {
        // applicationTermExtra ::== epsilon
        case T_RPAR:
        case T_UEOF:
            return last;

        // applicationTermExtra ::== atom applicationTermExtra
        case T_LPAR:
        case T_VAR: {
            struct ast_node* node = NULL;
            BUILD_APP(node, last, parse_atom());
            return parse_application_term_extra(node);
        }

        // other: fail
        default:
            PARSE_ERROR("Unexpected token %s, expected %s or %s or %s",
                        str_token_t[look_ahead.type], str_token_t[T_VAR], str_token_t[T_LPAR], str_token_t[T_RPAR]);
    }
}

void dump_ast(struct ast_node* node) {
    switch (node->type) {
        case A_ABS:
            printf("%c%.*s.", C_LAMBDA, (int) node->var_len, node->var_name);
            dump_ast(node->l);
            break;

        case A_APP:
            printf("(");
            dump_ast(node->l);
            printf(" ");
            dump_ast(node->r);
            printf(")");
            break;

        case A_VAR:
            printf("%.*s", (int) node->var_len, node->var_name);
    }
}

bool is_fv(struct ast_node* m, const char* name, size_t len) {
    bool eq = IS_STR_EQ(name, m->var_name, len, m->var_len);

    switch (m->type) {
        case A_ABS:
            return !eq && is_fv(m->l, name, len);
        case A_VAR:
            return eq;
        case A_APP:
            return is_fv(m->l, name, len) || is_fv(m->r, name, len);
    }
}

struct ast_node* dup_node(struct ast_node* m) {
    struct ast_node* node = NULL;
    char* name;
    switch (m->type) {
        case A_VAR: {
            COPY_STR(name, m->var_name, m->var_len);
            BUILD_VAR(node, name, m->var_len);
            break;
        }
        case A_APP:
            BUILD_APP(node, dup_node(m->l), dup_node(m->r));
            break;
        case A_ABS: {
            COPY_STR(name, m->var_name, m->var_len);
            BUILD_ABS(node, name, m->var_len, dup_node(m->l));
            break;
        }
    }
    return node;
}

char* new_var_name(const char* name, size_t len, size_t* new_len) {
    char* res;
    if (name[len - 1] < '0' || name[len - 1] > '8') {
        *new_len = len + 1;
        res = malloc(len + 1);
        memcpy(res, name, len);
        res[len] = '0';
    } else {
        *new_len = len;
        COPY_STR(res, name, len);
        res[len - 1]++;
    }
    return res;
}

void sub(struct ast_node* m, struct ast_node* n, const char* name, size_t len) {
    switch (m->type) {
        case A_VAR:
            if (IS_STR_EQ(m->var_name, name, m->var_len, len)) {
                memcpy(m, dup_node(n), sizeof(struct ast_node));
            }
            break;
        case A_APP:
            sub(m->l, n, name, len);
            sub(m->r, n, name, len);
            break;
        case A_ABS: {
            const char* name_org = m->var_name;
            size_t len_org = m->var_len;

            if (IS_STR_EQ(m->var_name, name, m->var_len, len)) {
                break;
            }

            while (is_fv(n, m->var_name, m->var_len)) {
                m->var_name = new_var_name(name_org, len_org, &m->var_len);
            }
            struct ast_node* var;
            BUILD_VAR(var, m->var_name, m->var_len);
            if (name_org != m->var_name) {
                sub(m->l, var, name_org, len_org);
            }

            sub(m->l, n, name, len);

            break;
        }
    }
}

bool beta_step(struct ast_node* m) {
    if (m->l->type != A_ABS) {
        return false;
    }
    sub(m->l->l, m->r, m->l->var_name, m->l->var_len);
    memcpy(m, m->l->l, sizeof(struct ast_node));
    return true;
}

bool nu_step(struct ast_node* m) {
    return eval_single_step(m->l);
}

bool mu_step(struct ast_node* m) {
    return eval_single_step(m->r);
}

bool eval_single_step(struct ast_node* m) {
    switch (m->type) {
        case A_VAR:
            return false;
        case A_ABS:
            return eval_single_step(m->l);
        case A_APP:
            return beta_step(m) || nu_step(m) || mu_step(m);
    }
}

void eval(struct ast_node *m, bool show_steps) {
    if (show_steps) {
        printf("\n-> ");
        dump_ast(m);
    }
    while (eval_single_step(m)) {
        if (show_steps) {
            printf("\n-> ");
            dump_ast(m);
        }
    }
    if (show_steps) {
        printf("\n");
    }
}

int to_int(struct ast_node* m) {
    switch (m->type) {
        case A_APP:
            return to_int(m->r) + 1;

        case A_ABS:
            return to_int(m->l);

        case A_VAR:
            return 0;
    }
}

bool to_bool(struct ast_node* m) {
    return true;
}

int ulam_atoi(const char* str, size_t len) {
    int result = 0;
    for (int i = 0; i < len; i++) {
        result = 10 * result + str[i] - '0';
    }
    return result;
}

struct ast_node* from_int_app_(int val) {
    struct ast_node* node;
    if (!val) {
        BUILD_VAR(node, "x", 1);
        return node;
    }
    struct ast_node* var;
    BUILD_VAR(var, "f", 1);
    BUILD_APP(node, var, from_int_app_(val - 1));
    return node;
}

struct ast_node* from_int(int val) {
    struct ast_node *f, *x;
    BUILD_ABS(x, "x", 1, from_int_app_(val));
    BUILD_ABS(f, "f", 1, x);
    return f;
}