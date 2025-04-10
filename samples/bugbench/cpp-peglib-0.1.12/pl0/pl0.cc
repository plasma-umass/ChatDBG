//
//  pl0.cc - PL/0 language (https://en.wikipedia.org/wiki/PL/0)
//
//  Copyright (c) 2020 Yuji Hirose. All rights reserved.
//  MIT License
//

#include <peglib.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"

using namespace peg;
using namespace peg::udl;
using namespace llvm;
using namespace std;

/*
 * PEG Grammar
 */
auto grammar = R"(
  program    <- _ block '.' _

  block      <- const var procedure statement
  const      <- ('CONST' __ ident '=' _ number (',' _ ident '=' _ number)* ';' _)?
  var        <- ('VAR' __ ident (',' _ ident)* ';' _)?
  procedure  <- ('PROCEDURE' __ ident ';' _ block ';' _)*

  statement  <- (assignment / call / statements / if / while / out / in)?
  assignment <- ident ':=' _ expression
  call       <- 'CALL' __ ident
  statements <- 'BEGIN' __ statement (';' _ statement )* 'END' __
  if         <- 'IF' __ condition 'THEN' __ statement
  while      <- 'WHILE' __ condition 'DO' __ statement
  out        <- ('out' __ / 'write' __ / '!' _) expression
  in         <- ('in' __ / 'read' __ / '?' _) ident

  condition  <- odd / compare
  odd        <- 'ODD' __ expression
  compare    <- expression compare_op expression
  compare_op <- < '=' / '#' / '<=' / '<' / '>=' / '>' > _

  expression <- sign term (term_op term)*
  sign       <- < [-+]? > _
  term_op    <- < [-+] > _

  term       <- factor (factor_op factor)*
  factor_op  <- < [*/] > _

  factor     <- ident / number / '(' _ expression ')' _

  ident      <- < [a-z] [a-z0-9]* > _
  number     <- < [0-9]+ > _

  ~_         <- [ \t\r\n]*
  ~__        <- ![a-z0-9_] _
)";

/*
 * Utilities
 */
string format_error_message(const string& path, size_t ln, size_t col,
                            const string& msg) {
  stringstream ss;
  ss << path << ":" << ln << ":" << col << ": " << msg << endl;
  return ss.str();
}

/*
 * Ast
 */
struct SymbolScope;

struct Annotation {
  shared_ptr<SymbolScope> scope;
};

typedef AstBase<Annotation> AstPL0;
shared_ptr<SymbolScope> get_closest_scope(shared_ptr<AstPL0> ast) {
  ast = ast->parent.lock();
  while (ast->tag != "block"_) {
    ast = ast->parent.lock();
  }
  return ast->scope;
}

/*
 * Symbol Table
 */
struct SymbolScope {
  SymbolScope(shared_ptr<SymbolScope> outer) : outer(outer) {}

  bool has_symbol(const string& ident, bool extend = true) const {
    auto ret = constants.count(ident) || variables.count(ident);
    return ret ? true : (extend && outer ? outer->has_symbol(ident) : false);
  }

  bool has_constant(const string& ident, bool extend = true) const {
    return constants.count(ident)
               ? true
               : (extend && outer ? outer->has_constant(ident) : false);
  }

  bool has_variable(const string& ident, bool extend = true) const {
    return variables.count(ident)
               ? true
               : (extend && outer ? outer->has_variable(ident) : false);
  }

  bool has_procedure(const string& ident, bool extend = true) const {
    return procedures.count(ident)
               ? true
               : (extend && outer ? outer->has_procedure(ident) : false);
  }

  shared_ptr<AstPL0> get_procedure(const string& ident) const {
    auto it = procedures.find(ident);
    return it != procedures.end() ? it->second : outer->get_procedure(ident);
  }

  map<string, int> constants;
  set<string> variables;
  map<string, shared_ptr<AstPL0>> procedures;
  set<string> free_variables;

 private:
  shared_ptr<SymbolScope> outer;
};

void throw_runtime_error(const shared_ptr<AstPL0> node, const string& msg) {
  throw runtime_error(
      format_error_message(node->path, node->line, node->column, msg));
}

