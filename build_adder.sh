#!/bin/sh
echo "interp"
./bazel-bin/xls/dslx/interpreter_main ./adder.x
echo "conv"
./bazel-bin/xls/dslx/ir_convert/ir_converter_main --top=adder adder.x > /tmp/adder.ir
echo "opt"
./bazel-bin/xls/tools/opt_main /tmp/adder.ir > /tmp/adder.opt.ir
echo "codegen"
./bazel-bin/xls/tools/codegen_main --pipeline_stages=1 --delay_model=unit /tmp/adder.opt.ir > /tmp/adder.v
