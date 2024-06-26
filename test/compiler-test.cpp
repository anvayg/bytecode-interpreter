#include <memory>
#include <utility>
#define BOOST_TEST_MODULE Compiler Tests

#include <boost/test/included/unit_test.hpp>

#include "../include/lexer.hpp"
#include "../include/ast.hpp"
#include "../include/interpreter.hpp"

#include <vector>

using Code = std::vector<Instruction>;

std::ostream &operator<<(std::ostream &os,
                         const std::vector<Instruction> &vec) {
  os << "[ ";
  for (const auto &inst : vec) {
    os << inst << ",\n";
  }
  os << "]";
  return os;
}

BOOST_AUTO_TEST_CASE(lex_val) {
    std::string text = "(val 5)";
    Lexer lexer(text);
    lexer.lex();
    BOOST_TEST(!lexer.lexError());

    text = "(val \"x\")";
    Lexer lexer2(text);
    BOOST_TEST(!lexer2.lexError());
}

BOOST_AUTO_TEST_CASE(lex_lambda) {
    std::string text = "(lambda x (+ x 1))";
    Lexer lexer(text);
    lexer.lex();
    BOOST_TEST(!lexer.lexError());
}

BOOST_AUTO_TEST_CASE(lex_with_newline) {
    std::string text = "(val 5)\n(lambda x (+ x 1))";
    Lexer lexer(text);
    lexer.lex();
    BOOST_TEST(!lexer.lexError());
}

BOOST_AUTO_TEST_CASE(lex_unterminated) {
    std::string text = "(val \"x";
    Lexer lexer(text);
    lexer.lex();
    BOOST_TEST(lexer.lexError());
}

BOOST_AUTO_TEST_CASE(compile_int) {
  Constant c(5);

  Code bytecode = interpreter::compile(c);
  Instruction i = bytecode[0];
  BOOST_TEST(i == Instruction(OpCode::LOAD_CONST, 5));
}

BOOST_AUTO_TEST_CASE(compile_and_eval_val) {
  std::unique_ptr<Expression> expr1 = std::make_unique<StringConstant>("val");
  std::unique_ptr<Expression> expr2 = std::make_unique<StringConstant>("x");
  std::unique_ptr<Expression> expr3 = std::make_unique<Constant>(5);

  std::vector<std::unique_ptr<Expression>> exps;
  exps.push_back(std::move(expr1));
  exps.push_back(std::move(expr2));
  exps.push_back(std::move(expr3));
  ExpressionList l(std::move(exps));

  // Test compilation
  Code bytecode = interpreter::compile(l);
  BOOST_TEST(bytecode.size() == 2);

  Instruction i1 = bytecode[0];
  BOOST_TEST(i1 == Instruction(OpCode::LOAD_CONST, 5));

  Instruction i2 = bytecode[1];
  BOOST_TEST(i2 == Instruction(OpCode::STORE_NAME, "x"));

  // Test evaluation
  Environment env = Environment();
  auto result = interpreter::eval(bytecode, env);
  BOOST_TEST(boost::get<int>(env.lookup("x")) == 5);
}

BOOST_AUTO_TEST_CASE(compile_and_eval_conditions) {
  std::unique_ptr<Expression> expr1 = std::make_unique<StringConstant>("if");
  std::unique_ptr<Expression> expr2 = std::make_unique<Constant>(1);
  std::unique_ptr<Expression> expr3 = std::make_unique<Constant>(2);
  std::unique_ptr<Expression> expr4 = std::make_unique<Constant>(3);

  std::vector<std::unique_ptr<Expression>> exps;
  exps.push_back(std::move(expr1));
  exps.push_back(std::move(expr2));
  exps.push_back(std::move(expr3));
  exps.push_back(std::move(expr4));
  ExpressionList l(std::move(exps));

  // Test compilation
  Code bytecode = interpreter::compile(l);
  BOOST_TEST(bytecode.size() == 5);

  Instruction i0 = bytecode[0];
  BOOST_TEST(i0 == Instruction(OpCode::LOAD_CONST, 1));

  Instruction i1 = bytecode[1];
  BOOST_TEST(i1 == Instruction(OpCode::RELATIVE_JUMP_IF_TRUE, 2));

  Instruction i2 = bytecode[2];
  BOOST_TEST(i2 == Instruction(OpCode::LOAD_CONST, 3));

  Instruction i3 = bytecode[3];
  BOOST_TEST(i3 == Instruction(OpCode::RELATIVE_JUMP, 1));

  Instruction i4 = bytecode[4];
  BOOST_TEST(i4 == Instruction(OpCode::LOAD_CONST, 2));

  // Test evaluation
  Environment env = Environment();
  auto result = interpreter::eval(bytecode, env);
  BOOST_TEST(boost::get<int>(result) == 2);
}

