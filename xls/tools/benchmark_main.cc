// Copyright 2020 The XLS Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "xls/codegen/module_signature.h"
#include "xls/codegen/pipeline_generator.h"
#include "xls/common/exit_status.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/init_xls.h"
#include "xls/common/logging/logging.h"
#include "xls/common/status/ret_check.h"
#include "xls/common/status/status_macros.h"
#include "xls/data_structures/binary_decision_diagram.h"
#include "xls/delay_model/analyze_critical_path.h"
#include "xls/delay_model/delay_estimator.h"
#include "xls/delay_model/delay_estimators.h"
#include "xls/interpreter/block_interpreter.h"
#include "xls/interpreter/function_interpreter.h"
#include "xls/interpreter/random_value.h"
#include "xls/ir/block.h"
#include "xls/ir/events.h"
#include "xls/ir/function_base.h"
#include "xls/ir/ir_parser.h"
#include "xls/ir/node.h"
#include "xls/ir/node_iterator.h"
#include "xls/ir/nodes.h"
#include "xls/ir/op.h"
#include "xls/ir/package.h"
#include "xls/ir/register.h"
#include "xls/ir/value.h"
#include "xls/ir/value_utils.h"
#include "xls/jit/block_jit.h"
#include "xls/jit/function_jit.h"
#include "xls/jit/jit_channel_queue.h"
#include "xls/jit/jit_runtime.h"
#include "xls/jit/proc_jit.h"
#include "xls/passes/bdd_function.h"
#include "xls/passes/bdd_query_engine.h"
#include "xls/passes/optimization_pass.h"
#include "xls/passes/optimization_pass_pipeline.h"
#include "xls/passes/pass_base.h"
#include "xls/passes/query_engine.h"
#include "xls/scheduling/pipeline_schedule.h"
#include "xls/scheduling/scheduling_pass.h"
#include "xls/scheduling/scheduling_pass_pipeline.h"

const char kUsage[] = R"(
Prints numerous metrics and other information about an XLS IR file including:
total delay, critical path, codegen information, optimization time, etc.

Expected invocation:
  benchmark_main <IR file>
where:
  - <IR file> is the path to the input IR file.

Example invocation:
  benchmark_main path/to/file.ir
)";

// LINT.IfChange
// TODO(meheff): These codegen flags are duplicated from codegen_main. Might be
// easier to wrap all the options into a proto or something codegen_main and
// this could consume. It would make it less likely that the benchmark and
// actual generated Verilog would diverge.
ABSL_FLAG(
    int64_t, clock_period_ps, 0,
    "The number of picoseconds in a cycle to use when generating a pipeline "
    "(codegen). Cannot be specified with --pipeline_stages. If both this "
    "flag value and --pipeline_stages are zero then codegen is not"
    "performed.");
ABSL_FLAG(
    int64_t, pipeline_stages, 0,
    "The number of stages in the generated pipeline when performing codegen. "
    "Cannot be specified with --clock_period_ps. If both this flag value and "
    "--clock_period_ps are zero then codegen is not performed.");
ABSL_FLAG(int64_t, clock_margin_percent, 0,
          "The percentage of clock period to set aside as a margin to ensure "
          "timing is met. Effectively, this lowers the clock period by this "
          "percentage amount for the purposes of scheduling. Must be specified "
          "with --clock_period_ps");
ABSL_FLAG(bool, show_known_bits, false,
          "Show known bits as determined via the query engine.");
ABSL_FLAG(std::string, top, "", "Top entity to use in lieu of the default.");
ABSL_FLAG(std::string, delay_model, "",
          "Delay model name to use from registry.");
ABSL_FLAG(int64_t, convert_array_index_to_select, -1,
          "If specified, convert array indexes with fewer than or "
          "equal to the given number of possible indices (by range analysis) "
          "into chains of selects. Otherwise, this optimization is skipped, "
          "since it can sometimes reduce output quality.");
ABSL_FLAG(bool, use_context_narrowing_analysis, false,
          "Use context sensitive narrowing analysis. This is somewhat slower "
          "but might produce better results in some circumstances by using "
          "usage context to narrow values more aggressively.");
ABSL_FLAG(std::optional<int64_t>, worst_case_throughput, std::nullopt,
          "Allow scheduling a pipeline with worst-case throughput no slower "
          "than once per N cycles. If unspecified, enforce throughput 1. Note: "
          "a higher value for --worst_case_throughput *decreases* the "
          "worst-case throughput, since this controls inverse throughput.");
