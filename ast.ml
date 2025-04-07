type token =
  | INT of int
  | FLOAT of float
  | PLUS
  | MINUS
  | TIMES
  | DIV
  | MOD
  | POWER
  | LPAREN
  | RPAREN
  | EOF

let is_digit c = c >= '0' && c <= '9'

let rec lexer (s : string) (pos : int) : token list =
  if pos >= String.length s then [EOF]
  else
    match s.[pos] with
    | ' ' | '\t' | '\n' -> lexer s (pos + 1)
    | '+' -> PLUS :: lexer s (pos + 1)
    | '-' -> MINUS :: lexer s (pos + 1)
    | '*' -> TIMES :: lexer s (pos + 1)
    | '/' -> DIV :: lexer s (pos + 1)
    | '%' -> MOD :: lexer s (pos + 1)
    | '^' -> POWER :: lexer s (pos + 1)
    | '(' -> LPAREN :: lexer s (pos + 1)
    | ')' -> RPAREN :: lexer s (pos + 1)
    | c when is_digit c ->
        let num_str, new_pos = read_number s pos in
        if String.contains num_str '.' then
          FLOAT (float_of_string num_str) :: lexer s new_pos
        else
          INT (int_of_string num_str) :: lexer s new_pos
    | _ -> failwith ("unexpected character " ^ String.make 1 s.[pos])

and read_number s pos =
  let len = String.length s in
  let rec loop pos acc has_decimal =
    if pos >= len then (acc, pos)
    else
      match s.[pos] with
      | c when is_digit c -> loop (pos + 1) (acc ^ String.make 1 c) has_decimal
      | '.' ->
          if has_decimal then failwith "invalid number with multiple decimal points"
          else loop (pos + 1) (acc ^ ".") true
      | _ -> (acc, pos)
  in
  loop pos "" false

type expr =
  | Int of int
  | Float of float
  | Binop of binop * expr * expr
  | Unop of unop * expr

and binop =
  | Add
  | Sub
  | Mul
  | Div
  | Mod
  | Pow

and unop =
  | Plus
  | Minus

let parse tokens =
  let rec parse_expr tokens =
    parse_add_sub tokens
  
  and parse_add_sub tokens =
    let left, tokens = parse_mul_div tokens in
    parse_add_sub' left tokens
  
  and parse_add_sub' left tokens =
    match tokens with
    | PLUS :: rest ->
        let right, rest = parse_mul_div rest in
        parse_add_sub' (Binop (Add, left, right)) rest
    | MINUS :: rest ->
        let right, rest = parse_mul_div rest in
        parse_add_sub' (Binop (Sub, left, right)) rest
    | _ -> (left, tokens)
  
  and parse_mul_div tokens =
    let left, tokens = parse_power tokens in
    parse_mul_div' left tokens
  
  and parse_mul_div' left tokens =
    match tokens with
    | TIMES :: rest ->
        let right, rest = parse_power rest in
        parse_mul_div' (Binop (Mul, left, right)) rest
    | DIV :: rest ->
        let right, rest = parse_power rest in
        parse_mul_div' (Binop (Div, left, right)) rest
    | MOD :: rest ->
        let right, rest = parse_power rest in
        parse_mul_div' (Binop (Mod, left, right)) rest
    | _ -> (left, tokens)
  
  and parse_power tokens =
    let left, tokens = parse_unary tokens in
    parse_power' left tokens
  
  and parse_power' left tokens =
    match tokens with
    | POWER :: rest ->
        let right, rest = parse_unary rest in
        (Binop (Pow, left, right)), rest
    | _ -> (left, tokens)
  
  and parse_unary tokens =
    match tokens with
    | PLUS :: rest ->
        let expr, rest = parse_primary rest in
        (Unop (Plus, expr)), rest
    | MINUS :: rest ->
        let expr, rest = parse_primary rest in
        (Unop (Minus, expr)), rest
    | _ -> parse_primary tokens
  
  and parse_primary tokens =
    match tokens with
    | INT n :: rest -> (Int n, rest)
    | FLOAT f :: rest -> (Float f, rest)
    | LPAREN :: rest ->
        let expr, rest = parse_expr rest in
        (match rest with
         | RPAREN :: rest' -> (expr, rest')
         | _ -> failwith "missing closing parenthesis")
    | _ -> failwith "unexpected token in primary expression"
  
  in
  let expr, rest = parse_expr tokens in
  match rest with
  | [EOF] -> expr
  | _ -> failwith "unexpected tokens at end of expression"

let rec eval = function
  | Int n -> float_of_int n
  | Float f -> f
  | Binop (op, e1, e2) ->
      let v1 = eval e1 in
      let v2 = eval e2 in
      (match op with
       | Add -> v1 +. v2
       | Sub -> v1 -. v2
       | Mul -> v1 *. v2
       | Div -> v1 /. v2
       | Mod -> mod_float v1 v2
       | Pow -> v1 ** v2)
  | Unop (op, e) ->
      let v = eval e in
      (match op with
       | Plus -> v
       | Minus -> -.v)

let evaluate_expression input =
  try
    let tokens = lexer input 0 in
    let ast = parse tokens in
    let result = eval ast in
    Printf.printf "out> %g\n" result
  with
  | Failure msg -> Printf.eprintf "err> %s\n" msg
  | _ -> Printf.eprintf "err> ?\n"

let () =
  print_string "in> ";
  flush stdout;
  match read_line () with
  | input ->
      evaluate_expression input;