BOOST_AUTO_TEST_CASE(compile_and_eval_conditions_with_vars) {
  std::unique_ptr<Expression> expr1 = std::make_unique<StringConstant>("if");
  std::unique_ptr<Expression> expr2 = std::make_unique<StringConstant>("cond");
  std::unique_ptr<Expression> expr3 = std::make_unique<Constant>(2);
  std::unique_ptr<Expression> expr4 = std::make_unique<Constant>(3);

  std::vector<std::unique_ptr<Expression>> exps;
  exps.push_back(std::move(expr1));
  exps.push_back(std::move(expr2));
  exps.push_back(std::move(expr3));
  exps.push_back(std::move(expr4));
  ExpressionList l(std::move(exps));

  Code bytecode = interpreter::compile(l);
  Environment env = Environment();
  env.define("cond", 1);
  auto result = interpreter::eval(bytecode, env);
  BOOST_TEST(boost::get<int>(result) == 2);

  env = Environment();
  env.define("cond", 0);
  result = interpreter::eval(bytecode, env);
  BOOST_TEST(boost::get<int>(result) == 3);
}

BOOST_AUTO_TEST_CASE(compile_arithmetic) {
  // Addition
  std::unique_ptr<Expression> expr1 = std::make_unique<Constant>(1);
  std::unique_ptr<Expression> expr2 = std::make_unique<Constant>(2);
  BinaryOperation binop_add('+', std::move(expr1), std::move(expr2));

  Code bytecode = interpreter::compile(binop_add);
  Environment env = Environment();
  auto result = interpreter::eval(bytecode, env);
  BOOST_TEST(boost::get<int>(result) == 3);

  // Subtraction
  bytecode.clear();
  std::unique_ptr<Expression> expr3 = std::make_unique<Constant>(1);
  std::unique_ptr<Expression> expr4 = std::make_unique<Constant>(2);
  BinaryOperation binop_sub('-', std::move(expr3), std::move(expr4));
  bytecode = interpreter::compile(binop_sub);
  result = interpreter::eval(bytecode, env);
  BOOST_TEST(boost::get<int>(result) == -1);

  // Nested binop
  bytecode.clear();
  std::unique_ptr<Expression> expr5 = std::make_unique<Constant>(1);
  std::unique_ptr<Expression> expr6 = std::make_unique<Constant>(2);
  std::unique_ptr<Expression> expr7 = std::make_unique<Constant>(3);
  BinaryOperation binop_add_2('+', std::move(expr5), std::move(expr6));
  std::unique_ptr<Expression> expr8 =
      std::make_unique<BinaryOperation>(std::move(binop_add_2));
  BinaryOperation nested_binop('-', std::move(expr8), std::move(expr7));
  bytecode = interpreter::compile(nested_binop);
  result = interpreter::eval(bytecode, env);
  BOOST_TEST(boost::get<int>(result) == 0);
}

BOOST_AUTO_TEST_CASE(compile_function) {
  // params
  StringConstant param("x");
  std::vector<StringConstant> params;
  params.push_back(param);

  // body
  std::unique_ptr<Expression> expr1 = std::make_unique<StringConstant>("x");
  std::unique_ptr<Expression> expr2 = std::make_unique<Constant>(1);
  BinaryOperation binop_add('+', std::move(expr1), std::move(expr2));
  std::unique_ptr<Expression> binop_add_exp =
      std::make_unique<BinaryOperation>(std::move(binop_add));

  Lambda f(params, std::move(binop_add_exp));

  Code f_code = interpreter::compile(f);

  // TODO: Add tests for instructions
}

BOOST_AUTO_TEST_CASE(compile_and_eval_function_call) {
  // params
  StringConstant param("x");
  std::vector<StringConstant> params;
  params.push_back(param);

  // body
  std::unique_ptr<Expression> expr1 = std::make_unique<StringConstant>("x");
  std::unique_ptr<Expression> expr2 = std::make_unique<Constant>(1);
  BinaryOperation binop_add('+', std::move(expr1), std::move(expr2));
  std::unique_ptr<Expression> binop_add_exp =
      std::make_unique<BinaryOperation>(std::move(binop_add));

  Lambda f(params, std::move(binop_add_exp));

  // arg
  std::unique_ptr<Expression> arg = std::make_unique<Constant>(1);

  // Make call expression
  std::vector<std::unique_ptr<Expression>> call_exps;
  call_exps.push_back(std::make_unique<Lambda>(std::move(f)));
  call_exps.push_back(std::move(arg));
  ExpressionList call(std::move(call_exps));

  Code bytecode = interpreter::compile(call);
  Environment env = Environment();
  auto result = interpreter::eval(bytecode, env);
  BOOST_TEST(boost::get<int>(result) == 2);
}