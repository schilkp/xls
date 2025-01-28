#!/bin/python3

import argparse
import os.path
import subprocess
from typing import List

COLOR_RED = '\033[0;31m'
COLOR_GREEN = '\033[0;32m'
COLOR_CYAN = '\033[0;36m'
COLOR_NC = '\033[0m'


def pretty_print_cmd(cmd: List[str]) -> str:
    pieces = []
    for piece in cmd:
        if " " in piece:
            pieces.append(f"\"{piece}\"")
        else:
            pieces.append(piece)

    return " ".join(pieces)


def cmd(short_name: str, cmd: List[str]):
    print(f"{COLOR_CYAN}[{short_name}]: {pretty_print_cmd(cmd)} {COLOR_NC}")
    p = subprocess.Popen(cmd)
    p.communicate()
    if p.returncode != 0:
        print(f"\n")
        print(
            f"{COLOR_RED}[{short_name}]: Error. Return code: {p.returncode}{COLOR_NC}")
        raise Exception(f"CMD error ({p.returncode}).")
    else:
        print(f"{COLOR_GREEN}[{short_name}]: OK{COLOR_NC}")


script_path = os.path.dirname(__file__)


def bazel_bin(name: List[str]) -> str:
    return os.path.join(script_path, "bazel-bin", *name)


def tmp_file(top_name: str, ext: str) -> str:
    return os.path.join("/tmp", top_name + "." + ext)


parser = argparse.ArgumentParser(prog='run_xls')
parser.add_argument("input_file")
parser.add_argument("top_module")
parser.add_argument("--ir_viz", action="store_true")
parser.add_argument("--opt_ir_viz", action="store_true")
parser.add_argument("--cg_reset", required=False, default="reset")
parser.add_argument("--cg_pipeline_stages", required=False)
parser.add_argument("--cg_clock_period_ps", required=False)
parser.add_argument("--cg_worst_case_throughput", required=False)
parser.add_argument("--cg_delay_model", required=False, default="unit")

group_sim_func = parser.add_argument_group("simulate")
group_sim_func.add_argument("--sim", action="store_true")
group_sim_func.add_argument("--sim_args", required=False)
group_sim_func.add_argument("--sim_channel_values_file", required=False)
group_sim_func.add_argument("--sim_output_channel_counts", required=False)

args = parser.parse_args()

# Run interpreter:
cmd("INTERP", [bazel_bin(["xls", "dslx", "interpreter_main"]),
    args.input_file, "--alsologtostderr"])

# Generate IR:
ir_file = tmp_file(args.top_module, "ir")
cmd("GEN_IR", [bazel_bin(["xls", "dslx", "ir_convert", "ir_converter_main"]),
    f"--top={args.top_module}", f"--output_file={ir_file}", args.input_file])

# IR viz:
if args.ir_viz:
    cmd("IR_VIZ", [bazel_bin(["xls", "visualization", "ir_viz", "app"]),
                   f"--delay_model={args.cg_delay_model}",
                   f"--preload_ir_path={ir_file}"])

# Optimize IR:
ir_opt_file = tmp_file(args.top_module, "opt.ir")
cmd("OPT_IR", [bazel_bin(["xls", "tools", "opt_main"]),
    f"--output_path={ir_opt_file}", ir_file])

# IR viz:
if args.opt_ir_viz:
    cmd("OPT_IR_VIZ", [bazel_bin(["xls", "visualization", "ir_viz", "app"]),
                       f"--delay_model={args.cg_delay_model}",
                       f"--preload_ir_path={ir_opt_file}"])

# Codegen:
verilog_file = tmp_file(args.top_module, "v")
signature_file = tmp_file(args.top_module, "sig.proto")

additional_cg_args = []

if args.cg_pipeline_stages:
    additional_cg_args.append(f"--pipeline_stages={args.cg_pipeline_stages}")

if args.cg_clock_period_ps:
    additional_cg_args.append(f"--clock_period_ps={args.cg_clock_period_ps}")

if args.cg_worst_case_throughput:
    additional_cg_args.append(f"--worst_case_throughput={args.cg_worst_case_throughput}")

cmd("CODEG", [bazel_bin(["xls", "tools", "codegen_main"]),
    f"--reset={args.cg_reset}",
    f"--delay_model={args.cg_delay_model}",
    f"--output_signature_path={signature_file}",
    f"--output_verilog_path={verilog_file}",
    f"--multi_proc",
    f"--use_system_verilog=false",
    ir_opt_file,
              *additional_cg_args
              ])

# Sim
if args.sim:

    sim_additonal_args = []

    if args.sim_args:
        sim_additonal_args.append(f"--args={args.sim_args}")

    if args.sim_channel_values_file:
        sim_additonal_args.append(
            f"--channel_values_file={args.sim_channel_values_file}")

    if args.sim_output_channel_counts:
        sim_additonal_args.append(
            f"--output_channel_counts={args.sim_output_channel_counts}")

    cmd("SIM", [bazel_bin(["xls", "tools", "simulate_module_main"]),
        f"--signature_file={signature_file}",
        verilog_file,
        f"--alsologtostderr=true", *sim_additonal_args])

