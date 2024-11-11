#!/bin/bash

if [ $# -ne 2 ]
  then
    echo "Usage: ./run.bash <file> <topname>"
fi

filename=$(basename -- "$1")
filename="${filename%.*}"

# Fail on error:
set -e

# Trace:
set -x

# Run interpreter
./bazel-bin/xls/dslx/interpreter_main "$1" --alsologtostderr

# Generate IR
./bazel-bin/xls/dslx/ir_convert/ir_converter_main --top="$2" "$filename".x > /tmp/"$filename".ir

# Optimize IR
./bazel-bin/xls/tools/opt_main /tmp/"$filename".ir > /tmp/"$filename".opt.ir

# IR Viz
# ./bazel-bin/xls/visualization/ir_viz/app --delay_model=unit --preload_ir_path=/tmp/"$filename".opt.ir

# Run Codegen
./bazel-bin/xls/tools/codegen_main --reset=reset --pipeline_stages=6 --delay_model=unit --multi_proc=true /tmp/"$filename".opt.ir --output_signature_path /tmp/"$filename".sig.proto > /tmp/"$filename".v
#./bazel-bin/xls/tools/codegen_main --pipeline_stages=1 --delay_model=unit --multi_proc=true /tmp/"$filename".opt.ir > /tmp/"$filename".v

# Run sim
