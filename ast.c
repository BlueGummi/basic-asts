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

Token *lex(const char *expr, int *token_count);
AST *parse(Token *tokens, int token_count);
int evaluate(AST *ast);
void print_ast(AST *ast, const char *prefix, int is_left);
void free_ast(AST *ast);

Token *lex(const char *expr, int *token_count) {
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
            fprintf(stderr, "err> unknown character: %c\n", *p);
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
        default: fprintf(stderr, "err> unknown operator %c\n", ast->value.binary_op.op); exit(1);
        }
    }
    default: fprintf(stderr, "err> unknown AST type\n"); exit(1);
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
    printf("in~> ");
    fflush(stdout);
    if (fgets(expression, sizeof(expression), stdin) == NULL) {
        printf("err> read error\n");
        exit(1);
    }
    if (strcmp(expression, "\n") == 0) {
        printf("err> invalid expression\n");
        exit(1);
    }

    int token_count;
    Token *tokens = lex(expression, &token_count);
    AST *ast = parse(tokens, token_count);
    int result = evaluate(ast);

    printf("out> %d\n", result);

    free_ast(ast);
    free(tokens);

    return 0;
}
