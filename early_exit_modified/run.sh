#!/bin/bash
python3 ../run.py test.x user_module \
  --cg_clock_period_ps 1298 \
  --cg_delay_model asap7 \
  --cg_worst_case_throughput 16 \
  --cg_flop_inputs false \
  --cg_flop_outputs false \
  --output_dir=. \
  --sched_printer_viz \
  --sim \
  --sim_channel_values_file=sim_inputs \
  --sim_output_channel_counts="test__output=15"