struct SymbolTable {
  static void build_on_ast(const shared_ptr<AstPL0> ast,
                           shared_ptr<SymbolScope> scope = nullptr) {
    switch (ast->tag) {
      case "block"_:
        block(ast, scope);
        break;
      case "assignment"_:
        assignment(ast, scope);
        break;
      case "call"_:
        call(ast, scope);
        break;
      case "ident"_:
        ident(ast, scope);
        break;
      default:
        for (auto node : ast->nodes) {
          build_on_ast(node, scope);
        }
        break;
    }
  }

 private:
  static void block(const shared_ptr<AstPL0> ast,
                    shared_ptr<SymbolScope> outer) {
    // block <- const var procedure statement
    auto scope = make_shared<SymbolScope>(outer);
    const auto& nodes = ast->nodes;
    constants(nodes[0], scope);
    variables(nodes[1], scope);
    procedures(nodes[2], scope);
    build_on_ast(nodes[3], scope);
    ast->scope = scope;
  }

  static void constants(const shared_ptr<AstPL0> ast,
                        shared_ptr<SymbolScope> scope) {
    // const <- ('CONST' __ ident '=' _ number(',' _ ident '=' _ number)* ';'
    // _)?
    const auto& nodes = ast->nodes;
    for (auto i = 0u; i < nodes.size(); i += 2) {
      const auto& ident = nodes[i + 0]->token;
      if (scope->has_symbol(ident)) {
        throw_runtime_error(nodes[i], "'" + ident + "' is already defined...");
      }
      auto number = stoi(nodes[i + 1]->token);
      scope->constants.emplace(ident, number);
    }
  }

  static void variables(const shared_ptr<AstPL0> ast,
                        shared_ptr<SymbolScope> scope) {
    // var <- ('VAR' __ ident(',' _ ident)* ';' _) ?
    const auto& nodes = ast->nodes;
    for (auto i = 0u; i < nodes.size(); i += 1) {
      const auto& ident = nodes[i]->token;
      if (scope->has_symbol(ident)) {
        throw_runtime_error(nodes[i], "'" + ident + "' is already defined...");
      }
      scope->variables.emplace(ident);
    }
  }

  static void procedures(const shared_ptr<AstPL0> ast,
                         shared_ptr<SymbolScope> scope) {
    // procedure <- ('PROCEDURE' __ ident ';' _ block ';' _)*
    const auto& nodes = ast->nodes;
    for (auto i = 0u; i < nodes.size(); i += 2) {
      const auto& ident = nodes[i + 0]->token;
      auto block = nodes[i + 1];
      scope->procedures[ident] = block;
      build_on_ast(block, scope);
    }
  }

  static void assignment(const shared_ptr<AstPL0> ast,
                         shared_ptr<SymbolScope> scope) {
    // assignment <- ident ':=' _ expression
    const auto& ident = ast->nodes[0]->token;
    if (scope->has_constant(ident)) {
      throw_runtime_error(ast->nodes[0],
                          "cannot modify constant value '" + ident + "'...");
    } else if (!scope->has_variable(ident)) {
      throw_runtime_error(ast->nodes[0],
                          "undefined variable '" + ident + "'...");
    }

    build_on_ast(ast->nodes[1], scope);

    if (!scope->has_symbol(ident, false)) {
      scope->free_variables.emplace(ident);
    }
  }

  static void call(const shared_ptr<AstPL0> ast,
                   shared_ptr<SymbolScope> scope) {
    // call <- 'CALL' __ ident
    const auto& ident = ast->nodes[0]->token;
    if (!scope->has_procedure(ident)) {
      throw_runtime_error(ast->nodes[0],
                          "undefined procedure '" + ident + "'...");
    }

    auto block = scope->get_procedure(ident);
    if (block->scope) {
      for (const auto& free : block->scope->free_variables) {
        if (!scope->has_symbol(free, false)) {
          scope->free_variables.emplace(free);
        }
      }
    }
  }

  static void ident(const shared_ptr<AstPL0> ast,
                    shared_ptr<SymbolScope> scope) {
    const auto& ident = ast->token;
    if (!scope->has_symbol(ident)) {
      throw_runtime_error(ast, "undefined variable '" + ident + "'...");
    }

    if (!scope->has_symbol(ident, false)) {
      scope->free_variables.emplace(ident);
    }
  }
};

/*
 * Interpreter
 */
struct Environment {
  Environment(shared_ptr<SymbolScope> scope, shared_ptr<Environment> outer)
      : scope(scope), outer(outer) {}

