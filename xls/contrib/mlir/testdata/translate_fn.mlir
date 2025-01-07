// RUN: xls_translate --mlir-xls-to-xls %s --main-function="__sample__main" -- 2>&1 | FileCheck %s

// CHECK-LABEL: top fn __sample__main
func.func @__sample__main(%arg0: i26, %arg1: i3, %arg2: i6) -> i6 {
  return %arg2 : i6
}

