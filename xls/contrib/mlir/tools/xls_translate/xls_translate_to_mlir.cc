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

#include "llvm/include/llvm/Support/raw_ostream.h"
#include "mlir/include/mlir/IR/BuiltinOps.h"
#include "mlir/include/mlir/IR/Location.h"
#include "xls/contrib/mlir/IR/xls_ops.h"
#include "xls/public/ir_parser.h"

namespace mlir::xls {

Type translateType(::xls::Type* xls_type, OpBuilder& builder,
                   MLIRContext* ctx) {
  llvm::outs() << "translated type! \n";
  switch (xls_type->kind()) {
    case ::xls::TypeKind::kTuple: {
      // FIXME: Clean this up
      // FIXME: Handle translateType error?
      std::vector<Type> types{};
      for (auto type : xls_type->AsTupleOrDie()->element_types()) {
        types.push_back(translateType(type, builder, ctx));
      }
      return TupleType::get(ctx, types);
      break;
    }
    case ::xls::TypeKind::kBits: {
      return builder.getIntegerType(xls_type->AsBitsOrDie()->bit_count(),
                                    false);
    }
    case ::xls::TypeKind::kArray: {
      ArrayType::get(xls_type->AsArrayOrDie()->size(),
                     translateType(xls_type->AsArrayOrDie()->element_type(),
                                   builder, ctx));
    }
    case ::xls::TypeKind::kToken: {
      return TokenType::get(ctx);
    }
  }
}

OwningOpRef<Operation*> XlsToMlirXlsTranslate(llvm::SourceMgr& mgr,
                                              MLIRContext* ctx) {
  auto buf = mgr.getMemoryBuffer(mgr.getMainFileID());

  llvm::outs() << "Dialects ops:\n";
  for (auto name : ctx->getDialectRegistry().getDialectNames()) {
    llvm::outs() << name  << "\n";
  }

  llvm::outs() << "XLS Registerd ops:\n";
  for (auto name : ctx->getRegisteredOperationsByDialect("xls")) {
    llvm::outs() << name << "\n";
  }

  // Parse XLS IR:
  auto package = ::xls::ParsePackage(buf->getBuffer().str(), std::nullopt);
  if (!package.ok()) {
    llvm::errs() << "Failed to parse: " << package.status().message() << "\n";
    return nullptr;
  }

  // New top module to hold generated MLIR:
  auto loc = FileLineColLoc::get(
      StringAttr::get(ctx, buf->getBufferIdentifier()), /*line=*/0,
      /*column=*/0);
  OwningOpRef<ModuleOp> module(ModuleOp::create(loc));

  OpBuilder builder(ctx);

  // Translate all channels:
  for (auto c : package->get()->channels()) {
    builder.setInsertionPointToEnd(module->getBody());
    builder.create<xls::ChanOp>(
        builder.getUnknownLoc(),
        /*name=*/builder.getStringAttr(c->name()),
        /*type=*/TypeAttr::get(translateType(c->type(), builder, ctx)),
        /*send_supported=*/builder.getBoolAttr(c->CanSend()),
        /*recv_supported=*/builder.getBoolAttr(c->CanReceive()));
  }

  return module;
}

}  // namespace mlir::xls