  int get_value(const shared_ptr<AstPL0> ast, const string& ident) const {
    auto it = scope->constants.find(ident);
    if (it != scope->constants.end()) {
      return it->second;
    } else if (scope->variables.count(ident)) {
      if (variables.find(ident) == variables.end()) {
        throw_runtime_error(ast, "uninitialized variable '" + ident + "'...");
      }
      return variables.at(ident);
    }
    return outer->get_value(ast, ident);
  }

  void set_variable(const string& ident, int val) {
    if (scope->variables.count(ident)) {
      variables[ident] = val;
    } else {
      outer->set_variable(ident, val);
    }
  }

  shared_ptr<AstPL0> get_procedure(const string& ident) const {
    return scope->get_procedure(ident);
  }

 private:
  shared_ptr<SymbolScope> scope;
  shared_ptr<Environment> outer;
  map<string, int> variables;
};

struct Interpreter {
  static void exec(const shared_ptr<AstPL0> ast,
                   shared_ptr<Environment> env = nullptr) {
    switch (ast->tag) {
      case "block"_:
        exec_block(ast, env);
        break;
      case "statement"_:
        exec_statement(ast, env);
        break;
      case "assignment"_:
        exec_assignment(ast, env);
        break;
      case "call"_:
        exec_call(ast, env);
        break;
      case "statements"_:
        exec_statements(ast, env);
        break;
      case "if"_:
        exec_if(ast, env);
        break;
      case "while"_:
        exec_while(ast, env);
        break;
      case "out"_:
        exec_out(ast, env);
        break;
      case "in"_:
        exec_in(ast, env);
        break;
      default:
        exec(ast->nodes[0], env);
        break;
    }
  }

 private:
  static void exec_block(const shared_ptr<AstPL0> ast,
                         shared_ptr<Environment> outer) {
    // block <- const var procedure statement
    exec(ast->nodes[3], make_shared<Environment>(ast->scope, outer));
  }

  static void exec_statement(const shared_ptr<AstPL0> ast,
                             shared_ptr<Environment> env) {
    // statement  <- (assignment / call / statements / if / while / out / in)?
    if (!ast->nodes.empty()) {
      exec(ast->nodes[0], env);
    }
  }

  static void exec_assignment(const shared_ptr<AstPL0> ast,
                              shared_ptr<Environment> env) {
    // assignment <- ident ':=' _ expression
    env->set_variable(ast->nodes[0]->token, eval(ast->nodes[1], env));
  }

  static void exec_call(const shared_ptr<AstPL0> ast,
                        shared_ptr<Environment> env) {
    // call <- 'CALL' __ ident
    exec_block(env->get_procedure(ast->nodes[0]->token), env);
  }

  static void exec_statements(const shared_ptr<AstPL0> ast,
                              shared_ptr<Environment> env) {
    // statements <- 'BEGIN' __ statement (';' _ statement )* 'END' __
    for (auto stmt : ast->nodes) {
      exec(stmt, env);
    }
  }

  static void exec_if(const shared_ptr<AstPL0> ast,
                      shared_ptr<Environment> env) {
    // if <- 'IF' __ condition 'THEN' __ statement
    if (eval_condition(ast->nodes[0], env)) {
      exec(ast->nodes[1], env);
    }
  }

  static void exec_while(const shared_ptr<AstPL0> ast,
                         shared_ptr<Environment> env) {
    // while <- 'WHILE' __ condition 'DO' __ statement
    auto cond = ast->nodes[0];
    auto stmt = ast->nodes[1];
    while (eval_condition(cond, env)) {
      exec(stmt, env);
    }
  }

  static void exec_out(const shared_ptr<AstPL0> ast,
                       shared_ptr<Environment> env) {
    // out <- ('out' __ / 'write' __ / '!' _) expression
    cout << eval(ast->nodes[0], env) << endl;
  }

  static void exec_in(const shared_ptr<AstPL0> ast,
                      shared_ptr<Environment> env) {
    // in <- ('in' __ / 'read' __ / '?' _) ident
    int val;
    cin >> val;
    env->set_variable(ast->nodes[0]->token, val);
  }

  static bool eval_condition(const shared_ptr<AstPL0> ast,
                             shared_ptr<Environment> env) {
    // condition <- odd / compare
    const auto& node = ast->nodes[0];
    switch (node->tag) {
      case "odd"_:
        return eval_odd(node, env);
      case "compare"_:
        return eval_compare(node, env);
      default:
        throw logic_error("invalid AstPL0 type");
    }
  }

