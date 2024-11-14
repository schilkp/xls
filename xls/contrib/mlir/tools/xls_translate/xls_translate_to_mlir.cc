// Copyright 2024 The XLS Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/contrib/mlir/tools/xls_translate/xls_translate_to_mlir.h"

#include <cassert>
#include <cstdint>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "llvm/include/llvm/Support/SourceMgr.h"
#include "llvm/include/llvm/Support/raw_ostream.h"
#include "mlir/include/mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/include/mlir/IR/BuiltinOps.h"
#include "mlir/include/mlir/IR/Location.h"
#include "xls/contrib/mlir/IR/xls_ops.h"
#include "xls/ir/function.h"
#include "xls/ir/nodes.h"
#include "xls/public/ir_parser.h"

// TODO(schilkp): Work-out error handling + source diagnostic tracking.
// XLS/Abseil infra or LLVM/MLIR infra?

namespace mlir::xls {

//===----------------------------------------------------------------------===//
// Package Context
//===----------------------------------------------------------------------===//

class PackageContext {
 public:
  PackageContext() = default;

  absl::Status add_fn(std::string name, FlatSymbolRefAttr fn) {
    if (fn_map_.contains(name)) {
      return absl::InternalError(
          absl::StrCat("Duplicate function name ", name));
    }
    fn_map_[name] = fn;
    return absl::OkStatus();
  }

  absl::StatusOr<SymbolRefAttr> get_fn(std::string name) {
    auto op = fn_map_.find(name);
    if (op == fn_map_.end()) {
      return absl::InternalError(absl::StrCat("Unknown function name  ", name));
    } else {
      return op->second;
    }
  }

  absl::Status add_chn(std::string name, FlatSymbolRefAttr chn) {
    if (chn_map_.contains(name)) {
      return absl::InternalError(
          absl::StrCat("Duplicate function name ", name));
    }
    chn_map_[name] = chn;
    return absl::OkStatus();
  }

  absl::StatusOr<SymbolRefAttr> get_chn(std::string name) {
    auto op = chn_map_.find(name);
    if (op == chn_map_.end()) {
      return absl::InternalError(absl::StrCat("Unknown function name  ", name));
    } else {
      return op->second;
    }
  }

 private:
  std::unordered_map<std::string, SymbolRefAttr> fn_map_;
  std::unordered_map<std::string, SymbolRefAttr> chn_map_;
};

//===----------------------------------------------------------------------===//
// Function Context
//===----------------------------------------------------------------------===//

class FunctionContext {
 public:
  FunctionContext() = default;

  absl::Status add_ssa(int64_t id, Value op) {
    if (ssa_map_.contains(id)) {
      return absl::InternalError(
          absl::StrCat("Duplicate assignment to op id ", id));
    }
    ssa_map_[id] = op;
    return absl::OkStatus();
  }

  absl::StatusOr<Value> get_ssa(int64_t id) {
    auto op = ssa_map_.find(id);
    if (op == ssa_map_.end()) {
      return absl::InternalError(absl::StrCat("Unknown op id ", id));
    } else {
      return op->second;
    }
  }

