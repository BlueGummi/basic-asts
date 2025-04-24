package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
	"unicode"
)

const (
	TokenEOF      = "EOF"
	TokenNumber   = "NUMBER"
	TokenPlus     = "+"
	TokenMinus    = "-"
	TokenMultiply = "*"
	TokenDivide   = "/"
	TokenLParen   = "("
	TokenRParen   = ")"
)

type Token struct {
	Type  string
	Value string
}

type Lexer struct {
	input        string
	position     int
	readPosition int
	ch           rune
}

func NewLexer(input string) *Lexer {
	l := &Lexer{input: input}
	l.readChar()
	return l
}

func (l *Lexer) readChar() {
	if l.readPosition >= len(l.input) {
		l.ch = 0
	} else {
		l.ch = rune(l.input[l.readPosition])
	}
	l.position = l.readPosition
	l.readPosition++
}

func (l *Lexer) skipWhitespace() {
	for unicode.IsSpace(l.ch) {
		l.readChar()
	}
}

func (l *Lexer) NextToken() Token {
	var tok Token

	l.skipWhitespace()

	switch l.ch {
	case '+':
		tok = Token{Type: TokenPlus, Value: string(l.ch)}
	case '-':
		tok = Token{Type: TokenMinus, Value: string(l.ch)}
	case '*':
		tok = Token{Type: TokenMultiply, Value: string(l.ch)}
	case '/':
		tok = Token{Type: TokenDivide, Value: string(l.ch)}
	case '(':
		tok = Token{Type: TokenLParen, Value: string(l.ch)}
	case ')':
		tok = Token{Type: TokenRParen, Value: string(l.ch)}
	case 0:
		tok = Token{Type: TokenEOF, Value: ""}
	default:
		if unicode.IsDigit(l.ch) || l.ch == '.' {
			tok.Type = TokenNumber
			tok.Value = l.readNumber()
			return tok
		} else {
			tok = Token{Type: TokenEOF, Value: string(l.ch)}
		}
	}

	l.readChar()
	return tok
}

func (l *Lexer) readNumber() string {
	startPos := l.position
	for unicode.IsDigit(l.ch) || l.ch == '.' {
		l.readChar()
	}
	return l.input[startPos:l.position]
}

type Node interface {
	Eval() float64
}

type NumberNode struct {
	Value float64
}

func (n *NumberNode) Eval() float64 {
	return n.Value
}

type BinaryOpNode struct {
	Left     Node
	Operator string
	Right    Node
}

func (n *BinaryOpNode) Eval() float64 {
	left := n.Left.Eval()
	right := n.Right.Eval()

	switch n.Operator {
	case TokenPlus:
		return left + right
	case TokenMinus:
		return left - right
	case TokenMultiply:
		return left * right
	case TokenDivide:
		return left / right
	default:
		panic(fmt.Sprintf("unknown operator: %s", n.Operator))
	}
}

type UnaryOpNode struct {
	Operator string
	Right    Node
}

func (n *UnaryOpNode) Eval() float64 {
	right := n.Right.Eval()
	if n.Operator == TokenMinus {
		return -right
	}
	return right
}

type Parser struct {
	lexer  *Lexer
	curTok Token
}

func NewParser(lexer *Lexer) *Parser {
	p := &Parser{lexer: lexer}
	p.advance()
	return p
}

func (p *Parser) advance() {
	p.curTok = p.lexer.NextToken()
}

func (p *Parser) parseExpression() Node {
	return p.parseAddSub()
}

func (p *Parser) parseAddSub() Node {
	node := p.parseMulDiv()

	for p.curTok.Type == TokenPlus || p.curTok.Type == TokenMinus {
		op := p.curTok.Type
		p.advance()
		node = &BinaryOpNode{Left: node, Operator: op, Right: p.parseMulDiv()}
	}

	return node
}

func (p *Parser) parseMulDiv() Node {
	node := p.parseUnary()

	for p.curTok.Type == TokenMultiply || p.curTok.Type == TokenDivide {
		op := p.curTok.Type
		p.advance()
		node = &BinaryOpNode{Left: node, Operator: op, Right: p.parseUnary()}
	}

	return node
}

func (p *Parser) parseUnary() Node {
	if p.curTok.Type == TokenMinus {
		op := p.curTok.Type
		p.advance()
		return &UnaryOpNode{Operator: op, Right: p.parsePrimary()}
	}
	return p.parsePrimary()
}

func (p *Parser) parsePrimary() Node {
	switch p.curTok.Type {
	case TokenNumber:
		val, err := strconv.ParseFloat(p.curTok.Value, 64)
		if err != nil {
			panic(err)
		}
		node := &NumberNode{Value: val}
		p.advance()
		return node
	case TokenLParen:
		p.advance()
		node := p.parseExpression()
		if p.curTok.Type != TokenRParen {
			panic("expected ')'")
		}
		p.advance()
		return node
	default:
		panic(fmt.Sprintf("unexpected token: %v", p.curTok))
	}
}

func main() {
	reader := bufio.NewReader(os.Stdin)
	fmt.Print("in> ")
	input, err := reader.ReadString('\n')
	if err != nil {
		fmt.Println("err> ", err)
	}

	input = strings.TrimSpace(input)
	if input == "" {
		os.Exit(0)
	}

	lexer := NewLexer(input)
	parser := NewParser(lexer)

	ast, err := func() (result float64, err error) {
		defer func() {
			if r := recover(); r != nil {
				err = fmt.Errorf("%v", r)
			}
		}()

		node := parser.parseExpression()
		if parser.curTok.Type != TokenEOF {
			panic("err> unexpected token at end of input")
		}
		return node.Eval(), nil
	}()

	if err != nil {
		fmt.Println("err> ", err)
	} else {
		fmt.Println("out> ", ast)
	}
}