ABSL_FLAG(bool, run_evaluators, true,
          "Whether to run the JIT and interpreter.");
// LINT.ThenChange(//xls/build_rules/xls_ir_rules.bzl)

namespace xls {
namespace {

std::string KnownBitString(Node* node, const QueryEngine& query_engine) {
  if (!node->GetType()->IsBits()) {
    return "?";
  }
  return query_engine.ToString(node);
}

void PrintNodeBreakdown(FunctionBase* f) {
  std::cout << absl::StreamFormat("Entry function (%s) node count: %d nodes\n",
                                  f->name(), f->node_count());
  std::vector<Op> ops;
  absl::flat_hash_map<Op, int64_t> op_count;
  for (Node* node : f->nodes()) {
    if (!op_count.contains(node->op())) {
      ops.push_back(node->op());
    }
    op_count[node->op()] += 1;
  }
  std::sort(ops.begin(), ops.end(),
            [&](Op a, Op b) { return op_count.at(a) > op_count.at(b); });
  std::cout << "Breakdown by op of all nodes in the graph:" << '\n';
  for (Op op : ops) {
    std::cout << absl::StreamFormat("  %15s : %5d (%5.2f%%)\n", OpToString(op),
                                    op_count.at(op),
                                    100.0 * op_count.at(op) / f->node_count());
  }
}

int64_t DurationToMs(absl::Duration duration) {
  return duration / absl::Milliseconds(1);
}

// Run the standard pipeline on the given package and prints stats about the
// passes and execution time.
absl::Status RunOptimizationAndPrintStats(Package* package) {
  std::unique_ptr<OptimizationCompoundPass> pipeline =
      CreateOptimizationPassPipeline();

  absl::Time start = absl::Now();
  OptimizationPassOptions pass_options;
  int64_t convert_array_index_to_select =
      absl::GetFlag(FLAGS_convert_array_index_to_select);
  pass_options.convert_array_index_to_select =
      (convert_array_index_to_select < 0)
          ? std::nullopt
          : std::make_optional(convert_array_index_to_select);
  // TODO(meheff): 2022/3/23 Add this as a flag and benchmark_ir option.
  pass_options.inline_procs = true;
  pass_options.use_context_narrowing_analysis =
      absl::GetFlag(FLAGS_use_context_narrowing_analysis);
  PassResults pass_results;
  XLS_RETURN_IF_ERROR(
      pipeline->Run(package, pass_options, &pass_results).status());
  absl::Duration total_time = absl::Now() - start;
  std::cout << absl::StreamFormat("Optimization time: %dms\n",
                                  DurationToMs(total_time));
  std::cout << absl::StreamFormat("Dynamic pass count: %d\n",
                                  pass_results.invocations.size());

  // Aggregate run times by the pass name and print a table of the aggregate
  // execution time of each pass in descending order.
  absl::flat_hash_map<std::string, absl::Duration> pass_times;
  absl::flat_hash_map<std::string, int64_t> pass_counts;
  absl::flat_hash_map<std::string, int64_t> changed_counts;
  for (const PassInvocation& invocation : pass_results.invocations) {
    pass_times[invocation.pass_name] += invocation.run_duration;
    ++pass_counts[invocation.pass_name];
    changed_counts[invocation.pass_name] += invocation.ir_changed ? 1 : 0;
  }
  std::vector<std::string> pass_names;
  for (const auto& pair : pass_times) {
    pass_names.push_back(pair.first);
  }
  std::sort(pass_names.begin(), pass_names.end(),
            [&](const std::string& a, const std::string& b) {
              // Sort by time (at the same resolution we show), breaking ties by
              // # of times run, # of times changed, and finally pass name.
              int64_t a_time = DurationToMs(pass_times.at(a));
              int64_t b_time = DurationToMs(pass_times.at(b));
              if (a_time > b_time) {
                return true;
              }
              if (a_time < b_time) {
                return false;
              }
              if (pass_counts.at(a) > pass_counts.at(b)) {
                return true;
              }
              if (pass_counts.at(a) < pass_counts.at(b)) {
                return false;
              }
              if (changed_counts.at(a) > changed_counts.at(b)) {
                return true;
              }
              if (changed_counts.at(a) < changed_counts.at(b)) {
                return false;
              }
              return a > b;
            });
  std::cout << "Pass run durations (# of times pass changed IR / # of times "
               "pass was run):"
            << '\n';
  for (const std::string& name : pass_names) {
    std::cout << absl::StreamFormat("  %-20s : %-5dms (%3d / %3d)\n", name,
                                    DurationToMs(pass_times.at(name)),
                                    changed_counts.at(name),
                                    pass_counts.at(name));
  }
  return absl::OkStatus();
}

absl::Status PrintCriticalPath(
    FunctionBase* f, const QueryEngine& query_engine,
    const DelayEstimator& delay_estimator,
    std::optional<int64_t> effective_clock_period_ps) {
  XLS_ASSIGN_OR_RETURN(
      std::vector<CriticalPathEntry> critical_path,
      AnalyzeCriticalPath(f, effective_clock_period_ps, delay_estimator));
  std::cout << absl::StrFormat("Critical path delay: %dps\n",
                               critical_path.front().path_delay_ps);
  std::cout << absl::StrFormat("Critical path entry count: %d\n",
                               critical_path.size());

  absl::flat_hash_map<Op, std::pair<int64_t, int64_t>> op_to_sum;
  std::cout << "Critical path:" << '\n';
  std::cout << CriticalPathToString(
      critical_path, [&query_engine](Node* n) -> std::string {
        if (absl::GetFlag(FLAGS_show_known_bits)) {
          return absl::StrFormat(
              "        %s <= [%s]\n", KnownBitString(n, query_engine),
              absl::StrJoin(
                  n->operands(), ", ", [&](std::string* out, Node* n) {
                    absl::StrAppend(out, KnownBitString(n, query_engine));
                  }));
        }
        return "";
      });

  for (CriticalPathEntry& entry : critical_path) {
    // Make a note of the sums.
    auto& tally = op_to_sum[entry.node->op()];
    tally.first += entry.node_delay_ps;
    tally.second += 1;
  }

  int64_t total_delay = critical_path.front().path_delay_ps;
  std::cout << absl::StrFormat("Contribution by op (total %dps):\n",
                               total_delay);
  std::vector<Op> ops = AllOps();
  std::sort(ops.begin(), ops.end(), [&](Op lhs, Op rhs) {
    return op_to_sum[lhs].first > op_to_sum[rhs].first;
  });
  for (Op op : ops) {
    if (op_to_sum[op].second == 0) {
      continue;
    }
    std::cout << absl::StreamFormat(
        " %20s: %4d (%5.2f%%, %4d nodes, %5.1f avg)\n", OpToString(op),
        op_to_sum[op].first,
        static_cast<double>(op_to_sum[op].first) / total_delay * 100.0,
        op_to_sum[op].second,
        static_cast<double>(op_to_sum[op].first) / op_to_sum[op].second);
  }
  return absl::OkStatus();
}

absl::Status PrintTotalDelay(FunctionBase* f,
                             const DelayEstimator& delay_estimator) {
  int64_t total_delay = 0;
  for (Node* node : f->nodes()) {
    XLS_ASSIGN_OR_RETURN(int64_t op_delay,
                         delay_estimator.GetOperationDelayInPs(node));
    total_delay += op_delay;
  }
  std::cout << absl::StrFormat("Total delay: %dps\n", total_delay);
  return absl::OkStatus();
}

// Returns the critical-path delay through each pipeline stage.
absl::StatusOr<std::vector<int64_t>> GetDelayPerStageInPs(
    FunctionBase* f, const PipelineSchedule& schedule,
    const DelayEstimator& delay_estimator) {
  std::vector<int64_t> delay_per_stage(schedule.length() + 1);
  // The delay from the beginning of the stage at which each node completes.
  absl::flat_hash_map<Node*, int64_t> completion_time;
  for (Node* node : TopoSort(f)) {
    int64_t start_time = 0;
    for (Node* operand : node->operands()) {
      if (schedule.cycle(operand) == schedule.cycle(node)) {
        start_time = std::max(start_time, completion_time[operand]);
      }
    }
    XLS_ASSIGN_OR_RETURN(int64_t node_delay,
                         delay_estimator.GetOperationDelayInPs(node));
    completion_time[node] = start_time + node_delay;
    delay_per_stage[schedule.cycle(node)] =
        std::max(delay_per_stage[schedule.cycle(node)], completion_time[node]);
  }
  return delay_per_stage;
}

absl::StatusOr<PipelineSchedule> ScheduleAndPrintStats(
    Package* package, const DelayEstimator& delay_estimator,
    std::optional<int64_t> clock_period_ps,
    std::optional<int64_t> pipeline_stages,
    std::optional<int64_t> clock_margin_percent,
    std::optional<int64_t> worst_case_throughput) {
  SchedulingPassOptions options;
  options.delay_estimator = &delay_estimator;
  if (clock_period_ps.has_value()) {
    options.scheduling_options.clock_period_ps(*clock_period_ps);
  }
  if (pipeline_stages.has_value()) {
    options.scheduling_options.pipeline_stages(*pipeline_stages);
  }
  if (clock_margin_percent.has_value()) {
    options.scheduling_options.clock_margin_percent(*clock_margin_percent);
  }
  if (worst_case_throughput.has_value()) {
    options.scheduling_options.worst_case_throughput(*worst_case_throughput);
  }

  std::optional<FunctionBase*> top = package->GetTop();
  if (!top.has_value()) {
    return absl::InternalError(absl::StrFormat(
        "Top entity not set for package: %s.", package->name()));
  }

  std::unique_ptr<SchedulingCompoundPass> scheduling_pipeline =
      CreateSchedulingPassPipeline();
  SchedulingPassResults results;
  auto scheduling_unit = SchedulingUnit::CreateForSingleFunction(*top);

  absl::Time start = absl::Now();
  XLS_RETURN_IF_ERROR(
      scheduling_pipeline->Run(&scheduling_unit, options, &results).status());
  absl::Duration total_time = absl::Now() - start;
  std::cout << absl::StreamFormat("Scheduling time: %dms\n",
                                  total_time / absl::Milliseconds(1));

  return std::move(scheduling_unit.schedules().at(*top));
}

absl::Status PrintCodegenInfo(FunctionBase* f,
                              const PipelineSchedule& schedule) {
  absl::Time start = absl::Now();
  XLS_ASSIGN_OR_RETURN(verilog::ModuleGeneratorResult codegen_result,
                       verilog::ToPipelineModuleText(
                           schedule, f, verilog::BuildPipelineOptions()));
  absl::Duration total_time = absl::Now() - start;
  std::cout << absl::StreamFormat("Codegen time: %dms\n",
                                  total_time / absl::Milliseconds(1));

  // TODO(meheff): Add an estimate of total number of gates.
  std::cout << absl::StreamFormat(
      "Lines of Verilog: %d\n",
      std::vector<std::string>(
          absl::StrSplit(codegen_result.verilog_text, '\n'))
          .size());

  return absl::OkStatus();
}

absl::Status PrintScheduleInfo(FunctionBase* f,
                               const PipelineSchedule& schedule,
                               const BddQueryEngine& bdd_query_engine,
                               const DelayEstimator& delay_estimator,
                               std::optional<int64_t> clock_period_ps) {
  int64_t total_flops = 0;
  int64_t total_duplicates = 0;
  int64_t total_constants = 0;
  std::vector<int64_t> flops_per_stage(schedule.length());
  std::vector<int64_t> duplicates_per_stage(schedule.length());
  std::vector<int64_t> constants_per_stage(schedule.length());
  for (int64_t i = 0; i < schedule.length(); ++i) {
    absl::flat_hash_map<BddNodeIndex, std::pair<Node*, int64_t>> bdd_nodes;
    for (Node* node : schedule.GetLiveOutOfCycle(i)) {
      flops_per_stage[i] += node->GetType()->GetFlatBitCount();
      if (node->GetType()->IsBits()) {
        for (int64_t bit_index = 0; bit_index < node->BitCountOrDie();
             ++bit_index) {
          BddNodeIndex bdd_node =
              bdd_query_engine.bdd_function().GetBddNode(node, bit_index);
          if (bdd_node == bdd_query_engine.bdd_function().bdd().zero() ||
              bdd_node == bdd_query_engine.bdd_function().bdd().one()) {
            XLS_VLOG(1) << absl::StreamFormat(
                "%s:%d in stage %d is known %s", node->GetName(), bit_index, i,
                bdd_node == bdd_query_engine.bdd_function().bdd().zero()
                    ? "zero"
                    : "one");
            constants_per_stage[i]++;
            total_constants++;
          } else if (bdd_nodes.contains(bdd_node)) {
            XLS_VLOG(1) << absl::StreamFormat(
                "%s:%d in stage %d duplicate of %s:%d", node->GetName(),
                bit_index, i, bdd_nodes.at(bdd_node).first->GetName(),
                bdd_nodes.at(bdd_node).second);
            duplicates_per_stage[i]++;
            total_duplicates++;
          } else {
            bdd_nodes[bdd_node] = {node, bit_index};
          }
        }
      }
    }
    total_flops += flops_per_stage[i];
  }

  XLS_ASSIGN_OR_RETURN(std::vector<int64_t> delay_per_stage,
                       GetDelayPerStageInPs(f, schedule, delay_estimator));

  // TODO(tedhong) 2023-03-06 - Add functionality to report I/O flop count.
  std::cout << "Pipeline:\n";
  for (int64_t i = 0; i < schedule.length(); ++i) {
    std::string stage_str = absl::StrFormat(
        "  [Stage %2d] flops: %4d (%4d dups, %4d constant)\n", i,
        flops_per_stage[i], duplicates_per_stage[i], constants_per_stage[i]);

    // Horizontally offset the information about the logic in the stage from
    // the information about stage registers to make it easier to scan the
    // information vertically without register and logic info intermixing.
    std::cout << std::string(stage_str.size(), ' ');
    std::cout << absl::StreamFormat("nodes: %4d, delay: %4dps\n",
                                    schedule.nodes_in_cycle(i).size(),
                                    delay_per_stage[i]);

    if (i + 1 != schedule.length()) {
      std::cout << stage_str;
    }
  }
  std::cout << absl::StreamFormat(
      "Total pipeline flops: %d (%d dups, %4d constant)\n", total_flops,
      total_duplicates, total_constants);

  if (clock_period_ps.has_value()) {
    int64_t min_slack = std::numeric_limits<int64_t>::max();
    for (int64_t stage_delay : delay_per_stage) {
      min_slack = std::min(min_slack, *clock_period_ps - stage_delay);
    }
    std::cout << absl::StreamFormat("Min stage slack: %d\n", min_slack);
  }

  return absl::OkStatus();
}

absl::Status PrintProcInfo(Proc* p) {
  XLS_RET_CHECK(p != nullptr);

  int64_t total_flops = 0;
  for (Param* param : p->StateParams()) {
    total_flops += param->GetType()->GetFlatBitCount();
  }

  std::cout << absl::StreamFormat("Total state flops: %d\n", total_flops);

  return absl::OkStatus();
}

// Invokes `f` until (approximately) at least`duration_ms` milliseconds have
// passed and returns the number of calls per second.
absl::StatusOr<float> CountRate(const std::function<void()>& f,
                                int64_t duration_ms) {
  // To avoid including absl::Now() calls in the time measurement, first
  // estimate how many calls it will take for `duration_ms` milliseconds to
  // elapse. This is done my running until `duration_ms / 10` milliseconds have
  // elapsed with absl::Now() in the loop.
  int64_t call_count = 0;
  absl::Time start_estimate = absl::Now();
  while (DurationToMs(absl::Now() - start_estimate) < duration_ms / 10) {
    f();
    ++call_count;
  }

  // Then run 10x the estimated call count to get close to `duration_ms` total
  // run time.
  absl::Time start_actual = absl::Now();
  for (int64_t i = 0; i < call_count * 10; ++i) {
    f();
  }
  int64_t elapsed_ms = DurationToMs(absl::Now() - start_actual);
  return elapsed_ms == 0 ? 0.0f : (1000.0 * call_count) / elapsed_ms;
}

template <typename ParamNode, typename Rng>
std::vector<std::vector<Value>> GetRandomParams(
    FunctionBase* function, int64_t input_count,
    absl::Span<ParamNode* const> params, Rng& rng_engine) {
  std::vector<std::vector<Value>> arg_set(input_count);
  for (std::vector<Value>& args : arg_set) {
    args.reserve(params.size());
    for (Node* param : params) {
      args.push_back(RandomValue(param->GetType(), rng_engine));
    }
  }
  return arg_set;
}

struct JitArguments {
  std::vector<std::vector<std::vector<uint8_t>>> arg_buffers;
  std::vector<std::vector<uint8_t*>> arg_pointers;
};

absl::StatusOr<JitArguments> ConvertToJitArguments(
    const std::vector<std::vector<Value>>& arg_set,
    absl::Span<Type* const> params, JitRuntime* runtime) {
  std::vector<std::vector<std::vector<uint8_t>>> arg_buffers;
  std::vector<std::vector<uint8_t*>> arg_pointers;
  for (const std::vector<Value>& args : arg_set) {
    std::vector<std::vector<uint8_t>> buffers;
    std::vector<uint8_t*> pointers;
    buffers.reserve(args.size());
    pointers.reserve(args.size());
    for (int64_t i = 0; i < args.size(); ++i) {
      buffers.push_back(
          std::vector<uint8_t>(runtime->ShouldAllocateForAlignment(
                                   runtime->GetTypeByteSize(params[i]),
                                   runtime->GetTypeAlignment(params[i])),
                               0));
      pointers.push_back(runtime
                             ->AsAligned(absl::MakeSpan(buffers.back()),
                                         runtime->GetTypeAlignment(params[i]))
                             .data());
      XLS_CHECK_NE(pointers.back(), nullptr);
    }
    arg_buffers.push_back(std::move(buffers));
    arg_pointers.push_back(pointers);
    XLS_RETURN_IF_ERROR(runtime->PackArgs(args, params, arg_pointers.back()));
  }
  return JitArguments{std::move(arg_buffers), std::move(arg_pointers)};
}

// Run the interpreter/JIT for a fixed amount of time and measure the rate of
// calls per second.
constexpr int64_t kRunDurationMs = 500;
constexpr int64_t kJitRunMultiplier = 1000;

template <typename Rng>
absl::Status RunFunctionInterpreterAndJit(Function* function,
                                          std::string_view description,
                                          Rng& rng_engine) {
  absl::Time start_jit_compile = absl::Now();
  XLS_ASSIGN_OR_RETURN(std::unique_ptr<FunctionJit> jit,
                       FunctionJit::Create(function));
  std::cout << absl::StreamFormat(
      "JIT compile time (%s): %dms\n", description,
      DurationToMs(absl::Now() - start_jit_compile));

  const int64_t kInputCount = 100;
  auto arg_set =
      GetRandomParams(function, kInputCount, function->params(), rng_engine);

  // To avoid being dominated by xls::Value conversion to native
  // format, preconvert the arguments.
  XLS_ASSIGN_OR_RETURN(
      JitArguments jit_args,
      ConvertToJitArguments(arg_set, function->GetType()->parameters(),
                            jit->runtime()));
  auto [jit_arg_buffers, jit_arg_pointers] = std::move(jit_args);

  // The JIT is much faster so run many times.
  InterpreterEvents events;
  std::vector<uint8_t> result_buffer(jit->runtime()->ShouldAllocateForAlignment(
      jit->GetReturnTypeSize(), jit->GetReturnTypeAlignment()));
  absl::Span<uint8_t> result_aligned = jit->runtime()->AsAligned(
      absl::MakeSpan(result_buffer), jit->GetReturnTypeAlignment());
  XLS_ASSIGN_OR_RETURN(
      float jit_run_rate,
      CountRate(
          [&]() -> absl::Status {
            for (int64_t i = 0; i < kJitRunMultiplier; ++i) {
              for (const std::vector<uint8_t*>& pointers : jit_arg_pointers) {
                XLS_CHECK_OK(
                    jit->RunWithViews(pointers, result_aligned, &events));
              }
            }
            return absl::OkStatus();
          },
          kRunDurationMs));
  std::cout << absl::StreamFormat(
      "JIT run time (%s): %d Kcalls/s\n", description,
      static_cast<int64_t>(kInputCount * jit_run_rate));

  XLS_ASSIGN_OR_RETURN(
      float interpreter_run_rate,
      CountRate(
          [&]() -> absl::Status {
            for (const std::vector<Value>& args : arg_set) {
              XLS_CHECK_OK(InterpretFunction(function, args).status());
            }
            return absl::OkStatus();
          },
          kRunDurationMs));
  std::cout << absl::StreamFormat(
      "Interpreter run time (%s): %d calls/s\n", description,
      static_cast<int64_t>(kInputCount * interpreter_run_rate));
  return absl::OkStatus();
}

template <typename Rng>
absl::Status RunBlockInterpreterAndJit(Block* block,
                                       std::string_view description,
                                       Rng& rng_engine) {
  absl::Time start_jit_compile = absl::Now();
  XLS_ASSIGN_OR_RETURN(std::unique_ptr<JitRuntime> runtime,
                       JitRuntime::Create());
  XLS_ASSIGN_OR_RETURN(std::unique_ptr<BlockJit> jit,
                       BlockJit::Create(block, runtime.get()));
  std::cout << absl::StreamFormat(
      "JIT compile time (%s): %dms\n", description,
      DurationToMs(absl::Now() - start_jit_compile));

  const int64_t kInputCount = 100;
  std::vector<std::vector<Value>> arg_set =
      GetRandomParams(block, kInputCount, block->GetInputPorts(), rng_engine);
  std::vector<absl::flat_hash_map<std::string, Value>> port_set;
  port_set.reserve(arg_set.size());
  absl::c_transform(arg_set, std::back_inserter(port_set),
                    [&](const std::vector<Value>& ports) {
                      absl::flat_hash_map<std::string, Value> out;
                      out.reserve(ports.size());
                      for (int i = 0; i < ports.size(); ++i) {
                        out[block->GetInputPorts()[i]->name()] = ports[i];
                      }
                      return out;
                    });

  // To avoid being dominated by xls::Value conversion to native
  // format, preconvert the arguments.
  std::vector<Type*> input_types;
  input_types.reserve(block->GetInputPorts().size());
  absl::c_transform(block->GetInputPorts(), std::back_inserter(input_types),
                    [](InputPort* v) { return v->GetType(); });
  XLS_ASSIGN_OR_RETURN(auto jit_args, ConvertToJitArguments(
                                          arg_set, input_types, runtime.get()));
  auto [jit_arg_buffers, jit_arg_pointers] = std::move(jit_args);

  // The JIT is much faster so run many times.
  auto jit_continuation = jit->NewContinuation();
  XLS_ASSIGN_OR_RETURN(
      float jit_run_rate,
      CountRate(
          [&]() -> absl::Status {
            for (int64_t i = 0; i < kJitRunMultiplier; ++i) {
              for (const std::vector<uint8_t*>& pointers : jit_arg_pointers) {
                XLS_CHECK_OK(jit_continuation->SetInputPorts(pointers));
                XLS_CHECK_OK(jit->RunOneCycle(*jit_continuation));
              }
            }
            return absl::OkStatus();
          },
          kRunDurationMs));
  std::cout << absl::StreamFormat(
      "JIT run time (%s): %d Kcalls/s\n", description,
      static_cast<int64_t>(kInputCount * jit_run_rate));

  absl::flat_hash_map<std::string, Value> interpreter_regs;
  interpreter_regs.reserve(block->GetRegisters().size());
  for (Register* r : block->GetRegisters()) {
    interpreter_regs[r->name()] = ZeroOfType(r->type());
  }
  XLS_ASSIGN_OR_RETURN(
      float interpreter_run_rate,
      CountRate(
          [&]() -> absl::Status {
            for (const absl::flat_hash_map<std::string, Value>& ports :
                 port_set) {
              XLS_CHECK_OK(kInterpreterBlockEvaluator
                               .EvaluateBlock(ports, interpreter_regs, block)
                               .status());
            }
            return absl::OkStatus();
          },
          kRunDurationMs));
  std::cout << absl::StreamFormat(
      "Interpreter run time (%s): %d calls/s\n", description,
      static_cast<int64_t>(kInputCount * interpreter_run_rate));
  return absl::OkStatus();
}

template <typename Rng>
absl::Status RunProcInterpreterAndJit(Proc* proc, std::string_view description,
                                      Rng& rng_engine) {
  absl::Time start_jit_compile = absl::Now();
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<JitChannelQueueManager> queue_manager,
      JitChannelQueueManager::CreateThreadSafe(proc->package()));
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<ProcJit> jit,
      ProcJit::Create(proc, &queue_manager->runtime(), queue_manager.get()));
  std::cout << absl::StreamFormat(
      "JIT compile time (%s): %dms\n", description,
      DurationToMs(absl::Now() - start_jit_compile));
  // TODO(meheff): 2022/5/16 Run the proc as well.

