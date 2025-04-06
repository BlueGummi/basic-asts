module Main where

import Text.Parsec
import Text.Parsec.Expr
import Text.Parsec.String (Parser)
import Text.Parsec.Language (emptyDef)
import qualified Text.Parsec.Token as Token
import Control.Monad (void)
import Control.Monad.Identity (Identity)
import System.IO (hFlush, stdout)

data Expr
  = Lit Double
  | Add Expr Expr
  | Sub Expr Expr
  | Mul Expr Expr
  | Div Expr Expr 
  | Neg Expr
  | Pow Expr Expr
  deriving (Show)

lexer :: Token.TokenParser ()
lexer = Token.makeTokenParser style
  where 
    ops = ["+","-","*","/","^"]
    names = []
    style = emptyDef {
      Token.reservedOpNames = ops,
      Token.reservedNames = names,
      Token.identStart = letter,
      Token.identLetter = alphaNum
    }

integer :: Parser Double
integer = Token.integer lexer >>= return . fromIntegral

float :: Parser Double
float = Token.float lexer

parens :: Parser a -> Parser a
parens = Token.parens lexer

reservedOp :: String -> Parser ()
reservedOp = Token.reservedOp lexer

whiteSpace :: Parser ()
whiteSpace = Token.whiteSpace lexer

expr :: Parser Expr
expr = buildExpressionParser table term <?> "expression"

table :: [[Operator String () Identity Expr]]
table = 
  [ [prefix "-" Neg, prefix "+" id]
  , [binary "^" Pow AssocRight]
  , [binary "*" Mul AssocLeft, binary "/" Div AssocLeft]
  , [binary "+" Add AssocLeft, binary "-" Sub AssocLeft]
  ]
  where
    binary name fun assoc = Infix (reservedOp name >> return fun) assoc
    prefix name fun = Prefix (reservedOp name >> return fun)

term :: Parser Expr
term = 
  parens expr 
  <|> (Lit <$> try float) 
  <|> (Lit <$> integer)
  <?> "simple expression"

eval :: Expr -> Double
eval (Lit x) = x
eval (Add a b) = eval a + eval b
eval (Sub a b) = eval a - eval b
eval (Mul a b) = eval a * eval b
eval (Div a b) = eval a / eval b
eval (Neg a) = negate (eval a)
eval (Pow a b) = eval a ** eval b

parseExpr :: String -> Either ParseError Expr
parseExpr = parse (whiteSpace >> expr <* eof) "<input>"

main :: IO ()
main = do
  putStr "in> "
  hFlush stdout
  input <- getLine
  case parseExpr input of
    Left err -> putStrLn $ "err> " ++ show err
    Right ast -> do
      putStrLn $ "ast> " ++ show ast
      putStrLn $ "out> " ++ show (eval ast)