  static bool eval_odd(const shared_ptr<AstPL0> ast,
                       shared_ptr<Environment> env) {
    // odd <- 'ODD' __ expression
    return eval_expression(ast->nodes[0], env) != 0;
  }

  static bool eval_compare(const shared_ptr<AstPL0> ast,
                           shared_ptr<Environment> env) {
    // compare <- expression compare_op expression
    const auto& nodes = ast->nodes;
    auto lval = eval_expression(nodes[0], env);
    auto op = peg::str2tag(nodes[1]->token.c_str());
    auto rval = eval_expression(nodes[2], env);
    switch (op) {
      case "="_:
        return lval == rval;
      case "#"_:
        return lval != rval;
      case "<="_:
        return lval <= rval;
      case "<"_:
        return lval < rval;
      case ">="_:
        return lval >= rval;
      case ">"_:
        return lval > rval;
      default:
        throw logic_error("invalid operator");
    }
  }

  static int eval(const shared_ptr<AstPL0> ast, shared_ptr<Environment> env) {
    switch (ast->tag) {
      case "expression"_:
        return eval_expression(ast, env);
      case "term"_:
        return eval_term(ast, env);
      case "ident"_:
        return eval_ident(ast, env);
      case "number"_:
        return eval_number(ast, env);
      default:
        return eval(ast->nodes[0], env);
    }
  }

  static int eval_expression(const shared_ptr<AstPL0> ast,
                             shared_ptr<Environment> env) {
    // expression <- sign term (term_op term)*
    const auto& nodes = ast->nodes;
    auto sign = nodes[0]->token;
    auto sign_val = (sign.empty() || sign == "+") ? 1 : -1;
    auto val = eval(nodes[1], env) * sign_val;
    for (auto i = 2u; i < nodes.size(); i += 2) {
      auto ope = nodes[i + 0]->token[0];
      auto rval = eval(nodes[i + 1], env);
      switch (ope) {
        case '+':
          val = val + rval;
          break;
        case '-':
          val = val - rval;
          break;
      }
    }
    return val;
  }

  static int eval_term(const shared_ptr<AstPL0> ast,
                       shared_ptr<Environment> env) {
    // term <- factor (factor_op factor)*
    const auto& nodes = ast->nodes;
    auto val = eval(nodes[0], env);
    for (auto i = 1u; i < nodes.size(); i += 2) {
      auto ope = nodes[i + 0]->token[0];
      auto rval = eval(nodes[i + 1], env);
      switch (ope) {
        case '*':
          val = val * rval;
          break;
        case '/':
          if (rval == 0) {
            throw_runtime_error(ast, "divide by 0 error");
          }
          val = val / rval;
          break;
      }
    }
    return val;
  }

  static int eval_ident(const shared_ptr<AstPL0> ast,
                        shared_ptr<Environment> env) {
    return env->get_value(ast, ast->token);
  }

  static int eval_number(const shared_ptr<AstPL0> ast,
                         shared_ptr<Environment> env) {
    return stol(ast->token);
  }
};

/*
 * LLVM
 */
struct LLVM {
  LLVM(const shared_ptr<AstPL0> ast) : builder_(context_) {
    module_ = make_unique<Module>("pl0", context_);
    compile(ast);
  }

  void dump() { module_->print(llvm::outs(), nullptr); }

  void exec() {
    unique_ptr<ExecutionEngine> ee(EngineBuilder(std::move(module_)).create());
    std::vector<GenericValue> noargs;
    auto fn = ee->FindFunctionNamed("main");
    auto ret = ee->runFunction(fn, noargs);
  }

 private:
  LLVMContext context_;
  IRBuilder<> builder_;
  unique_ptr<Module> module_;

