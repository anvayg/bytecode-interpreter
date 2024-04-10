#include "../include/interpreter.hpp"
#include "../include/ast.hpp"
#include "../include/environment.hpp"
#include "../include/instruction.hpp"
#include <stack>
#include <stdexcept>
#include <variant>
#include <vector>

interpreter::Code interpreter::compile(Expression &e) {
  Compiler compiler;
  return e.accept(compiler);
}

std::vector<Instruction> Compiler::visit(Constant &constant) {
  std::vector<Instruction> ins;
  ins.push_back(Instruction(OpCode::LOAD_CONST, constant.getValue()));
  return ins;
}

std::vector<Instruction> Compiler::visit(StringConstant &constant) {
  std::vector<Instruction> ins;
  ins.push_back(Instruction(OpCode::LOAD_NAME, constant.getValue()));
  return ins;
}

std::vector<Instruction> Compiler::visit(BinaryOperation &binOp) {
  std::vector<Instruction> ins;

  return ins;
}

std::vector<Instruction> Compiler::visit(ExpressionList &list) {
  std::vector<Instruction> ins;
  const auto &exps = list.getExpressions();

  if (exps.size() == 3) {
    auto &first = exps[0];
    const StringConstant *strConstPtr =
        dynamic_cast<const StringConstant *>(first.get());
    if (strConstPtr && strConstPtr->getValue() == "val") {
      auto &name = exps[1];
      const StringConstant *strConstPtr =
          dynamic_cast<const StringConstant *>(name.get());

      if (strConstPtr) {
        Instruction store(OpCode::STORE_NAME, strConstPtr->getValue());

        auto &subexp = exps[2];
        std::vector<Instruction> subexp_code = subexp.get()->accept(*this);

        ins.insert(ins.end(), subexp_code.begin(), subexp_code.end());
        ins.push_back(store);
      } else {
        throw std::runtime_error("Unsupported instruction");
      }

    } else {
      throw std::runtime_error("Unsupported instruction");
    }

  } else if (exps.size() == 4) {
    auto &first = exps[0];
    const StringConstant *strConstPtr =
        dynamic_cast<const StringConstant *>(first.get());
    if (strConstPtr && strConstPtr->getValue() == "if") {
      // Compile condition, true and false branches
      auto &cond = exps[1];
      std::vector<Instruction> cond_code = cond.get()->accept(*this);

      auto &true_branch = exps[2];
      std::vector<Instruction> true_code = true_branch.get()->accept(*this);

      auto &false_branch = exps[3];
      std::vector<Instruction> false_code = false_branch.get()->accept(*this);

      // Construct relative jumps
      Instruction jmp_to_end(OpCode::RELATIVE_JUMP,
                             static_cast<int>(true_code.size()));
      Instruction jmp_to_true(OpCode::RELATIVE_JUMP_IF_TRUE,
                              static_cast<int>(false_code.size()) + 1);

      // Put instructions together
      ins.insert(ins.end(), cond_code.begin(), cond_code.end());
      ins.push_back(jmp_to_true);
      ins.insert(ins.end(), false_code.begin(), false_code.end());
      ins.push_back(jmp_to_end);
      ins.insert(ins.end(), true_code.begin(), true_code.end());
    } else {
      throw std::runtime_error("Unsupported instruction");
    }

  } else {
    throw std::runtime_error("Unsupported instruction");
  }

  return ins;
}

interpreter::Code interpreter::compile(std::vector<ValueType> exp) {
  Code ins;

  if (exp.size() == 0) {
    return ins;
  }
  if (exp.size() == 1) {
    auto subexp = exp[0];
    // Check if the variant holds an int
    if (std::holds_alternative<int>(subexp)) {
      ins.push_back(Instruction(OpCode::LOAD_CONST, std::get<int>(subexp)));
    } else {
      ins.push_back(
          Instruction(OpCode::LOAD_NAME, std::get<std::string>(subexp)));
    }
  } else if (exp.size() == 3) {
    // [val, x, 5]
    auto first_exp = exp[0];
    if (std::holds_alternative<std::string>(first_exp)) {
      std::string first = std::get<std::string>(first_exp);
      if (first == "val") {
        auto name = exp[1];
        Instruction store(OpCode::STORE_NAME, name);

        std::vector<ValueType> subexp(exp.begin() + 2, exp.end());
        Code subexp_code = compile(subexp);
        ins.insert(ins.end(), subexp_code.begin(), subexp_code.end());
        ins.push_back(store);
      } else
        throw std::runtime_error("Unsupported instruction");
    }
  } else if (exp.size() == 4) {
    auto first_exp = exp[0];
    if (std::holds_alternative<std::string>(first_exp)) {
      std::string first = std::get<std::string>(first_exp);
      if (first == "if") {
        // Compile condition, true and false branches
        std::vector<ValueType> cond(exp.begin() + 1, exp.begin() + 2);
        Code cond_code = compile(cond);

        std::vector<ValueType> true_exp(exp.begin() + 2, exp.begin() + 3);
        Code true_code = compile(true_exp);

        std::vector<ValueType> false_exp(exp.begin() + 3, exp.begin() + 4);
        Code false_code = compile(false_exp);

        // Construct relative jumps
        Instruction jmp_to_end(OpCode::RELATIVE_JUMP,
                               static_cast<int>(true_code.size()));
        Instruction jmp_to_true(OpCode::RELATIVE_JUMP_IF_TRUE,
                                static_cast<int>(false_code.size()) + 1);

        // Put instructions together
        ins.insert(ins.end(), cond_code.begin(), cond_code.end());
        ins.push_back(jmp_to_true);
        ins.insert(ins.end(), false_code.begin(), false_code.end());
        ins.push_back(jmp_to_end);
        ins.insert(ins.end(), true_code.begin(), true_code.end());

      } else
        throw std::runtime_error("Unsupported instruction");
    } else
      throw std::runtime_error("Unsupported instruction");
  } else
    throw std::runtime_error("Unsupported supported");

  return ins;
}

ValueType interpreter::eval(Code &bytecode, Environment &env) {
  int program_counter = 0;
  std::stack<ValueType> stack;

  while (program_counter < bytecode.size()) {
    Instruction ins = bytecode[program_counter];
    auto op = ins.opCode;
    program_counter++;

    if (op == OpCode::LOAD_CONST) {
      stack.push(ins.arg);
    } else if (op == OpCode::LOAD_NAME) {
      // Find name in environment and push corresponding value onto stack
      if (std::holds_alternative<std::string>(ins.arg)) {
        auto val = env.lookup(std::get<std::string>(ins.arg));
        stack.push(val);
      } else {
        throw std::runtime_error("Unsupported instruction");
      }
    } else if (op == OpCode::STORE_NAME) {
      // Get name from top of stack and add a binding for it in the
      // environment
      auto name = stack.top();
      stack.pop();

      if (std::holds_alternative<int>(name) &&
          std::holds_alternative<std::string>(ins.arg)) {
        env.define(std::get<std::string>(ins.arg), std::get<int>(name));
      } else {
        throw std::runtime_error("Unsupported instruction");
      }
    } else if (op == OpCode::RELATIVE_JUMP_IF_TRUE) {
      auto cond = stack.top();
      stack.pop();

      if (std::get<int>(cond))
        program_counter += std::get<int>(ins.arg);

    } else if (op == OpCode::RELATIVE_JUMP) {
      program_counter += std::get<int>(ins.arg);
    } else {
      throw std::runtime_error("Unsupported instruction");
    }
  }

  if (!stack.empty())
    return stack.top();
  else
    return -1;
}
