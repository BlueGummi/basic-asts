#include <cctype>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

enum class TokenType {
    Number,
    Operator,
    LeftParen,
    RightParen
};

struct Token {
    TokenType type;
    union {
        int number;
        char op;
    };

    Token(int num)
        : type(TokenType::Number), number(num) {}
    Token(char op)
        : type(TokenType::Operator), op(op) {}
    Token(TokenType t)
        : type(t) {
        if (t != TokenType::LeftParen && t != TokenType::RightParen) {
            throw std::invalid_argument(std::string("invalid delimiter kind"));
        }
    }
};

class AST {
  public:
    virtual ~AST() = default;
    virtual int evaluate() const = 0;
};

class NumberAST : public AST {
    int value;

  public:
    NumberAST(int v)
        : value(v) {}
    int evaluate() const override { return value; }
};

class BinaryOpAST : public AST {
    char op;
    std::unique_ptr<AST> left;
    std::unique_ptr<AST> right;

  public:
    BinaryOpAST(char op_, std::unique_ptr<AST> l, std::unique_ptr<AST> r)
        : op(op_), left(std::move(l)), right(std::move(r)) {}

    int evaluate() const override {
        int l_val = left->evaluate();
        int r_val = right->evaluate();
        switch (op) {
        case '+': return l_val + r_val;
        case '-': return l_val - r_val;
        case '*': return l_val * r_val;
        case '/': return l_val / r_val;
        default: throw std::invalid_argument(std::string("invalid token `") + op + "`");
        }
    }
};

std::vector<Token> lex(const std::string &expr) {
    std::vector<Token> tokens;
    size_t i = 0;
    while (i < expr.size()) {
        if (std::isdigit(expr[i])) {
            int num = 0;
            while (i < expr.size() && std::isdigit(expr[i])) {
                num = num * 10 + (expr[i] - '0');
                i++;
            }
            tokens.emplace_back(num);
        } else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
            tokens.emplace_back(expr[i]);
            i++;
        } else if (expr[i] == '(') {
            tokens.emplace_back(TokenType::LeftParen);
            i++;
        } else if (expr[i] == ')') {
            tokens.emplace_back(TokenType::RightParen);
            i++;
        } else if (std::isspace(expr[i])) {
            i++;
        } else {
            throw std::runtime_error(std::string("unknown character: ") + expr[i]);
        }
    }
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

std::unique_ptr<AST> parse(const std::vector<Token> &tokens) {
    std::vector<std::unique_ptr<AST>> output_stack;
    std::vector<Token> operator_stack;

    for (const Token &token : tokens) {
        switch (token.type) {
        case TokenType::Number: {
            output_stack.push_back(std::make_unique<NumberAST>(token.number));
            break;
        }
        case TokenType::Operator: {
            while (!operator_stack.empty() &&
                   operator_stack.back().type == TokenType::Operator &&
                   precedence(operator_stack.back().op) >= precedence(token.op)) {
                char op = operator_stack.back().op;
                operator_stack.pop_back();

                if (output_stack.size() < 2) {
                    throw std::runtime_error("invalid expression");
                }

                auto right = std::move(output_stack.back());
                output_stack.pop_back();
                auto left = std::move(output_stack.back());
                output_stack.pop_back();

                output_stack.push_back(
                    std::make_unique<BinaryOpAST>(op, std::move(left), std::move(right)));
            }
            operator_stack.push_back(token);
            break;
        }
        case TokenType::LeftParen: {
            operator_stack.push_back(token);
            break;
        }
        case TokenType::RightParen: {
            while (!operator_stack.empty() && operator_stack.back().type != TokenType::LeftParen) {
                if (operator_stack.back().type != TokenType::Operator) {
                    throw std::runtime_error("invalid operator stack state");
                }
                char op = operator_stack.back().op;
                operator_stack.pop_back();

                if (output_stack.size() < 2) {
                    throw std::runtime_error("invalid expression");
                }

                auto right = std::move(output_stack.back());
                output_stack.pop_back();
                auto left = std::move(output_stack.back());
                output_stack.pop_back();

                output_stack.push_back(
                    std::make_unique<BinaryOpAST>(op, std::move(left), std::move(right)));
            }
            if (operator_stack.empty()) {
                throw std::runtime_error("mismatched parentheses");
            }
            operator_stack.pop_back();
            break;
        }
        }
    }

    while (!operator_stack.empty()) {
        if (operator_stack.back().type != TokenType::Operator) {
            throw std::runtime_error("mismatched parentheses");
        }
        char op = operator_stack.back().op;
        operator_stack.pop_back();

        if (output_stack.size() < 2) {
            throw std::runtime_error("invalid expression");
        }

        auto right = std::move(output_stack.back());
        output_stack.pop_back();
        auto left = std::move(output_stack.back());
        output_stack.pop_back();

        output_stack.push_back(
            std::make_unique<BinaryOpAST>(op, std::move(left), std::move(right)));
    }

    if (output_stack.size() != 1) {
        throw std::runtime_error("invalid expression");
    }

    return std::move(output_stack.back());
}

int main() {
    std::string expr;
    std::cout << "in> ";
    std::getline(std::cin, expr);
    try {
        auto tokens = lex(expr);
        auto ast = parse(tokens);
        int result = ast->evaluate();
        std::cout << "out> " << result << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "err> " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
