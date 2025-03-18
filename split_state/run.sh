#!/bin/bash
python3 ../run.py test.x user_module --cg_clock_period_ps 882 --cg_delay_model asap7 --cg_worst_case_throughput=2 --output_dir=. --sched_printer_viz --sim --sim_channel_values_file=sim_inputs --sim_output_channel_count="test__output_l=10,test__output_r=10"
