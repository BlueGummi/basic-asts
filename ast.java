import java.util.*;

enum TokenType {
    NUMBER,
    OPERATOR,
    LEFT_PAREN,
    RIGHT_PAREN
}

class Token {
    TokenType type;
    String value;

    Token(TokenType type, String value) {
        this.type = type;
        this.value = value;
    }

    @Override
    public String toString() {
        return type + "(" + value + ")";
    }
}

abstract class ASTNode {
    @Override
    public abstract String toString();
}

class NumberNode extends ASTNode {
    int value;

    NumberNode(int value) {
        this.value = value;
    }

    @Override
    public String toString() {
        return String.valueOf(value);
    }
}

class BinaryOpNode extends ASTNode {
    char operator;
    ASTNode left, right;

    BinaryOpNode(char operator, ASTNode left, ASTNode right) {
        this.operator = operator;
        this.left = left;
        this.right = right;
    }

    @Override
    public String toString() {
        return "(" + left + " " + operator + " " + right + ")";
    }
}

class Tokenizer {
    private String input;
    private int pos = 0;

    Tokenizer(String input) {
        this.input = input.replaceAll("\\s+", "");
    }

    List<Token> tokenize() {
        List<Token> tokens = new ArrayList<>();

        while (pos < input.length()) {
            char ch = input.charAt(pos);
            if (Character.isDigit(ch)) {
                tokens.add(new Token(TokenType.NUMBER, readNumber()));
            } else if ("+-*/".indexOf(ch) != -1) {
                tokens.add(new Token(TokenType.OPERATOR, Character.toString(ch)));
                pos++;
            } else if (ch == '(') {
                tokens.add(new Token(TokenType.LEFT_PAREN, "("));
                pos++;
            } else if (ch == ')') {
                tokens.add(new Token(TokenType.RIGHT_PAREN, ")"));
                pos++;
            } else {
                throw new RuntimeException("invalid character: " + ch);
            }
        }
        return tokens;
    }

    private String readNumber() {
        StringBuilder num = new StringBuilder();
        while (pos < input.length() && Character.isDigit(input.charAt(pos))) {
            num.append(input.charAt(pos));
            pos++;
        }
        return num.toString();
    }
}

class Parser {
    private List<Token> tokens;
    private int pos = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    private int precedence(char op) {
        return (op == '+' || op == '-') ? 1 : (op == '*' || op == '/') ? 2
                                                                       : 0;
    }

    ASTNode parse() {
        Stack<ASTNode> output = new Stack<>();
        Stack<Token> operators = new Stack<>();

        while (pos < tokens.size()) {
            Token token = tokens.get(pos);
            switch (token.type) {
            case NUMBER:
                output.push(new NumberNode(Integer.parseInt(token.value)));
                break;
            case OPERATOR:
                while (!operators.isEmpty() && operators.peek().type == TokenType.OPERATOR && precedence(operators.peek().value.charAt(0)) >= precedence(token.value.charAt(0))) {
                    popOperator(output, operators.pop());
                }
                operators.push(token);
                break;
            case LEFT_PAREN:
                operators.push(token);
                break;
            case RIGHT_PAREN:
                while (!operators.isEmpty() && operators.peek().type != TokenType.LEFT_PAREN) {
                    popOperator(output, operators.pop());
                }
                if (!operators.isEmpty() && operators.peek().type == TokenType.LEFT_PAREN) {
                    operators.pop();
                }
                break;
            }
            pos++;
        }

        while (!operators.isEmpty()) {
            popOperator(output, operators.pop());
        }

        return output.pop();
    }

    private void popOperator(Stack<ASTNode> output, Token operatorToken) {
        ASTNode right = output.pop();
        ASTNode left = output.pop();
        output.push(new BinaryOpNode(operatorToken.value.charAt(0), left, right));
    }
}

class Evaluator {
    int evaluate(ASTNode node) {
        if (node instanceof NumberNode) {
            return ((NumberNode) node).value;
        } else if (node instanceof BinaryOpNode) {
            BinaryOpNode opNode = (BinaryOpNode) node;
            int left = evaluate(opNode.left);
            int right = evaluate(opNode.right);
            switch (opNode.operator) {
            case '+': return left + right;
            case '-': return left - right;
            case '*': return left * right;
            case '/': return left / right;
            default: throw new RuntimeException("unknown operator: " + opNode.operator);
            }
        }
        throw new RuntimeException("unknown AST node");
    }
}

public class ast {
    public static void main(String[] args) {
        String expression;
        Scanner kb = new Scanner(System.in);
        System.out.print("in> ");
        expression = kb.nextLine();
        Tokenizer tokenizer = new Tokenizer(expression);
        List<Token> tokens = tokenizer.tokenize();

        Parser parser = new Parser(tokens);
        ASTNode ast = parser.parse();

        Evaluator evaluator = new Evaluator();
        int result = evaluator.evaluate(ast);
        System.out.println("out> " + result);
    }
}