 private:
  std::unordered_map<int64_t, Value> ssa_map_;
};

//===----------------------------------------------------------------------===//
// Type/Value Translation
//===----------------------------------------------------------------------===//

Type translateType(::xls::Type* xls_type, OpBuilder& builder,
                   MLIRContext* ctx) {
  switch (xls_type->kind()) {
    case ::xls::TypeKind::kTuple: {
      std::vector<Type> types{};
      for (auto type : xls_type->AsTupleOrDie()->element_types()) {
        types.push_back(translateType(type, builder, ctx));
      }
      return TupleType::get(ctx, types);
    }
    case ::xls::TypeKind::kBits: {
      return builder.getIntegerType(xls_type->AsBitsOrDie()->bit_count());
    }
    case ::xls::TypeKind::kArray: {
      return ArrayType::get(
          xls_type->AsArrayOrDie()->size(),
          translateType(xls_type->AsArrayOrDie()->element_type(), builder,
                        ctx));
    }
    case ::xls::TypeKind::kToken: {
      return TokenType::get(ctx);
    }
  }
}

APInt translateBits(const ::xls::Bits& b) {
  uint64_t num_words = b.bitmap().word_count();
  std::vector<uint64_t> words{};
  for (uint64_t i = 0; i < num_words; i++) {
    words.push_back(b.bitmap().GetWord(i));
  }
  return APInt(b.bit_count(), words);
}

//===----------------------------------------------------------------------===//
// Operation Translation
//===----------------------------------------------------------------------===//

absl::StatusOr<Operation*> translateOp(::xls::ArithOp& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto opr_lhs =
      func_ctx.get_ssa(node.operands()[::xls::ArithOp::kLhsOperand]->id());
  if (!opr_lhs.ok()) {
    return opr_lhs.status();
  }

  auto opr_rhs =
      func_ctx.get_ssa(node.operands()[::xls::ArithOp::kRhsOperand]->id());
  if (!opr_rhs.ok()) {
    return opr_rhs.status();
  }

  switch (node.op()) {
    case ::xls::Op::kUMul:
      return builder.create<xls::UmulOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    case ::xls::Op::kSMul:
      return builder.create<xls::SmulOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    default:
      return absl::InternalError(absl::StrCat(
          "Expected ArithOp operation, not ", ::xls::OpToString(node.op())));
  }
}

absl::StatusOr<Operation*> translateOp(::xls::CompareOp& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto opr_lhs =
      func_ctx.get_ssa(node.operands()[::xls::CompareOp::kLhsOperand]->id());
  if (!opr_lhs.ok()) {
    return opr_lhs.status();
  }

  auto opr_rhs =
      func_ctx.get_ssa(node.operands()[::xls::CompareOp::kRhsOperand]->id());
  if (!opr_rhs.ok()) {
    return opr_rhs.status();
  }

  switch (node.op()) {
    case ::xls::Op::kEq:
      return builder.create<xls::EqOp>(builder.getUnknownLoc(), *opr_lhs,
                                       *opr_rhs);
    case ::xls::Op::kNe:
      return builder.create<xls::NeOp>(builder.getUnknownLoc(), *opr_lhs,
                                       *opr_rhs);
    case ::xls::Op::kSLe:
      return builder.create<xls::SleOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kSGe:
      return builder.create<xls::SgeOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kSLt:
      return builder.create<xls::SltOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kSGt:
      return builder.create<xls::SgtOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kULe:
      return builder.create<xls::UleOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kUGe:
      return builder.create<xls::UgeOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kULt:
      return builder.create<xls::UltOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kUGt:
      return builder.create<xls::UgtOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    default:
      return absl::InternalError(absl::StrCat(
          "Expected CompareOp operation, not ", ::xls::OpToString(node.op())));
  }
}

absl::StatusOr<Operation*> translateOp(::xls::BinOp& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto opr_lhs =
      func_ctx.get_ssa(node.operands()[::xls::BinOp::kLhsOperand]->id());
  if (!opr_lhs.ok()) {
    return opr_lhs.status();
  }
  auto opr_rhs =
      func_ctx.get_ssa(node.operands()[::xls::BinOp::kRhsOperand]->id());
  if (!opr_rhs.ok()) {
    return opr_rhs.status();
  }

  switch (node.op()) {
    case ::xls::Op::kAdd:
      return builder.create<xls::AddOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kSDiv:
      return builder.create<xls::SdivOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    case ::xls::Op::kSMod:
      return builder.create<xls::SmodOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    case ::xls::Op::kShll:
      return builder.create<xls::ShllOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    case ::xls::Op::kShrl:
      return builder.create<xls::ShrlOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    case ::xls::Op::kShra:
      return builder.create<xls::ShraOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    case ::xls::Op::kSub:
      return builder.create<xls::SubOp>(builder.getUnknownLoc(), *opr_lhs,
                                        *opr_rhs);
    case ::xls::Op::kUDiv:
      return builder.create<xls::UdivOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    case ::xls::Op::kUMod:
      return builder.create<xls::UmodOp>(builder.getUnknownLoc(), *opr_lhs,
                                         *opr_rhs);
    default:
      return absl::InternalError(absl::StrCat("Expected BinOp operation, not ",
                                              ::xls::OpToString(node.op())));
  }
}

absl::StatusOr<Operation*> translateOp(::xls::UnOp& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto operand =
      func_ctx.get_ssa(node.operands()[::xls::UnOp::kArgOperand]->id());
  if (!operand.ok()) {
    return operand.status();
  }

  switch (node.op()) {
    case ::xls::Op::kIdentity:
      return builder.create<xls::IdentityOp>(builder.getUnknownLoc(), *operand);
    case ::xls::Op::kNeg:
      return builder.create<xls::NegOp>(builder.getUnknownLoc(), *operand);
    case ::xls::Op::kNot:
      return builder.create<xls::NotOp>(builder.getUnknownLoc(), *operand);
    case ::xls::Op::kReverse:
      return builder.create<xls::ReverseOp>(builder.getUnknownLoc(), *operand);
    default:
      return absl::InternalError(absl::StrCat("Expected UnOp operation, not ",
                                              ::xls::OpToString(node.op())));
  }
}

absl::StatusOr<Operation*> translateOp(::xls::ExtendOp& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto operand =
      func_ctx.get_ssa(node.operands()[::xls::ExtendOp::kArgOperand]->id());
  if (!operand.ok()) {
    return operand.status();
  }

  auto result_type = builder.getIntegerType(node.new_bit_count());

  switch (node.op()) {
    case ::xls::Op::kZeroExt:
      return builder.create<xls::ZeroExtOp>(builder.getUnknownLoc(),
                                            result_type, *operand);
    case ::xls::Op::kSignExt:
      return builder.create<xls::SignExtOp>(builder.getUnknownLoc(),
                                            result_type, *operand);
    default:
      return absl::InternalError(absl::StrCat(
          "Expected ExtendOp operation, not ", ::xls::OpToString(node.op())));
  }
}

absl::StatusOr<Operation*> translateOp(::xls::TupleIndex& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto operand =
      func_ctx.get_ssa(node.operands()[::xls::TupleIndex::kArgOperand]->id());
  if (!operand.ok()) {
    return operand.status();
  }

  return builder.create<xls::TupleIndexOp>(builder.getUnknownLoc(), *operand,
                                           node.index());
}

absl::StatusOr<Operation*> translateOp(::xls::Array& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  std::vector<Value> operands_vec{};
  for (auto xls_operand : node.operands()) {
    auto operand = func_ctx.get_ssa(xls_operand->id());
    if (!operand.ok()) {
      return operand.status();
    }
    operands_vec.push_back(*operand);
  }
  ValueRange operands(operands_vec);

  auto result_type = translateType(node.GetType(), builder, ctx);

  return builder.create<xls::ArrayOp>(builder.getUnknownLoc(), result_type,
                                      operands);
}

absl::StatusOr<Operation*> translateOp(::xls::ArrayIndex& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto xls_arg =
      func_ctx.get_ssa(node.operands()[::xls::ArrayIndex::kArgOperand]->id());
  if (!xls_arg.ok()) {
    return xls_arg.status();
  }

  if (node.indices().length() != 1) {
    return absl::InternalError(
        "MLIR currently only supports ArrayIndex with a single index!");
  }
  auto xls_index = func_ctx.get_ssa(
      node.operands()[::xls::ArrayIndex::kIndexOperandStart]->id());
  if (!xls_arg.ok()) {
    return xls_arg.status();
  }

  return builder.create<xls::ArrayIndexOp>(builder.getUnknownLoc(), *xls_arg,
                                           *xls_index);
}

absl::StatusOr<Operation*> translateOp(::xls::ArrayConcat& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  std::vector<Value> operands_vec{};
  for (auto xls_operand : node.operands()) {
    auto operand = func_ctx.get_ssa(xls_operand->id());
    if (!operand.ok()) {
      return operand.status();
    }
    operands_vec.push_back(*operand);
  }
  ValueRange operands(operands_vec);

  auto result_type = translateType(node.GetType(), builder, ctx);

  return builder.create<xls::ArrayConcatOp>(builder.getUnknownLoc(),
                                            result_type, operands);
}

absl::StatusOr<Operation*> translateOp(::xls::ArraySlice& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto array =
      func_ctx.get_ssa(node.operands()[::xls::ArraySlice::kArrayOperand]->id());
  if (!array.ok()) {
    return array.status();
  }
  auto start =
      func_ctx.get_ssa(node.operands()[::xls::ArraySlice::kStartOperand]->id());
  if (!start.ok()) {
    return start.status();
  }

  auto result_type = translateType(node.GetType(), builder, ctx);

  return builder.create<xls::ArraySliceOp>(builder.getUnknownLoc(), result_type,
                                           *array, *start, node.width());
}

absl::StatusOr<Operation*> translateOp(::xls::ArrayUpdate& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto array_to_update = func_ctx.get_ssa(node.array_to_update()->id());
  if (!array_to_update.ok()) {
    return array_to_update.status();
  }

  if (node.indices().length() != 1) {
    return absl::InternalError(
        "MLIR currently only supports ArrayUpdate with a single index!");
  }
  auto index = func_ctx.get_ssa(node.indices()[0]->id());
  if (!index.ok()) {
    return index.status();
  }

  auto value = func_ctx.get_ssa(node.update_value()->id());
  if (!value.ok()) {
    return value.status();
  }

  return builder.create<xls::ArrayUpdateOp>(builder.getUnknownLoc(),
                                            *array_to_update, *value, *index);
}

absl::StatusOr<Operation*> translateOp(::xls::BitSlice& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto arg =
      func_ctx.get_ssa(node.operands()[::xls::BitSlice::kArgOperand]->id());
  if (!arg.ok()) {
    return arg.status();
  }

  auto result_type = builder.getIntegerType(node.width());

  return builder.create<xls::BitSliceOp>(builder.getUnknownLoc(), result_type,
                                         *arg, node.start(), node.width());
}

absl::StatusOr<Operation*> translateOp(::xls::BitSliceUpdate& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto to_update = func_ctx.get_ssa(node.to_update()->id());
  if (!to_update.ok()) {
    return to_update.status();
  }

  auto start = func_ctx.get_ssa(node.start()->id());
  if (!start.ok()) {
    return start.status();
  }

  auto update_value = func_ctx.get_ssa(node.update_value()->id());
  if (!update_value.ok()) {
    return update_value.status();
  }

  auto result_type =
      builder.getIntegerType(node.to_update()->GetType()->GetFlatBitCount());

  return builder.create<xls::BitSliceUpdateOp>(
      builder.getUnknownLoc(), result_type, *to_update, *start, *update_value);
}

absl::StatusOr<Operation*> translateOp(::xls::Concat& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  std::vector<Value> operands_vec{};
  uint64_t result_width = 0;
  for (auto xls_operand : node.operands()) {
    auto operand = func_ctx.get_ssa(xls_operand->id());
    if (!operand.ok()) {
      return operand.status();
    }
    operands_vec.push_back(*operand);
    result_width += xls_operand->GetType()->GetFlatBitCount();
  }
  ValueRange operands(operands_vec);

  auto result_type = builder.getIntegerType(result_width);

  return builder.create<xls::BitSliceUpdateOp>(builder.getUnknownLoc(),
                                               result_type, operands);
}

absl::StatusOr<Operation*> translateOp(::xls::Tuple& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  std::vector<Value> operands_vec{};
  for (auto xls_operand : node.operands()) {
    auto operand = func_ctx.get_ssa(xls_operand->id());
    if (!operand.ok()) {
      return operand.status();
    }
    operands_vec.push_back(*operand);
  }
  ValueRange operands(operands_vec);

  return builder.create<xls::TupleOp>(builder.getUnknownLoc(), operands);
}

absl::StatusOr<Operation*> translateOp(::xls::Literal& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  switch (node.GetType()->kind()) {
    case ::xls::TypeKind::kBits: {
      auto value = translateBits(node.value().bits());
      auto type = builder.getIntegerType(node.value().bits().bit_count());
      return builder
          .create<ConstantScalarOp>(builder.getUnknownLoc(), type,
                                    builder.getIntegerAttr(type, value))
          .getOperation();
    }
    case ::xls::TypeKind::kTuple: {
      return absl::InternalError("TODO: Tuple");  // TODO(schilkp): Required?
    }
    default: {
      return absl::InternalError(absl::StrCat(
          "Cannot produce literal of type '", node.GetType()->ToString(), "'"));
    }
  }
}

absl::StatusOr<Operation*> translateOp(::xls::NaryOp& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  std::vector<Value> operands_vec{};
  for (auto xls_operand : node.operands()) {
    auto operand = func_ctx.get_ssa(xls_operand->id());
    if (!operand.ok()) {
      return operand.status();
    }
    operands_vec.push_back(*operand);
  }
  ValueRange operands(operands_vec);

  switch (node.op()) {
    case ::xls::Op::kAnd:
      return builder.create<xls::AndOp>(builder.getUnknownLoc(), operands);
    case ::xls::Op::kNand:
      return absl::InternalError(
          "Unsuported operation: nand - Not yet available in XLS MLIR!");
    case ::xls::Op::kNor:
      return absl::InternalError(
          "Unsuported operation: nor - Not yet available in XLS MLIR!");
    case ::xls::Op::kOr:
      return builder.create<xls::OrOp>(builder.getUnknownLoc(), operands);
    case ::xls::Op::kXor:
      return builder.create<xls::XorOp>(builder.getUnknownLoc(), operands);
    default:
      return absl::InternalError(absl::StrCat("Expected BinOp operation, not ",
                                              ::xls::OpToString(node.op())));
  }
}

absl::StatusOr<Operation*> translateOp(::xls::Encode& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto arg =
      func_ctx.get_ssa(node.operands()[::xls::Encode::kArgOperand]->id());
  if (!arg.ok()) {
    return arg.status();
  }

  auto result_type = builder.getIntegerType(node.GetType()->GetFlatBitCount());

  return builder.create<xls::EncodeOp>(builder.getUnknownLoc(), result_type,
                                       *arg);
}

absl::StatusOr<Operation*> translateOp(::xls::Decode& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto arg =
      func_ctx.get_ssa(node.operands()[::xls::Decode::kArgOperand]->id());
  if (!arg.ok()) {
    return arg.status();
  }

  auto result_type = builder.getIntegerType(node.GetType()->GetFlatBitCount());

  return builder.create<xls::DecodeOp>(builder.getUnknownLoc(), result_type,
                                       *arg);
}

absl::StatusOr<Operation*> translateOp(::xls::OneHot& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto arg =
      func_ctx.get_ssa(node.operands()[::xls::OneHot::kInputOperand]->id());
  if (!arg.ok()) {
    return arg.status();
  }

  auto result_type = builder.getIntegerType(node.GetType()->GetFlatBitCount());

  auto lsb_prio = builder.getBoolAttr(node.priority() == ::xls::LsbOrMsb::kLsb);

  return builder.create<xls::OneHotOp>(builder.getUnknownLoc(), result_type,
                                       *arg, lsb_prio);
}

absl::StatusOr<Operation*> translateOp(::xls::OneHotSelect& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto selector = func_ctx.get_ssa(
      node.operands()[::xls::OneHotSelect::kSelectorOperand]->id());
  if (!selector.ok()) {
    return selector.status();
  }

  std::vector<Value> cases_vec{};
  for (auto xls_case : node.cases()) {
    auto operand = func_ctx.get_ssa(xls_case->id());
    if (!operand.ok()) {
      return operand.status();
    }
    cases_vec.push_back(*operand);
  }
  ValueRange cases(cases_vec);

  auto result_type = builder.getIntegerType(node.GetType()->GetFlatBitCount());

  return builder.create<xls::OneHotSelOp>(builder.getUnknownLoc(), result_type,
                                          *selector, cases);
}

absl::StatusOr<Operation*> translateOp(::xls::Invoke& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx,
                                       PackageContext& pkg_ctx) {
  auto fn = pkg_ctx.get_fn(node.to_apply()->name());
  if (!fn.ok()) {
    return fn.status();
  }

  std::vector<Value> operands_vec{};
  for (auto xls_operand : node.operands()) {
    auto operand = func_ctx.get_ssa(xls_operand->id());
    if (!operand.ok()) {
      return operand.status();
    }
    operands_vec.push_back(*operand);
  }
  ValueRange operands(operands_vec);

  auto result_type = translateType(node.GetType(), builder, ctx);

  return builder.create<func::CallOp>(builder.getUnknownLoc(), *fn, result_type,
                                      operands);
}

absl::StatusOr<Operation*> translateOp(::xls::Select& node, OpBuilder& builder,
                                       MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto selector = func_ctx.get_ssa(node.selector()->id());
  if (!selector.ok()) {
    return selector.status();
  }

  std::vector<Value> cases_vec{};
  for (auto xls_case : node.cases()) {
    auto operand = func_ctx.get_ssa(xls_case->id());
    if (!operand.ok()) {
      return operand.status();
    }
    cases_vec.push_back(*operand);
  }
  ValueRange cases(cases_vec);

  auto result_type = builder.getIntegerType(node.GetType()->GetFlatBitCount());

  Value default_value{};

  if (auto xls_default_val = node.default_value();
      xls_default_val.has_value()) {
    auto maybe_default_val =
        func_ctx.get_ssa(node.default_value().value()->id());
    if (!maybe_default_val.ok()) {
      return maybe_default_val.status();
    }
    default_value = maybe_default_val.value();
  }

  return builder.create<xls::SelOp>(builder.getUnknownLoc(), result_type,
                                    *selector, default_value, cases);
}

absl::StatusOr<Operation*> translateOp(::xls::PrioritySelect& node,
                                       OpBuilder& builder, MLIRContext* ctx,
                                       FunctionContext& func_ctx) {
  auto selector = func_ctx.get_ssa(
      node.operands()[::xls::PrioritySelect::kSelectorOperand]->id());
  if (!selector.ok()) {
    return selector.status();
  }

  std::vector<Value> cases_vec{};
  for (auto xls_case : node.cases()) {
    auto operand = func_ctx.get_ssa(xls_case->id());
    if (!operand.ok()) {
      return operand.status();
    }
    cases_vec.push_back(*operand);
  }
  ValueRange cases(cases_vec);

  auto default_value = func_ctx.get_ssa(node.default_value()->id());
  if (!default_value.ok()) {
    return default_value.status();
  }

  auto result_type = builder.getIntegerType(node.GetType()->GetFlatBitCount());

  return builder.create<xls::PrioritySelOp>(
      builder.getUnknownLoc(), result_type, *selector, cases, *default_value);
}

absl::StatusOr<Operation*> translateAnyOp(::xls::Node& xls_node,
                                          OpBuilder& builder, MLIRContext* ctx,
                                          FunctionContext& func_ctx,
                                          PackageContext& pkg_ctx) {
  absl::StatusOr<Operation*> op;

  if (auto xls_op = dynamic_cast<::xls::Literal*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::BinOp*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::ArithOp*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::UnOp*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::CompareOp*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::NaryOp*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::ExtendOp*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::Tuple*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::TupleIndex*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::Array*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::ArrayIndex*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::ArrayConcat*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::ArraySlice*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::ArrayUpdate*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::BitSlice*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::BitSliceUpdate*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::Concat*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::Encode*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::Decode*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::OneHot*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::OneHotSelect*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::Invoke*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx, pkg_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::Select*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (auto xls_op = dynamic_cast<::xls::PrioritySelect*>(&xls_node)) {
    op = translateOp(*xls_op, builder, ctx, func_ctx);
  } else if (dynamic_cast<::xls::BitwiseReductionOp*>(&xls_node) ||
             dynamic_cast<::xls::PartialProductOp*>(&xls_node) ||
             dynamic_cast<::xls::Cover*>(&xls_node) ||
             dynamic_cast<::xls::Gate*>(&xls_node) ||
             dynamic_cast<::xls::Assert*>(&xls_node) ||
             dynamic_cast<::xls::MinDelay*>(&xls_node) ||
             dynamic_cast<::xls::RegisterRead*>(&xls_node) ||
             dynamic_cast<::xls::RegisterWrite*>(&xls_node)) {
    return absl::InternalError(
        absl::StrCat("Unsuported operation: ", ::xls::OpToString(xls_node.op()),
                     " - Not yet available in XLS MLIR!"));
  } else {
    return absl::InternalError(absl::StrCat("Unsuported operation: ",
                                            ::xls::OpToString(xls_node.op())));
  }

  if (!op.ok()) {
    return op.status();
  }

  if (auto err = func_ctx.add_ssa(xls_node.id(), (*op)->getResult(0));
      !err.ok()) {
    return err;
  }

  return *op;
}

//===----------------------------------------------------------------------===//
// Function Translation
//===----------------------------------------------------------------------===//

absl::StatusOr<Operation*> translateFunction(::xls::Function& xls_func,
                                             OpBuilder& builder,
                                             MLIRContext* ctx,
                                             PackageContext& pkg_ctx) {
  // Argument types:
  std::vector<Type> mlir_arg_types;
  for (auto arg : xls_func.GetType()->parameters()) {
    mlir_arg_types.push_back(translateType(arg, builder, ctx));
  }

  // Return type:
  auto return_type =
      translateType(xls_func.GetType()->return_type(), builder, ctx);

  // Create Function:
  auto funcType =
      mlir::FunctionType::get(ctx, TypeRange(mlir_arg_types), {return_type});
  auto func =
      func::FuncOp::create(builder.getUnknownLoc(), xls_func.name(), funcType);
  builder.insert(func);

  // Add function to package context:
  if (auto err = pkg_ctx.add_fn(xls_func.name(),
                                SymbolRefAttr::get(func.getNameAttr()));
      !err.ok()) {
    return err;
  }

  // Create function body:
  auto* body = func.addEntryBlock();

  // Track Function context (XLS IR id -> MLIR SSA/OP mapping)
  FunctionContext func_ctx{};

  // Add parameters to function context:
  for (uint64_t arg_idx = 0; arg_idx < xls_func.params().length(); arg_idx++) {
    auto xls_param = xls_func.params()[arg_idx];
    auto mlir_arg = body->getArgument(arg_idx);
    if (auto err = func_ctx.add_ssa(xls_param->id(), mlir_arg); !err.ok()) {
    }
  }

  // Function body:
  Operation* returning_op = nullptr;
  for (auto n : xls_func.nodes()) {
    builder.setInsertionPointToEnd(body);
    if (auto op = dynamic_cast<::xls::Param*>(n)) {
      continue;  // Params have already been converted and added to func
                 // context.
    }

    auto op = translateAnyOp(*n, builder, ctx, func_ctx, pkg_ctx);
    if (!op.ok()) {
      return op;
    }

    if (xls_func.return_value() == n) {
      returning_op = *op;
    }
  }

  assert(returning_op && "Function return op not translated.");
  assert(returning_op->getResults().size() == 1 &&
         "Function with multiple return ops.");

  // Function body terminator (return):
  Value r = returning_op->getResult(0);
  ValueRange rs{r};

  builder.setInsertionPointToEnd(body);
  builder.create<func::ReturnOp>(builder.getUnknownLoc(), rs);

  return func;
}

//===----------------------------------------------------------------------===//
// Channel Translation
//===----------------------------------------------------------------------===//

absl::Status translateChannel(::xls::Channel& xls_chn, OpBuilder& builder,
                              MLIRContext* ctx, PackageContext& pkg_ctx) {
  auto chn = builder.create<xls::ChanOp>(
      builder.getUnknownLoc(),
      /*name=*/builder.getStringAttr(xls_chn.name()),
      /*type=*/TypeAttr::get(translateType(xls_chn.type(), builder, ctx)),
      /*send_supported=*/builder.getBoolAttr(xls_chn.CanSend()),
      /*recv_supported=*/builder.getBoolAttr(xls_chn.CanReceive()));

  return pkg_ctx.add_chn(std::string(xls_chn.name()),
                         SymbolRefAttr::get(chn.getNameAttr()));
}

//===----------------------------------------------------------------------===//
// Package Translation
//===----------------------------------------------------------------------===//

absl::Status translatePackage(::xls::Package& xls_pkg, OpBuilder& builder,
                              MLIRContext* ctx, ModuleOp& module) {
  PackageContext pkg_ctx{};

  // Translate all channels:
  for (auto c : xls_pkg.channels()) {
    builder.setInsertionPointToEnd(module.getBody());
    if (auto err = translateChannel(*c, builder, ctx, pkg_ctx); !err.ok()) {
      return err;
    }
  }

  // Translate all functions:
  for (const auto& f : xls_pkg.functions()) {
    builder.setInsertionPointToEnd(module.getBody());
    auto func = translateFunction(*f, builder, ctx, pkg_ctx);
    if (!func.ok()) {
      return func.status();
    }
  }

  return absl::OkStatus();
}

OwningOpRef<Operation*> XlsToMlirXlsTranslate(llvm::SourceMgr& mgr,
                                              MLIRContext* ctx) {
  OpBuilder builder(ctx);

  // Load XLS dialect we will be emitting:
  ctx->loadDialect<XlsDialect>();

  // New top module to hold generated MLIR:
  const llvm::MemoryBuffer* buf = mgr.getMemoryBuffer(mgr.getMainFileID());
  auto loc = FileLineColLoc::get(
      StringAttr::get(ctx, buf->getBufferIdentifier()), /*line=*/0,
      /*column=*/0);
  ModuleOp module = ModuleOp::create(loc);

  // Parse XLS IR:
  absl::StatusOr<std::unique_ptr<::xls::Package>> package =
      ::xls::ParsePackage(buf->getBuffer().str(), std::nullopt);
  if (!package.ok()) {
    llvm::errs() << "Failed to parse: " << package.status().message() << "\n";
    return {};
  }

  // Translate package from XLS IR to MLIR:
  if (auto err = translatePackage(**package, builder, ctx, module); !err.ok()) {
    llvm::errs() << err.message() << "\n";
    return {};
  }

  return OwningOpRef(module);
}

}  // namespace mlir::xls
