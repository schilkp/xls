#!/bin/bash
/home/schilkp/.cache/bazel/_bazel_schilkp/e4cc209945c2029dba88a1739abf7043/execroot/com_google_xls/bazel-out/k8-opt/bin/external/com_icarus_iverilog/iverilog-bin -B /home/schilkp/.cache/bazel/_bazel_schilkp/e4cc209945c2029dba88a1739abf7043/execroot/com_google_xls/bazel-out/k8-opt/bin/external/com_icarus_iverilog/ ./if_add_proc_full_tb.v -DSIMULATION -o out
/home/schilkp/.cache/bazel/_bazel_schilkp/e4cc209945c2029dba88a1739abf7043/execroot/com_google_xls/bazel-out/k8-opt/bin/external/com_icarus_iverilog/vvp ./out
gtkwave ./testbench.vcd
