#!/bin/bash
KEEP_TEMPS=1 ../run_xls.py ./if_add_s32.x if_add --sim --sim_output_channel_counts="if_add_s32__output=15" --sim_channel_values_file=if_add_s32.inp --cg_delay_mode=asap7 --cg_clock_period_ps=6000 --cg_worst_case_throughput=16
