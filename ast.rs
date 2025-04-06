#[derive(Debug, Clone, PartialEq)]
enum Token {
    Number(i32),
    Operator(char),
    LeftParen,
    RightParen,
}
#[derive(Debug)]
enum AST {
    Number(i32),
    BinaryOp {
        op: char,
        left: Box<AST>,
        right: Box<AST>,
    },
}
fn tokenize(expr: &str) -> Vec<Token> {
    let mut tokens = Vec::new();
    let mut chars = expr.chars().peekable();

    while let Some(&ch) = chars.peek() {
        match ch {
            '0'..='9' => {
                let mut num = 0;
                while let Some(digit @ '0'..='9') = chars.peek() {
                    num = num * 10 + digit.to_digit(10).unwrap() as i32;
                    chars.next();
                }
                tokens.push(Token::Number(num));
            }
            '+' | '-' | '*' | '/' => {
                tokens.push(Token::Operator(ch));
                chars.next();
            }
            '(' => {
                tokens.push(Token::LeftParen);
                chars.next();
            }
            ')' => {
                tokens.push(Token::RightParen);
                chars.next();
            }
            ' ' => {
                chars.next();
            }
            _ => panic!("Unknown character: {}", ch),
        }
    }

    tokens
}
fn parse(tokens: &[Token]) -> AST {
    let mut output_stack: Vec<AST> = Vec::new();
    let mut operator_stack: Vec<Token> = Vec::new();

    let precedence = |op: char| match op {
        '+' | '-' => 1,
        '*' | '/' => 2,
        _ => 0,
    };

    for token in tokens {
        match token {
            Token::Number(n) => output_stack.push(AST::Number(*n)),
            Token::Operator(op) => {
                while let Some(Token::Operator(top_op)) = operator_stack.last() {
                    if precedence(*top_op) >= precedence(*op) {
                        let right = output_stack.pop().unwrap();
                        let left = output_stack.pop().unwrap();
                        output_stack.push(AST::BinaryOp {
                            op: *top_op,
                            left: Box::new(left),
                            right: Box::new(right),
                        });
                        operator_stack.pop();
                    } else {
                        break;
                    }
                }
                operator_stack.push(Token::Operator(*op));
            }
            Token::LeftParen => operator_stack.push(Token::LeftParen),
            Token::RightParen => {
                while let Some(top) = operator_stack.pop() {
                    if let Token::LeftParen = top {
                        break;
                    }
                    let right = output_stack.pop().unwrap();
                    let left = output_stack.pop().unwrap();
                    if let Token::Operator(op) = top {
                        output_stack.push(AST::BinaryOp {
                            op,
                            left: Box::new(left),
                            right: Box::new(right),
                        });
                    }
                }
            }
        }
    }

    while let Some(top) = operator_stack.pop() {
        let right = output_stack.pop().unwrap();
        let left = output_stack.pop().unwrap();
        if let Token::Operator(op) = top {
            output_stack.push(AST::BinaryOp {
                op,
                left: Box::new(left),
                right: Box::new(right),
            });
        }
    }

    output_stack.pop().unwrap()
}
fn evaluate(ast: &AST) -> i32 {
    match ast {
        AST::Number(n) => *n,
        AST::BinaryOp { op, left, right } => {
            let left_val = evaluate(left);
            let right_val = evaluate(right);
            match op {
                '+' => left_val + right_val,
                '-' => left_val - right_val,
                '*' => left_val * right_val,
                '/' => left_val / right_val,
                _ => panic!("Unknown operator"),
            }
        }
    }
}
fn main() {
    let mut expression = String::new();
    print!("in> ");
    use std::io::Write;
    std::io::stdout().flush().unwrap();
    std::io::stdin().read_line(&mut expression).unwrap();
    let tokens = tokenize(&expression[..expression.len() - 1]); // - 1 cuts off newline
    let ast = parse(&tokens);
    let result = evaluate(&ast);
    println!("out> {}", result);
}
