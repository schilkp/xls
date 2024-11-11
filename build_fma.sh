#!/bin/sh
echo "interp"
./bazel-bin/xls/dslx/interpreter_main ./fmac.x
echo "conv"
./bazel-bin/xls/dslx/ir_convert/ir_converter_main --top=fp32_fmac fmac.x > /tmp/fmac.ir
echo "opt"
./bazel-bin/xls/tools/opt_main /tmp/fmac.ir > /tmp/fmac.opt.ir
echo "codegen"
./bazel-bin/xls/tools/codegen_main --pipeline_stages=1 --delay_model=unit --multi_proc=true /tmp/fmac.opt.ir > /tmp/fmac.v