  return absl::OkStatus();
}

absl::Status RunInterpreterAndJit(FunctionBase* function_base,
                                  std::string_view description) {
  std::minstd_rand rng_engine;
  if (function_base->IsFunction()) {
    Function* function = function_base->AsFunctionOrDie();
    return RunFunctionInterpreterAndJit(function, description, rng_engine);
  }

  if (function_base->IsBlock()) {
    Block* block = function_base->AsBlockOrDie();
    return RunBlockInterpreterAndJit(block, description, rng_engine);
  }

  XLS_RET_CHECK(function_base->IsProc());
  Proc* proc = function_base->AsProcOrDie();
  return RunProcInterpreterAndJit(proc, description, rng_engine);
}

absl::Status RealMain(std::string_view path,
                      std::optional<int64_t> clock_period_ps,
                      std::optional<int64_t> pipeline_stages,
                      std::optional<int64_t> clock_margin_percent,
                      std::optional<int64_t> worst_case_throughput) {
  XLS_VLOG(1) << "Reading contents at path: " << path;
  XLS_ASSIGN_OR_RETURN(std::string contents, GetFileContents(path));
  XLS_ASSIGN_OR_RETURN(std::unique_ptr<Package> package,
                       Parser::ParsePackage(contents));
  if (!absl::GetFlag(FLAGS_top).empty()) {
    XLS_RETURN_IF_ERROR(package->SetTopByName(absl::GetFlag(FLAGS_top)));
  }

  if (!package->GetTop().has_value()) {
    return absl::InternalError(absl::StrFormat(
        "Top entity not set for package: %s.", package->name()));
  }
  if (absl::GetFlag(FLAGS_run_evaluators)) {
    XLS_RETURN_IF_ERROR(
        RunInterpreterAndJit(package->GetTop().value(), "unoptimized"));
  }
  XLS_RETURN_IF_ERROR(RunOptimizationAndPrintStats(package.get()));

  FunctionBase* f = package->GetTop().value();
  BddQueryEngine query_engine(BddFunction::kDefaultPathLimit);
  XLS_RETURN_IF_ERROR(query_engine.Populate(f).status());
  PrintNodeBreakdown(f);

  std::optional<int64_t> effective_clock_period_ps;
  if (clock_period_ps.has_value()) {
    effective_clock_period_ps = clock_period_ps;
    if (clock_margin_percent.has_value()) {
      effective_clock_period_ps =
          *effective_clock_period_ps -
          (*clock_period_ps * *clock_margin_percent + 50) / 100;
    }
  }
  const DelayEstimator* pdelay_estimator;
  if (absl::GetFlag(FLAGS_delay_model).empty()) {
    pdelay_estimator = &GetStandardDelayEstimator();
  } else {
    XLS_ASSIGN_OR_RETURN(pdelay_estimator,
                         GetDelayEstimator(absl::GetFlag(FLAGS_delay_model)));
  }
  const auto& delay_estimator = *pdelay_estimator;
  XLS_RETURN_IF_ERROR(PrintCriticalPath(f, query_engine, delay_estimator,
                                        effective_clock_period_ps));
  XLS_RETURN_IF_ERROR(PrintTotalDelay(f, delay_estimator));

  if (absl::GetFlag(FLAGS_run_evaluators)) {
    XLS_RETURN_IF_ERROR(RunInterpreterAndJit(f, "optimized"));
  }

  const bool benchmark_codegen =
      clock_period_ps.has_value() || pipeline_stages.has_value();
  if (benchmark_codegen) {
    XLS_ASSIGN_OR_RETURN(
        PipelineSchedule schedule,
        ScheduleAndPrintStats(package.get(), delay_estimator, clock_period_ps,
                              pipeline_stages, clock_margin_percent,
                              worst_case_throughput));

    // Only print codegen info for functions.
    //
    // TODO(tedhong): 2022-09-28 - Support passing additional codegen options
    // to benchmark_main to be able to codegen procs.
    if (f->IsFunction()) {
      XLS_RETURN_IF_ERROR(PrintCodegenInfo(f, schedule));
    }

    XLS_RETURN_IF_ERROR(PrintScheduleInfo(f, schedule, query_engine,
                                          delay_estimator, clock_period_ps));

    // Print out state information for procs.
    if (f->IsProc()) {
      XLS_RETURN_IF_ERROR(PrintProcInfo(f->AsProcOrDie()));
    }
    // TODO(allight): 2023-10-30 - Until we're able to codegen procs we cannot
    // get execution stats for them.
    if (absl::GetFlag(FLAGS_run_evaluators) && f->IsFunction()) {
      XLS_RETURN_IF_ERROR(
          RunInterpreterAndJit(package->blocks()[0].get(), "block"));
    }
  }

  return absl::OkStatus();
}

}  // namespace
}  // namespace xls

int main(int argc, char** argv) {
  std::vector<std::string_view> positional_arguments =
      xls::InitXls(kUsage, argc, argv);

  if (positional_arguments.empty() || positional_arguments[0].empty()) {
    XLS_LOG(QFATAL) << "Expected path argument with IR: " << argv[0]
                    << " <ir_path>";
  }

  std::optional<int64_t> clock_period_ps;
  if (absl::GetFlag(FLAGS_clock_period_ps) > 0) {
    clock_period_ps = absl::GetFlag(FLAGS_clock_period_ps);
  }
  std::optional<int64_t> pipeline_stages;
  if (absl::GetFlag(FLAGS_pipeline_stages) > 0) {
    pipeline_stages = absl::GetFlag(FLAGS_pipeline_stages);
  }
  std::optional<int64_t> clock_margin_percent;
  if (absl::GetFlag(FLAGS_clock_margin_percent) > 0) {
    clock_margin_percent = absl::GetFlag(FLAGS_clock_margin_percent);
  }
  return xls::ExitStatus(xls::RealMain(
      positional_arguments[0], clock_period_ps, pipeline_stages,
      clock_margin_percent, absl::GetFlag(FLAGS_worst_case_throughput)));
}
