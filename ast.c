#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN
} TokenType;

typedef struct {
    TokenType type;
    union {
        int number;
        char operator;
    } value;
} Token;

typedef enum {
    AST_NUMBER,
    AST_BINARY_OP
} ASTType;

typedef struct AST {
    ASTType type;
    union {
        int number;
        struct {
            char op;
            struct AST *left;
            struct AST *right;
        } binary_op;
    } value;
} AST;

Token *tokenize(const char *expr, int *token_count);
AST *parse(Token *tokens, int token_count);
int evaluate(AST *ast);
void print_ast(AST *ast, const char *prefix, int is_left);
void free_ast(AST *ast);

Token *tokenize(const char *expr, int *token_count) {
    Token *tokens = malloc(strlen(expr) * sizeof(Token));
    int count = 0;
    const char *p = expr;

    while (*p) {
        if (isdigit(*p)) {
            int num = 0;
            while (isdigit(*p)) {
                num = num * 10 + (*p - '0');
                p++;
            }
            tokens[count].type = TOKEN_NUMBER;
            tokens[count].value.number = num;
            count++;
        } else if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            tokens[count].type = TOKEN_OPERATOR;
            tokens[count].value.operator= * p;
            count++;
            p++;
        } else if (*p == '(') {
            tokens[count].type = TOKEN_LEFT_PAREN;
            count++;
            p++;
        } else if (*p == ')') {
            tokens[count].type = TOKEN_RIGHT_PAREN;
            count++;
            p++;
        } else if (isspace(*p)) {
            p++;
        } else {
            fprintf(stderr, "Unknown character: %c\n", *p);
            exit(1);
        }
    }

    *token_count = count;
    return tokens;
}
int precedence(char op) {
    switch (op) {
    case '+':
    case '-': return 1;
    case '*':
    case '/': return 2;
    default: return 0;
    }
}
AST *parse(Token *tokens, int token_count) {
    AST **output_stack = malloc(token_count * sizeof(AST *));
    Token *operator_stack = malloc(token_count * sizeof(Token));
    int output_top = -1;
    int operator_top = -1;

    for (int i = 0; i < token_count; i++) {
        Token token = tokens[i];
        switch (token.type) {
        case TOKEN_NUMBER: {
            AST *num_node = malloc(sizeof(AST));
            num_node->type = AST_NUMBER;
            num_node->value.number = token.value.number;
            output_stack[++output_top] = num_node;
            break;
        }
        case TOKEN_OPERATOR: {
            while (operator_top >= 0 && operator_stack[operator_top].type == TOKEN_OPERATOR &&
                   precedence(operator_stack[operator_top].value.operator) >= precedence(token.value.operator)) {
                AST *right = output_stack[output_top--];
                AST *left = output_stack[output_top--];
                AST *op_node = malloc(sizeof(AST));
                op_node->type = AST_BINARY_OP;
                op_node->value.binary_op.op = operator_stack[operator_top].value.operator;
                op_node->value.binary_op.left = left;
                op_node->value.binary_op.right = right;
                output_stack[++output_top] = op_node;
                operator_top--;
            }
            operator_stack[++operator_top] = token;
            break;
        }
        case TOKEN_LEFT_PAREN: {
            operator_stack[++operator_top] = token;
            break;
        }
        case TOKEN_RIGHT_PAREN: {
            while (operator_top >= 0 && operator_stack[operator_top].type != TOKEN_LEFT_PAREN) {
                AST *right = output_stack[output_top--];
                AST *left = output_stack[output_top--];
                AST *op_node = malloc(sizeof(AST));
                op_node->type = AST_BINARY_OP;
                op_node->value.binary_op.op = operator_stack[operator_top].value.operator;
                op_node->value.binary_op.left = left;
                op_node->value.binary_op.right = right;
                output_stack[++output_top] = op_node;
                operator_top--;
            }
            if (operator_top >= 0 && operator_stack[operator_top].type == TOKEN_LEFT_PAREN) {
                operator_top--;
            }
            break;
        }
        }
    }

    while (operator_top >= 0) {
        AST *right = output_stack[output_top--];
        AST *left = output_stack[output_top--];
        AST *op_node = malloc(sizeof(AST));
        op_node->type = AST_BINARY_OP;
        op_node->value.binary_op.op = operator_stack[operator_top].value.operator;
        op_node->value.binary_op.left = left;
        op_node->value.binary_op.right = right;
        output_stack[++output_top] = op_node;
        operator_top--;
    }

    AST *result = output_stack[output_top];
    free(output_stack);
    free(operator_stack);
    return result;
}

int evaluate(AST *ast) {
    switch (ast->type) {
    case AST_NUMBER:
        return ast->value.number;
    case AST_BINARY_OP: {
        int left_val = evaluate(ast->value.binary_op.left);
        int right_val = evaluate(ast->value.binary_op.right);
        switch (ast->value.binary_op.op) {
        case '+': return left_val + right_val;
        case '-': return left_val - right_val;
        case '*': return left_val * right_val;
        case '/': return left_val / right_val;
        default: fprintf(stderr, "Unknown operator\n"); exit(1);
        }
    }
    default: fprintf(stderr, "Unknown AST type\n"); exit(1);
    }
}

void print_ast(AST *ast, const char *prefix, int is_left) {
    if (ast->type == AST_BINARY_OP) {
        printf("%s%s┬────┐\n", prefix, is_left ? "├" : "└");
        printf("%s%s│ %c  │\n", prefix, is_left ? "│" : " ", ast->value.binary_op.op);
        printf("%s%s└──┬─┘\n", prefix, is_left ? "│" : " ");

        char new_prefix[256];
        snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_left ? "│   " : "    ");
        print_ast(ast->value.binary_op.left, new_prefix, 1);
        print_ast(ast->value.binary_op.right, new_prefix, 0);
    } else if (ast->type == AST_NUMBER) {
        printf("%s%s┬────┐\n", prefix, is_left ? "├" : "└");
        printf("%s%s│ %2d │\n", prefix, is_left ? "│" : " ", ast->value.number);
        printf("%s%s└────┘\n", prefix, is_left ? "│" : " ");
    }
}

void free_ast(AST *ast) {
    if (ast->type == AST_BINARY_OP) {
        free_ast(ast->value.binary_op.left);
        free_ast(ast->value.binary_op.right);
    }
    free(ast);
}

int main() {
    char expression[500] = {0};
    printf("in> ");
    fflush(stdout);
    fgets(expression, sizeof(expression), stdin);
    int token_count;
    Token *tokens = tokenize(expression, &token_count);
    AST *ast = parse(tokens, token_count);
    int result = evaluate(ast);

    printf("ast>\n");
    print_ast(ast, "", 0);
    printf("out> %d\n", result);

    free_ast(ast);
    free(tokens);

    return 0;
}