  void compile(const shared_ptr<AstPL0> ast) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    compile_libs();
    compile_program(ast);
  }

  void compile_switch(const shared_ptr<AstPL0> ast) {
    switch (ast->tag) {
      case "assignment"_:
        compile_assignment(ast);
        break;
      case "call"_:
        compile_call(ast);
        break;
      case "statements"_:
        compile_statements(ast);
        break;
      case "if"_:
        compile_if(ast);
        break;
      case "while"_:
        compile_while(ast);
        break;
      case "out"_:
        compile_out(ast);
        break;
      default:
        compile_switch(ast->nodes[0]);
        break;
    }
  }

  Value* compile_switch_value(const shared_ptr<AstPL0> ast) {
    switch (ast->tag) {
      case "odd"_:
        return compile_odd(ast);
      case "compare"_:
        return compile_compare(ast);
      case "expression"_:
        return compile_expression(ast);
      case "ident"_:
        return compile_ident(ast);
      case "number"_:
        return compile_number(ast);
      default:
        return compile_switch_value(ast->nodes[0]);
    }
  }

  void compile_libs() {
    auto printfF = module_->getOrInsertFunction(
        "printf",
        FunctionType::get(builder_.getInt32Ty(),
                          PointerType::get(builder_.getInt8Ty(), 0), true));

#if LLVM_VERSION_MAJOR >= 9
    auto funccallee = module_->getOrInsertFunction("out", builder_.getVoidTy(), builder_.getInt32Ty());
    auto outC = funccallee.getCallee();
#else
    auto outC = module_->getOrInsertFunction("out", builder_.getVoidTy(), builder_.getInt32Ty());
#endif
    auto outF = cast<Function>(outC);

    {
      auto BB = BasicBlock::Create(context_, "entry", outF);
      builder_.SetInsertPoint(BB);

      auto val = &*outF->arg_begin();

      auto fmt = builder_.CreateGlobalStringPtr("%d\n");
      std::vector<Value*> args = {fmt, val};
      builder_.CreateCall(printfF, args);

      builder_.CreateRetVoid();
    }
  }

  void compile_program(const shared_ptr<AstPL0> ast) {
#if LLVM_VERSION_MAJOR >= 9
    auto funccallee = module_->getOrInsertFunction("main", builder_.getVoidTy());
    auto c = funccallee.getCallee();
#else
    auto c = module_->getOrInsertFunction("main", builder_.getVoidTy());
#endif
    auto fn = cast<Function>(c);

    {
      auto BB = BasicBlock::Create(context_, "entry", fn);
      builder_.SetInsertPoint(BB);
      compile_block(ast->nodes[0]);
      builder_.CreateRetVoid();
      verifyFunction(*fn);
    }
  }

  void compile_block(const shared_ptr<AstPL0> ast) {
    compile_const(ast->nodes[0]);
    compile_var(ast->nodes[1]);
    compile_procedure(ast->nodes[2]);
    compile_statement(ast->nodes[3]);
  }

  void compile_const(const shared_ptr<AstPL0> ast) {
    for (auto i = 0u; i < ast->nodes.size(); i += 2) {
      auto ident = ast->nodes[i]->token;
      auto number = stoi(ast->nodes[i + 1]->token);

      auto alloca =
          builder_.CreateAlloca(builder_.getInt32Ty(), nullptr, ident);
      builder_.CreateStore(builder_.getInt32(number), alloca);
    }
  }

  void compile_var(const shared_ptr<AstPL0> ast) {
    for (const auto node : ast->nodes) {
      auto ident = node->token;
      builder_.CreateAlloca(builder_.getInt32Ty(), nullptr, ident);
    }
  }

  void compile_procedure(const shared_ptr<AstPL0> ast) {
    for (auto i = 0u; i < ast->nodes.size(); i += 2) {
      auto ident = ast->nodes[i]->token;
      auto block = ast->nodes[i + 1];

      std::vector<Type*> pt(block->scope->free_variables.size(),
                            Type::getInt32PtrTy(context_));
      auto ft = FunctionType::get(builder_.getVoidTy(), pt, false);
#if LLVM_VERSION_MAJOR >= 9
      auto funccallee = module_->getOrInsertFunction(ident, ft);
      auto c = funccallee.getCallee();
#else
      auto c = module_->getOrInsertFunction(ident, ft);
#endif
      auto fn = cast<Function>(c);

      {
        auto it = block->scope->free_variables.begin();
        for (auto& arg : fn->args()) {
          arg.setName(*it);
          ++it;
        }
      }

      {
        auto prevBB = builder_.GetInsertBlock();
        auto BB = BasicBlock::Create(context_, "entry", fn);
        builder_.SetInsertPoint(BB);
        compile_block(block);
        builder_.CreateRetVoid();
        verifyFunction(*fn);
        builder_.SetInsertPoint(prevBB);
      }
    }
  }

  void compile_statement(const shared_ptr<AstPL0> ast) {
    if (!ast->nodes.empty()) {
      compile_switch(ast->nodes[0]);
    }
  }

  void compile_assignment(const shared_ptr<AstPL0> ast) {
    auto ident = ast->nodes[0]->token;

    auto fn = builder_.GetInsertBlock()->getParent();
    auto tbl = fn->getValueSymbolTable();
    auto var = tbl->lookup(ident);
    if (!var) {
      throw_runtime_error(ast, "'" + ident + "' is not defined...");
    }

    auto val = compile_expression(ast->nodes[1]);
    builder_.CreateStore(val, var);
  }

  void compile_call(const shared_ptr<AstPL0> ast) {
    auto ident = ast->nodes[0]->token;

    auto scope = get_closest_scope(ast);
    auto block = scope->get_procedure(ident);

    std::vector<Value*> args;
    for (auto& free : block->scope->free_variables) {
      auto fn = builder_.GetInsertBlock()->getParent();
      auto tbl = fn->getValueSymbolTable();
      auto var = tbl->lookup(free);
      if (!var) {
        throw_runtime_error(ast, "'" + free + "' is not defined...");
      }
      args.push_back(var);
    }

    auto fn = module_->getFunction(ident);
    builder_.CreateCall(fn, args);
  }

  void compile_statements(const shared_ptr<AstPL0> ast) {
    for (auto node : ast->nodes) {
      compile_statement(node);
    }
  }

  void compile_if(const shared_ptr<AstPL0> ast) {
    auto cond = compile_condition(ast->nodes[0]);

    auto fn = builder_.GetInsertBlock()->getParent();
    auto ifThen = BasicBlock::Create(context_, "if.then", fn);
    auto ifEnd = BasicBlock::Create(context_, "if.end");

    builder_.CreateCondBr(cond, ifThen, ifEnd);

    builder_.SetInsertPoint(ifThen);
    compile_statement(ast->nodes[1]);
    builder_.CreateBr(ifEnd);

    fn->getBasicBlockList().push_back(ifEnd);
    builder_.SetInsertPoint(ifEnd);
  }

  void compile_while(const shared_ptr<AstPL0> ast) {
    auto whileCond = BasicBlock::Create(context_, "while.cond");
    builder_.CreateBr(whileCond);

    auto fn = builder_.GetInsertBlock()->getParent();
    fn->getBasicBlockList().push_back(whileCond);
    builder_.SetInsertPoint(whileCond);

    auto cond = compile_condition(ast->nodes[0]);

    auto whileBody = BasicBlock::Create(context_, "while.body", fn);
    auto whileEnd = BasicBlock::Create(context_, "while.end");
    builder_.CreateCondBr(cond, whileBody, whileEnd);

    builder_.SetInsertPoint(whileBody);
    compile_statement(ast->nodes[1]);

    builder_.CreateBr(whileCond);

    fn->getBasicBlockList().push_back(whileEnd);
    builder_.SetInsertPoint(whileEnd);
  }

  Value* compile_condition(const shared_ptr<AstPL0> ast) {
    return compile_switch_value(ast->nodes[0]);
  }

  Value* compile_odd(const shared_ptr<AstPL0> ast) {
    auto val = compile_expression(ast->nodes[0]);
    return builder_.CreateICmpNE(val, builder_.getInt32(0), "icmpne");
  }

  Value* compile_compare(const shared_ptr<AstPL0> ast) {
    auto lhs = compile_expression(ast->nodes[0]);
    auto rhs = compile_expression(ast->nodes[2]);

    const auto& ope = ast->nodes[1]->token;
    switch (ope[0]) {
      case '=':
        return builder_.CreateICmpEQ(lhs, rhs, "icmpeq");
      case '#':
        return builder_.CreateICmpNE(lhs, rhs, "icmpne");
      case '<':
        if (ope.size() == 1) {
          return builder_.CreateICmpSLT(lhs, rhs, "icmpslt");
        }
        // '<='
        return builder_.CreateICmpSLE(lhs, rhs, "icmpsle");
      case '>':
        if (ope.size() == 1) {
          return builder_.CreateICmpSGT(lhs, rhs, "icmpsgt");
        }
        // '>='
        return builder_.CreateICmpSGE(lhs, rhs, "icmpsge");
    }
    return nullptr;
  }

  void compile_out(const shared_ptr<AstPL0> ast) {
    auto val = compile_expression(ast->nodes[0]);
    auto outF = module_->getFunction("out");
    builder_.CreateCall(outF, val);
  }

  Value* compile_expression(const shared_ptr<AstPL0> ast) {
    const auto& nodes = ast->nodes;

    auto sign = nodes[0]->token;
    auto negative = !(sign.empty() || sign == "+");

    auto val = compile_term(nodes[1]);
    if (negative) {
      val = builder_.CreateNeg(val, "negative");
    }

    for (auto i = 2u; i < nodes.size(); i += 2) {
      auto ope = nodes[i + 0]->token[0];
      auto rval = compile_term(nodes[i + 1]);
      switch (ope) {
        case '+':
          val = builder_.CreateAdd(val, rval, "add");
          break;
        case '-':
          val = builder_.CreateSub(val, rval, "sub");
          break;
      }
    }
    return val;
  }

  Value* compile_term(const shared_ptr<AstPL0> ast) {
    const auto& nodes = ast->nodes;
    auto val = compile_factor(nodes[0]);
    for (auto i = 1u; i < nodes.size(); i += 2) {
      auto ope = nodes[i + 0]->token[0];
      auto rval = compile_switch_value(nodes[i + 1]);
      switch (ope) {
        case '*':
          val = builder_.CreateMul(val, rval, "mul");
          break;
        case '/': {
          // TODO: Zero devide error?
          // auto ret = builder_.CreateICmpEQ(rval, builder_.getInt32(0),
          // "icmpeq");
          // if (!ret) {
          //   throw_runtime_error(ast, "divide by 0 error");
          // }
          val = builder_.CreateSDiv(val, rval, "div");
          break;
        }
      }
    }
    return val;
  }

  Value* compile_factor(const shared_ptr<AstPL0> ast) {
    return compile_switch_value(ast->nodes[0]);
  }

  Value* compile_ident(const shared_ptr<AstPL0> ast) {
    auto ident = ast->token;

    auto fn = builder_.GetInsertBlock()->getParent();
    auto tbl = fn->getValueSymbolTable();
    auto var = tbl->lookup(ident);
    if (!var) {
      throw_runtime_error(ast, "'" + ident + "' is not defined...");
    }

    return builder_.CreateLoad(var);
  }

  Value* compile_number(const shared_ptr<AstPL0> ast) {
    return ConstantInt::getIntegerValue(builder_.getInt32Ty(),
                                        APInt(32, ast->token, 10));
  }
};

