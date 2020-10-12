/**
 * Untyped lambda calculus interpreter
 *
 * @author Junru Shen
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define C_LAMBDA '\\'

#define PARSE_ERROR(fmt, ...) \
do {\
    printf("Lambda: Parse error: "fmt"\n", __VA_ARGS__); \
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
struct ast_node* e();
void dump_ast(struct ast_node* node);

bool is_fv(struct ast_node* m, const char* name, size_t len);
void del(struct ast_node* m);
void sub(struct ast_node* m, struct ast_node* n, const char* name, size_t len);
struct ast_node* dup_node(struct ast_node* m);
char* new_var_name(const char* name, size_t len, size_t* new_len);
bool eval_single_step(struct ast_node* m);
void eval(struct ast_node* m);

int main() {
    const char* code = "(((\\f.\\g.\\x.((f x)  (g x))  \\m.\\n.(n m))  \\n.z)p)";
    load_src(code, strlen(code));
    struct ast_node* node = e();
    eval(node);
    dump_ast(node);
    return 0;
}

void load_src(const char* src_code, size_t len) {
    src_p = src_beg = src_code;
    src_end = src_code + len;
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

#define EXPECT(t) \
do { \
    next(); \
    if (token.type != t) { \
        PARSE_ERROR("Expected %s, got %s", str_token_t[t], str_token_t[token.type]); \
    } \
} while (0)
struct ast_node* e() {
    struct ast_node *node = NULL;

    switch (look_ahead.type) {
        case T_VAR:
            EXPECT(T_VAR);
            BUILD_VAR(node, token.val, token.len);
            break;
        case T_LAMBDA: {
            EXPECT(T_LAMBDA);
            EXPECT(T_VAR);
            struct token tmp = token;
            EXPECT(T_DOT);
            BUILD_ABS(node, tmp.val, tmp.len, e());
            break;
        }
        case T_LPAR:
            EXPECT(T_LPAR);
            BUILD_APP(node, e(), e());
            EXPECT(T_RPAR);
            break;
        case T_DOT:
            PARSE_ERROR("Unexpected token .", NULL);
            break;
        case T_RPAR:
            PARSE_ERROR("Unexpected token )", NULL);
            break;
        case T_UEOF:
            PARSE_ERROR("Unexpected token EOF", NULL);
            break;
    }

    return node;
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

#define IS_STR_EQ(a, b, a_len, b_len) ((a_len) == (b_len) && !memcmp((void*) a, (void*) b, (a_len)))
#define COPY_STR(dst, src, len) \
do { \
    dst = malloc((len)); \
    memcpy(dst, src, (len)); \
} while (0)

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
            BUILD_ABS(node, name, m->var_len, m->l);
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

void eval(struct ast_node* m) {
    while (eval_single_step(m));
}
