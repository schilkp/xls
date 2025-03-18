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


def cmd(short_name: str, cmd: List[str], allow_fail: bool = False, dump_stdout_to=None):
    try:
        print(
            f"{COLOR_CYAN}[{short_name}]: {pretty_print_cmd(cmd)} {COLOR_NC}")
        p = subprocess.run(cmd, capture_output=True, text=True)
        if (len(p.stderr) != 0):
            print(p.stderr)
        if p.returncode != 0:
            print(f"\n")
            print(
                f"{COLOR_RED}[{short_name}]: Error. Return code: {p.returncode}{COLOR_NC}")
            if not allow_fail:
                raise Exception(f"CMD error ({p.returncode}).")
        else:
            print(f"{COLOR_GREEN}[{short_name}]: OK{COLOR_NC}")
            if dump_stdout_to:
                with open(dump_stdout_to, "w") as outfile:
                    outfile.write(p.stdout)
    except KeyboardInterrupt:
        if not allow_fail:
            raise Exception(f"CMD error (Interrupted).")
        else:
            print(f"{COLOR_CYAN}[{short_name}]: ..Interrupted. {COLOR_NC}")


script_path = os.path.dirname(__file__)


def bazel_bin(name: List[str]) -> str:
    return os.path.join(script_path, "bazel-bin", *name)


parser = argparse.ArgumentParser(prog='run_xls')
parser.add_argument("input_file")
parser.add_argument("top_module")
parser.add_argument("--output_dir", required=False)
parser.add_argument("--ir_viz", action="store_true")
parser.add_argument("--opt_ir_viz", action="store_true")
parser.add_argument("--sched_ir_viz", action="store_true")
parser.add_argument("--sched_printer_viz", action="store_true")
parser.add_argument("--cg_pipeline_stages", required=False)
parser.add_argument("--cg_clock_period_ps", required=False)
parser.add_argument("--cg_worst_case_throughput", required=False)
parser.add_argument("--cg_delay_model", required=False, default="unit")
parser.add_argument("--cg_reset", required=False, default="rst")
parser.add_argument("--cg_flop_inputs", required=False)
parser.add_argument("--cg_flop_inputs_kind", required=False)
parser.add_argument("--cg_flop_outputs", required=False)
parser.add_argument("--cg_flop_outputs_kind", required=False)


group_sim_func = parser.add_argument_group("simulate")
group_sim_func.add_argument("--sim", action="store_true")
group_sim_func.add_argument("--sim_args", required=False)
group_sim_func.add_argument("--sim_channel_values_file", required=False)
group_sim_func.add_argument("--sim_output_channel_counts", required=False)

args = parser.parse_args()


def out_file(top_name: str, ext: str) -> str:
    if args.output_dir:
        return os.path.join(args.output_dir, top_name + "." + ext)
    else:
        return os.path.join("/tmp", top_name + "." + ext)


# Run interpreter:
cmd("INTERP", [bazel_bin(["xls", "dslx", "interpreter_main"]),
    args.input_file, "--alsologtostderr"])

# Generate IR:
ir_file = out_file(args.top_module, "ir")
cmd("GEN_IR", [bazel_bin(["xls", "dslx", "ir_convert", "ir_converter_main"]),
    f"--top={args.top_module}", f"--output_file={ir_file}", args.input_file])

# IR viz:
if args.ir_viz:
    cmd("IR_VIZ", [bazel_bin(["xls", "visualization", "ir_viz", "app"]),
                   f"--delay_model={args.cg_delay_model}",
                   f"--preload_ir_path={ir_file}"], allow_fail=True)

# Optimize IR:
ir_opt_file = out_file(args.top_module, "opt.ir")
cmd("OPT_IR", [bazel_bin(["xls", "tools", "opt_main"]),
    f"--output_path={ir_opt_file}", ir_file])

# IR viz:
if args.opt_ir_viz:
    cmd("OPT_IR_VIZ", [bazel_bin(["xls", "visualization", "ir_viz", "app"]),
                       f"--delay_model={args.cg_delay_model}",
                       f"--preload_ir_path={ir_opt_file}"], allow_fail=True)

# Codegen:
verilog_file = out_file(args.top_module, "v")
signature_file = out_file(args.top_module, "sig.proto")
schedule_ir_file = out_file(args.top_module, "sched.ir")
schedule_proto_file = out_file(args.top_module, "sched.proto")

additional_cg_args = []

if args.cg_pipeline_stages:
    additional_cg_args.append(f"--pipeline_stages={args.cg_pipeline_stages}")

if args.cg_clock_period_ps:
    additional_cg_args.append(f"--clock_period_ps={args.cg_clock_period_ps}")

if args.cg_worst_case_throughput:
    additional_cg_args.append(
        f"--worst_case_throughput={args.cg_worst_case_throughput}")

if args.cg_reset and len(args.cg_reset.strip()) > 0:
    additional_cg_args.append(f"--reset={args.cg_reset}")

if args.cg_flop_inputs:
    additional_cg_args.append(f"--flop_inputs={args.cg_flop_inputs}")

if args.cg_flop_inputs_kind:
    additional_cg_args.append(f"--flop_inputs_kind={args.cg_flop_inputs_kind}")

if args.cg_flop_outputs:
    additional_cg_args.append(f"--flop_outputs={args.cg_flop_outputs}")

if args.cg_flop_outputs_kind:
    additional_cg_args.append(
        f"--flop_outputs_kind={args.cg_flop_outputs_kind}")

cmd("CODEG", [bazel_bin(["xls", "tools", "codegen_main"]),
    f"--delay_model={args.cg_delay_model}",
    f"--output_signature_path={signature_file}",
    f"--output_schedule_ir_path={schedule_ir_file}",
    f"--output_schedule_path={schedule_proto_file}",
    f"--output_verilog_path={verilog_file}",
    f"--multi_proc",
    f"--use_system_verilog=false",
    ir_opt_file,
              *additional_cg_args
              ])

# IR viz:
if args.sched_ir_viz:
    cmd("SCHED_IR_VIZ", [bazel_bin(["xls", "visualization", "ir_viz", "app"]),
                         f"--delay_model={args.cg_delay_model}",
                         f"--preload_ir_path={schedule_ir_file}"], allow_fail=True)

# IR sched printer viz:
if args.sched_printer_viz:
    sched_dot_file = out_file(args.top_module, "sched.dot")
    additional_viz_args = []
    if args.cg_pipeline_stages:
        additional_viz_args.append(
            f"--pipeline_stages={args.cg_pipeline_stages}")
    if args.cg_clock_period_ps:
        additional_viz_args.append(
            f"--clock_period_ps={args.cg_clock_period_ps}")
    if args.cg_worst_case_throughput:
        additional_viz_args.append(
            f"--worst_case_throughput={args.cg_worst_case_throughput}")

    cmd("SCHED_PRINTER_VIZ", [bazel_bin(["xls", "visualization", "sched_printer_main",]),
                              f"--delay_model={args.cg_delay_model}",
                              *additional_viz_args,
                              ir_opt_file], allow_fail=True, dump_stdout_to=sched_dot_file)

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