/*
 * Main
 */
int main(int argc, const char** argv) {
  if (argc < 2) {
    cout << "usage: pl0 PATH [--ast] [--llvm] [--jit]" << endl;
    return 1;
  }

  // Parser commandline parameters
  auto path = argv[1];
  bool opt_jit = false;
  bool opt_ast = false;
  bool opt_llvm = false;
  {
    auto argi = 2;
    while (argi < argc) {
      if (string("--ast") == argv[argi]) {
        opt_ast = true;
      } else if (string("--jit") == argv[argi]) {
        opt_jit = true;
      } else if (string("--llvm") == argv[argi]) {
        opt_llvm = true;
      }
      argi++;
    }
  }

  // Read a source file into memory
  vector<char> source;
  ifstream ifs(path, ios::in | ios::binary);
  if (ifs.fail()) {
    cerr << "can't open the source file." << endl;
    return -1;
  }
  source.resize(static_cast<unsigned int>(ifs.seekg(0, ios::end).tellg()));
  if (!source.empty()) {
    ifs.seekg(0, ios::beg)
        .read(&source[0], static_cast<streamsize>(source.size()));
  }

  // Setup a PEG parser
  parser parser(grammar);
  parser.enable_ast<AstPL0>();
  parser.log = [&](size_t ln, size_t col, const string& msg) {
    cerr << format_error_message(path, ln, col, msg) << endl;
  };

  // Parse the source and make an AST
  shared_ptr<AstPL0> ast;
  if (parser.parse_n(source.data(), source.size(), ast, path)) {
    try {
      SymbolTable::build_on_ast(ast);

      if (opt_ast) {
        cout << ast_to_s<AstPL0>(ast);
      }

      if (opt_llvm || opt_jit) {
        LLVM compiler(ast);

        if (opt_llvm) {
          compiler.dump();
        }
        if (opt_jit) {
          compiler.exec();
        }
      } else {
        Interpreter::exec(ast);
      }

    } catch (const runtime_error& e) {
      cerr << e.what() << endl;
    }
    return 0;
  }

  return -1;
}
