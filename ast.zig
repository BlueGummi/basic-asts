const std = @import("std");
const io = std.io;
const mem = std.mem;
const fmt = std.fmt;
const math = std.math;
const unicode = std.unicode;
const ascii = std.ascii;

const TokenType = enum {
    eof,
    number,
    plus,
    minus,
    multiply,
    divide,
    lparen,
    rparen,
};

const Token = struct {
    type: TokenType,
    value: []const u8,
};

const Lexer = struct {
    input: []const u8,
    position: usize = 0,

    fn init(input: []const u8) Lexer {
        return Lexer{ .input = input };
    }

    fn nextToken(self: *Lexer) !Token {
        while (self.position < self.input.len and ascii.isWhitespace(self.input[self.position])) {
            self.position += 1;
        }

        if (self.position >= self.input.len) {
            return Token{ .type = .eof, .value = "" };
        }

        const ch = self.input[self.position];
        switch (ch) {
            '+' => {
                self.position += 1;
                return Token{ .type = .plus, .value = self.input[self.position - 1 .. self.position] };
            },
            '-' => {
                self.position += 1;
                return Token{ .type = .minus, .value = self.input[self.position - 1 .. self.position] };
            },
            '*' => {
                self.position += 1;
                return Token{ .type = .multiply, .value = self.input[self.position - 1 .. self.position] };
            },
            '/' => {
                self.position += 1;
                return Token{ .type = .divide, .value = self.input[self.position - 1 .. self.position] };
            },
            '(' => {
                self.position += 1;
                return Token{ .type = .lparen, .value = self.input[self.position - 1 .. self.position] };
            },
            ')' => {
                self.position += 1;
                return Token{ .type = .rparen, .value = self.input[self.position - 1 .. self.position] };
            },
            else => {
                if (ascii.isDigit(ch) or ch == '.') {
                    const start = self.position;
                    while (self.position < self.input.len and
                        (ascii.isDigit(self.input[self.position]) or self.input[self.position] == '.'))
                    {
                        self.position += 1;
                    }
                    return Token{ .type = .number, .value = self.input[start..self.position] };
                } else {
                    return error.InvalidCharacter;
                }
            },
        }
    }
};

const EvalError = error{
    DivisionByZero,
    InvalidOperator,
};

const Node = union(enum) {
    number: f64,
    binary_op: *BinaryOpNode,
    unary_op: *UnaryOpNode,

    fn eval(self: Node) EvalError!f64 {
        return switch (self) {
            .number => |val| val,
            .binary_op => |node| node.eval(),
            .unary_op => |node| node.eval(),
        };
    }
};

const BinaryOpNode = struct {
    left: Node,
    op: TokenType,
    right: Node,

    fn eval(self: *const BinaryOpNode) EvalError!f64 {
        const left_val = try self.left.eval();
        const right_val = try self.right.eval();

        return switch (self.op) {
            .plus => left_val + right_val,
            .minus => left_val - right_val,
            .multiply => left_val * right_val,
            .divide => if (right_val == 0) error.DivisionByZero else left_val / right_val,
            else => error.InvalidOperator,
        };
    }
};

const UnaryOpNode = struct {
    op: TokenType,
    right: Node,

    fn eval(self: *const UnaryOpNode) EvalError!f64 {
        const right_val = try self.right.eval();
        return switch (self.op) {
            .minus => -right_val,
            else => error.InvalidOperator,
        };
    }
};

const ParserError = error{
    UnexpectedToken,
    MissingClosingParenthesis,
    UnexpectedTokenAtEnd,
    InvalidNumber,
    AllocationFailed,
    InvalidCharacter,
};

const Parser = struct {
    lexer: Lexer,
    current_token: Token,

    fn init(lexer: Lexer) ParserError!Parser {
        var parser = Parser{
            .lexer = lexer,
            .current_token = undefined,
        };
        parser.current_token = try parser.lexer.nextToken();
        return parser;
    }

    fn advance(self: *Parser) ParserError!void {
        self.current_token = try self.lexer.nextToken();
    }

    fn parseExpression(self: *Parser) ParserError!Node {
        return try self.parseAddSub();
    }

    fn parseAddSub(self: *Parser) ParserError!Node {
        var node = try self.parseMulDiv();

        while (self.current_token.type == .plus or self.current_token.type == .minus) {
            const op = self.current_token.type;
            try self.advance();
            const right = try self.parseMulDiv();
            const binary_node = std.heap.page_allocator.create(BinaryOpNode) catch return error.AllocationFailed;
            binary_node.* = BinaryOpNode{
                .left = node,
                .op = op,
                .right = right,
            };
            node = Node{ .binary_op = binary_node };
        }

        return node;
    }

    fn parseMulDiv(self: *Parser) ParserError!Node {
        var node = try self.parseUnary();

        while (self.current_token.type == .multiply or self.current_token.type == .divide) {
            const op = self.current_token.type;
            try self.advance();
            const right = try self.parseUnary();
            const binary_node = std.heap.page_allocator.create(BinaryOpNode) catch return error.AllocationFailed;
            binary_node.* = BinaryOpNode{
                .left = node,
                .op = op,
                .right = right,
            };
            node = Node{ .binary_op = binary_node };
        }

        return node;
    }

    fn parseUnary(self: *Parser) ParserError!Node {
        if (self.current_token.type == .minus) {
            const op = self.current_token.type;
            try self.advance();
            const right = try self.parsePrimary();
            const unary_node = std.heap.page_allocator.create(UnaryOpNode) catch return error.AllocationFailed;
            unary_node.* = UnaryOpNode{
                .op = op,
                .right = right,
            };
            return Node{ .unary_op = unary_node };
        }
        return try self.parsePrimary();
    }

    fn parsePrimary(self: *Parser) ParserError!Node {
        switch (self.current_token.type) {
            .number => {
                const val = fmt.parseFloat(f64, self.current_token.value) catch return error.InvalidNumber;
                try self.advance();
                return Node{ .number = val };
            },
            .lparen => {
                try self.advance();
                const node = try self.parseExpression();
                if (self.current_token.type != .rparen) {
                    return error.MissingClosingParenthesis;
                }
                try self.advance();
                return node;
            },
            else => return error.UnexpectedToken,
        }
    }
};

pub fn main() !void {
    const stdin = io.getStdIn().reader();
    const stdout = io.getStdOut().writer();

    var buffer: [1024]u8 = undefined;
    try stdout.print("in> ", .{});
    const input = try stdin.readUntilDelimiterOrEof(&buffer, '\n') orelse {
        std.process.exit(0);
    };

    const trimmed = mem.trim(u8, input, " \t\r\n");
    if (mem.eql(u8, trimmed, "exit")) {
        std.process.exit(0);
    }

    if (trimmed.len == 0) {
        std.process.exit(0);
    }

    const lexer = Lexer.init(trimmed);
    var parser = try Parser.init(lexer);

    const result = blk: {
        const node = try parser.parseExpression();
        if (parser.current_token.type != .eof) {
            return error.UnexpectedTokenAtEnd;
        }
        break :blk try node.eval();
    };

    try stdout.print("out> {d}\n", .{result});
}
