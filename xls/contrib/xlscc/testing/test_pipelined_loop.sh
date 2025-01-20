#!/bin/bash
set -x
cd "$(dirname "$0")" || exit 1

INPUT="./test_pipelined_loop.cc"
CLASS="MyBlock"
RUN_NAME=$(basename -s ".cc" "$INPUT")

../../../../bazel-bin/xls/contrib/xlscc/xlscc $INPUT \
  --block_from_class $CLASS \
  --block_pb "$RUN_NAME".pb \
  --merge_states \
  --generate_fsms_for_pipelined_loops \
  --debug_print_fsm_states \
  > "$RUN_NAME".ir

../../../../bazel-bin/xls/tools/opt_main "$RUN_NAME".ir > "$RUN_NAME".opt.ir

../../../..//bazel-bin/xls/tools/codegen_main "$RUN_NAME".opt.ir \
  --generator=pipeline \
  --delay_model="sky130" \
  --output_verilog_path="$RUN_NAME".v \
  --module_name=xls_test \
  --top="$CLASS"_proc \
  --reset=rst \
  --reset_active_low=false \
  --reset_asynchronous=false \
  --reset_data_path=true \
  --pipeline_stages=1

