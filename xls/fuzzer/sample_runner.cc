// Copyright 2023 The XLS Authors
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

#include "xls/fuzzer/sample_runner.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>  // NOLINT
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/log/vlog_is_on.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "re2/re2.h"
#include "xls/common/file/filesystem.h"
#include "xls/common/file/get_runfile_path.h"
#include "xls/common/logging/log_lines.h"
#include "xls/common/revision.h"
#include "xls/common/status/ret_check.h"
#include "xls/common/status/status_macros.h"
#include "xls/common/stopwatch.h"
#include "xls/common/subprocess.h"
#include "xls/dslx/bytecode/bytecode.h"
#include "xls/dslx/bytecode/bytecode_emitter.h"
#include "xls/dslx/bytecode/bytecode_interpreter.h"
#include "xls/dslx/bytecode/proc_hierarchy_interpreter.h"
#include "xls/dslx/channel_direction.h"
#include "xls/dslx/create_import_data.h"
#include "xls/dslx/frontend/ast.h"
#include "xls/dslx/frontend/module.h"
#include "xls/dslx/import_data.h"
#include "xls/dslx/interp_value.h"
#include "xls/dslx/interp_value_utils.h"
#include "xls/dslx/parse_and_typecheck.h"
#include "xls/dslx/type_system/type.h"
#include "xls/dslx/type_system/type_info.h"
#include "xls/dslx/virtualizable_file_system.h"
#include "xls/dslx/warning_kind.h"
#include "xls/fuzzer/cpp_sample_runner.h"
#include "xls/fuzzer/sample.h"
#include "xls/fuzzer/sample.pb.h"
#include "xls/ir/format_preference.h"
#include "xls/ir/ir_parser.h"
#include "xls/ir/value.h"
#include "xls/public/runtime_build_actions.h"
#include "xls/simulation/check_simulator.h"
#include "xls/tests/testvector.pb.h"
#include "xls/tools/eval_utils.h"

// These are used to forward, but also see comment below.
ABSL_DECLARE_FLAG(int32_t, v);
ABSL_DECLARE_FLAG(std::string, vmodule);

namespace xls {

namespace {

using ArgsBatch = std::vector<std::vector<dslx::InterpValue>>;

// clang-format off
static constexpr struct BinaryPaths {
  std::string_view codegen_main       = "xls/tools/codegen_main";
  std::string_view eval_ir_main       = "xls/tools/eval_ir_main";
  std::string_view eval_proc_main     = "xls/tools/eval_proc_main";
  std::string_view ir_converter_main  = "xls/dslx/ir_convert/ir_converter_main";
  std::string_view ir_opt_main        = "xls/tools/opt_main";
  std::string_view simulate_module_main = "xls/tools/simulate_module_main";
  std::string_view xls_translate_main  = "xls/contrib/mlir/xls_translate";
} kBinary;
// clang-format on

absl::StatusOr<ArgsBatch> ConvertFunctionKwargs(
    const dslx::Function* f, const dslx::ImportData& import_data,
    const dslx::TypecheckedModule& tm, const ArgsBatch& args_batch) {
  XLS_ASSIGN_OR_RETURN(dslx::FunctionType * fn_type,
                       tm.type_info->GetItemAs<dslx::FunctionType>(f));
  ArgsBatch converted_args;
  converted_args.reserve(args_batch.size());
  for (const std::vector<dslx::InterpValue>& unsigned_args : args_batch) {
    XLS_ASSIGN_OR_RETURN(std::vector<dslx::InterpValue> args,
                         dslx::SignConvertArgs(*fn_type, unsigned_args));
    converted_args.push_back(std::move(args));
  }
  return converted_args;
}

absl::StatusOr<std::vector<dslx::InterpValue>> RunFunctionBatched(
    const dslx::Function& f, dslx::ImportData& import_data,
    const dslx::TypecheckedModule& tm, const ArgsBatch& args_batch) {
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<dslx::BytecodeFunction> bf,
      dslx::BytecodeEmitter::Emit(&import_data, tm.type_info, f,
                                  /*caller_bindings=*/{}));
  std::vector<dslx::InterpValue> results;
  results.reserve(args_batch.size());
  for (const std::vector<dslx::InterpValue>& args : args_batch) {
    XLS_ASSIGN_OR_RETURN(
        dslx::InterpValue result,
        dslx::BytecodeInterpreter::Interpret(&import_data, bf.get(), args));
    results.push_back(result);
  }
  return results;
}

absl::StatusOr<std::vector<dslx::InterpValue>> InterpretDslxFunction(
    std::string_view text, std::string_view top_name,
    const ArgsBatch& args_batch, const std::filesystem::path& run_dir) {
  dslx::ImportData import_data = dslx::CreateImportData(
      GetDefaultDslxStdlibPath(),
      /*additional_search_paths=*/{}, dslx::kDefaultWarningsSet,
      std::make_unique<dslx::RealFilesystem>());
  XLS_ASSIGN_OR_RETURN(
      dslx::TypecheckedModule tm,
      dslx::ParseAndTypecheck(text, "sample.x", "sample", &import_data));

  std::optional<dslx::ModuleMember*> module_member =
      tm.module->FindMemberWithName(top_name);
  CHECK(module_member.has_value());
  dslx::ModuleMember* member = module_member.value();
  CHECK(std::holds_alternative<dslx::Function*>(*member));
  dslx::Function* f = std::get<dslx::Function*>(*member);
  XLS_RET_CHECK(f != nullptr);

  XLS_ASSIGN_OR_RETURN(ArgsBatch converted_args_batch,
                       ConvertFunctionKwargs(f, import_data, tm, args_batch));
  XLS_ASSIGN_OR_RETURN(
      std::vector<dslx::InterpValue> results,
      RunFunctionBatched(*f, import_data, tm, converted_args_batch));
  XLS_ASSIGN_OR_RETURN(std::vector<Value> ir_results,
                       dslx::InterpValue::ConvertValuesToIr(results));
  std::string serialized_results = absl::StrCat(
      absl::StrJoin(ir_results, "\n",
                    [](std::string* out, const Value& value) {
                      absl::StrAppend(out,
                                      value.ToString(FormatPreference::kHex));
                    }),
      "\n");
  XLS_RETURN_IF_ERROR(
      SetFileContents(run_dir / "sample.x.results", serialized_results));
  return results;
}

absl::StatusOr<std::string> RunCommandFromExecutable(
    std::string_view executable_name, std::vector<std::string> args,
    const std::filesystem::path& run_dir, const SampleOptions& options,
    const std::string_view execution_disambiq) {
  XLS_ASSIGN_OR_RETURN(std::filesystem::path executable,
                       GetXlsRunfilePath(executable_name));

  std::string exec_name;
  if (!execution_disambiq.empty()) {
    exec_name =
        absl::StrCat(executable.filename().string(), ".", execution_disambiq);
  } else {
    exec_name = executable.filename();
  }

  std::vector<std::shared_ptr<RE2>> filters;
  for (const KnownFailure& filter : options.known_failures()) {
    if (filter.tool == nullptr || RE2::FullMatch(exec_name, *filter.tool)) {
      filters.emplace_back(filter.stderr_regex);
    }
  }

  std::vector<std::string> argv = {executable.string()};
  absl::c_move(std::move(args), std::back_inserter(argv));
  argv.push_back("--logtostderr");

  // TODO(epastor): We should probably inject these, rather than have them
  // grabbed from the command line inside of this library.
  if (int64_t verbosity = absl::GetFlag(FLAGS_v); verbosity > 0) {
    argv.push_back(absl::StrCat("--v=", verbosity));
  }
  if (std::string vmodule = absl::GetFlag(FLAGS_vmodule); !vmodule.empty()) {
    argv.push_back(absl::StrCat("--vmodule=", absl::GetFlag(FLAGS_vmodule)));
  }

  std::optional<absl::Duration> timeout =
      options.timeout_seconds().has_value()
          ? std::make_optional(absl::Seconds(*options.timeout_seconds()))
          : std::nullopt;
  XLS_ASSIGN_OR_RETURN(SubprocessResult result,
                       InvokeSubprocess(argv, run_dir, timeout));
  std::string command_string = absl::StrJoin(argv, " ");
  if (result.timeout_expired) {
    if (!options.timeout_seconds().has_value()) {
      return absl::DeadlineExceededError(
          absl::StrCat("Subprocess call timed out: ", command_string));
    }
    return absl::DeadlineExceededError(
        absl::StrCat("Subprocess call timed out after ",
                     *options.timeout_seconds(), " seconds: ", command_string));
  }
  XLS_RETURN_IF_ERROR(SetFileContents(
      run_dir / absl::StrCat(exec_name, ".stderr"), result.stderr_content));
  if (VLOG_IS_ON(4)) {
    // stdout and stderr can be long so split them by line to avoid clipping.
    VLOG(4) << exec_name << " stdout:";
    XLS_VLOG_LINES(4, result.stdout_content);

    VLOG(4) << exec_name << " stderr:";
    XLS_VLOG_LINES(4, result.stderr_content);
  }
  if (!result.normal_termination) {
    return absl::InternalError(
        absl::StrFormat("Subprocess call failed: %s\n\n"
                        "Subprocess stderr:\n%s",
                        command_string, result.stderr_content));
  }
  if (result.exit_status != EXIT_SUCCESS) {
    if (absl::c_any_of(filters, [&](const std::shared_ptr<RE2>& re) {
          return RE2::PartialMatch(result.stderr_content, *re);
        })) {
      return absl::FailedPreconditionError(
          absl::StrFormat("%s returned a non-zero exit status (%d) but failure "
                          "was suppressed due to stderr regexp",
                          executable.string(), result.exit_status));
    }
    return absl::InternalError(
        absl::StrFormat("%s returned a non-zero exit status (%d): %s\n\n"
                        "Subprocess stderr:\n%s",
                        executable.string(), result.exit_status, command_string,
                        result.stderr_content));
  }
  return result.stdout_content;
}

// Generates a Callable from an executable name.
//
//  - execution_disambiq: String included in name of any log files generated by
//    this command, to disambiguate/differentiate multiple invocations of
//    this tool for a single sample.
SampleRunner::Commands::Callable CallableFromExecutable(
    std::string_view executable,
    const std::string_view execution_disambiq = {}) {
  return [executable, execution_disambiq](
             const std::vector<std::string>& args,
             const std::filesystem::path& run_dir,
             const SampleOptions& options) -> absl::StatusOr<std::string> {
    return RunCommandFromExecutable(executable, args, run_dir, options,
                                    execution_disambiq);
  };
}

// Runs the given command, returning the command's stdout if successful, and
// attaching the command's stderr to the resulting status if not.
absl::StatusOr<std::string> RunCommand(
    std::string_view desc, const SampleRunner::Commands::Callable& command,
    const std::vector<std::string>& args, const std::filesystem::path& run_dir,
    const SampleOptions& options) {
  VLOG(1) << "Running: " << desc;
  Stopwatch timer;
  absl::StatusOr<std::string> result = command(args, run_dir, options);
  const absl::Duration elapsed = timer.GetElapsedTime();
  VLOG(1) << desc << " complete, elapsed " << elapsed;
  return result;
}

// Converts the DSLX file to an IR file with a function as the top whose
// filename is returned.
absl::StatusOr<std::filesystem::path> DslxToIrFunction(
    const std::filesystem::path& input_path, const SampleOptions& options,
    const std::filesystem::path& run_dir,
    const SampleRunner::Commands& commands) {
  std::optional<SampleRunner::Commands::Callable> command =
      commands.ir_converter_main;
  if (!command.has_value()) {
    command = CallableFromExecutable(kBinary.ir_converter_main);
  }

  std::vector<std::string> args;
  absl::c_copy(options.ir_converter_args(), std::back_inserter(args));
  args.push_back("--warnings_as_errors=false");
  args.push_back(input_path.string());
  XLS_ASSIGN_OR_RETURN(
      std::string ir_text,
      RunCommand("Converting DSLX to IR", *command, args, run_dir, options));
  VLOG(3) << "Unoptimized IR:\n" << ir_text;

  std::filesystem::path ir_path = run_dir / "sample.ir";
  XLS_RETURN_IF_ERROR(SetFileContents(ir_path, ir_text));
  return ir_path;
}

// Parses a line-delimited sequence of text-formatted values.
//
// Example of expected input:
//   bits[32]:0x42
//   bits[32]:0x123
absl::StatusOr<std::vector<dslx::InterpValue>> ParseValues(std::string_view s) {
  std::vector<dslx::InterpValue> values;
  for (std::string_view line : absl::StrSplit(s, '\n')) {
    line = absl::StripAsciiWhitespace(line);
    if (line.empty()) {
      continue;
    }
    XLS_ASSIGN_OR_RETURN(Value value, xls::Parser::ParseTypedValue(line));
    XLS_ASSIGN_OR_RETURN(dslx::InterpValue interp_value,
                         dslx::ValueToInterpValue(value));
    values.push_back(std::move(interp_value));
  }
  return values;
}

// Evaluate the IR file with a function as its top and return the result Values.
absl::StatusOr<std::vector<dslx::InterpValue>> EvaluateIrFunction(
    const std::filesystem::path& ir_path,
    const std::filesystem::path& testvector_path, bool use_jit,
    const SampleOptions& options, const std::filesystem::path& run_dir,
    const SampleRunner::Commands& commands) {
  std::optional<SampleRunner::Commands::Callable> command =
      commands.eval_ir_main;
  if (!command.has_value()) {
    command = CallableFromExecutable(kBinary.eval_ir_main);
  }

  XLS_ASSIGN_OR_RETURN(
      std::string results_text,
      RunCommand(
          absl::StrFormat("Evaluating IR file (%s): %s",
                          (use_jit ? "JIT" : "interpreter"), ir_path),
          *command,
          {
              absl::StrCat("--testvector_textproto=", testvector_path.string()),
              absl::StrFormat("--%suse_llvm_jit", use_jit ? "" : "no"),
              ir_path,
          },
          run_dir, options));
  XLS_RETURN_IF_ERROR(SetFileContents(
      absl::StrCat(ir_path.string(), ".results"), results_text));
  return ParseValues(results_text);
}

absl::StatusOr<std::filesystem::path> Codegen(
    const std::filesystem::path& ir_path,
    absl::Span<const std::string> codegen_args, const SampleOptions& options,
    const std::filesystem::path& run_dir,
    const SampleRunner::Commands& commands, bool use_codegen_ng = false) {
  std::optional<SampleRunner::Commands::Callable> command =
      commands.codegen_main;
  if (!command.has_value()) {
    command = CallableFromExecutable(kBinary.codegen_main);
  }

  std::vector<std::string> args;
  if (use_codegen_ng) {
    args = std::vector<std::string>{
        "--codegen_version=2",
        "--output_signature_path=module_sig.ng.textproto",
        "--delay_model=unit",
    };
  } else {
    args = std::vector<std::string>{
        "--output_signature_path=module_sig.textproto",
        "--delay_model=unit",
    };
  }

  args.insert(args.end(), codegen_args.begin(), codegen_args.end());
  args.push_back(ir_path.string());
  XLS_ASSIGN_OR_RETURN(
      std::string verilog_text,
      RunCommand("Generating Verilog", *command, args, run_dir, options));
  VLOG(3) << "Verilog:\n" << verilog_text;
  std::filesystem::path verilog_path;
  if (use_codegen_ng) {
    verilog_path = run_dir / (options.use_system_verilog() ? "sample.ng.sv"
                                                           : "sample.ng.v");
  } else {
    verilog_path =
        run_dir / (options.use_system_verilog() ? "sample.sv" : "sample.v");
  }
  XLS_RETURN_IF_ERROR(SetFileContents(verilog_path, verilog_text));
  return verilog_path;
}

// Optimizes the IR file and returns the resulting filename.
absl::StatusOr<std::filesystem::path> OptimizeIr(
    const std::filesystem::path& ir_path, const SampleOptions& options,
    const std::filesystem::path& run_dir,
    const SampleRunner::Commands& commands) {
  std::optional<SampleRunner::Commands::Callable> command =
      commands.ir_opt_main;
  if (!command.has_value()) {
    command = CallableFromExecutable(kBinary.ir_opt_main);
  }

  XLS_ASSIGN_OR_RETURN(
      std::string opt_ir_text,
      RunCommand("Optimizing IR", *command, {ir_path}, run_dir, options));
  VLOG(3) << "Optimized IR:\n" << opt_ir_text;
  std::filesystem::path opt_ir_path = run_dir / "sample.opt.ir";
  XLS_RETURN_IF_ERROR(SetFileContents(opt_ir_path, opt_ir_text));
  return opt_ir_path;
}

// Convert an IR file to MLIR.
absl::StatusOr<std::filesystem::path> TranslateIrToMlir(
    const std::filesystem::path& inp_ir_path, const SampleOptions& options,
    const std::filesystem::path& run_dir,
    const SampleRunner::Commands& commands) {
  std::string execution_disambiq = inp_ir_path.filename().string();
  if (execution_disambiq.empty()) {
    return absl::InvalidArgumentError("Input ir path not a file: " +
                                      inp_ir_path.string());
  }
  std::optional<SampleRunner::Commands::Callable> command_to_mlir =
      commands.xls_translate_main;
  if (!command_to_mlir.has_value()) {
    command_to_mlir =
        CallableFromExecutable(kBinary.xls_translate_main, execution_disambiq);
  }

  XLS_ASSIGN_OR_RETURN(
      std::string mlir_text,
      RunCommand("Converting to MLIR", *command_to_mlir,
                 {"--xls-to-mlir-xls", inp_ir_path, "--"}, run_dir, options));
  VLOG(3) << "MLIR:\n" << mlir_text;
  std::filesystem::path mlir_path =
      run_dir / (inp_ir_path.filename().string() + ".mlir");
  XLS_RETURN_IF_ERROR(SetFileContents(mlir_path, mlir_text));

  return mlir_path;
}

// Convert an MLIR file to IR.
absl::StatusOr<std::filesystem::path> TranslateMlirToIr(
    const std::filesystem::path& inp_mlir_path, const SampleOptions& options,
    const std::filesystem::path& run_dir,
    const SampleRunner::Commands& commands) {
  std::string execution_disambiq = inp_mlir_path.filename().string();

  if (execution_disambiq.empty()) {
    return absl::InvalidArgumentError("Input MLIR path not a file: " +
                                      inp_mlir_path.string());
  }

  std::optional<SampleRunner::Commands::Callable> command_from_mlir =
      commands.xls_translate_main;
  if (!command_from_mlir.has_value()) {
    command_from_mlir =
        CallableFromExecutable(kBinary.xls_translate_main, execution_disambiq);
  }

  // Check for comment indicating "top" entity. Super hacky ;)
  // TODO(schilkp): This is a workaround to enable fuzzing until proper "top"
  //                tracking is implemented in the MLIR dialect.
  XLS_ASSIGN_OR_RETURN(std::string mlir_text, GetFileContents(inp_mlir_path));
  std::optional<std::string> top_name;
  if (mlir_text.starts_with("// top: ")) {
    auto eol_idx = mlir_text.find_first_of('\n');
    top_name = mlir_text.substr(8, eol_idx - 8);
  }

  std::string ir_text;
  if (top_name.has_value()) {
    XLS_ASSIGN_OR_RETURN(
        ir_text, RunCommand("Converting back from MLIR", *command_from_mlir,
                            {"--mlir-xls-to-xls", inp_mlir_path,
                             "--main-function", *top_name, "--"},
                            run_dir, options));
  } else {
    XLS_ASSIGN_OR_RETURN(
        ir_text, RunCommand("Converting back from MLIR", *command_from_mlir,
                            {"--mlir-xls-to-xls", inp_mlir_path, "--"}, run_dir,
                            options));
  }
  VLOG(3) << "IR:\n" << ir_text;
  std::filesystem::path ir_path =
      run_dir / (inp_mlir_path.filename().string() + ".ir");

  XLS_RETURN_IF_ERROR(SetFileContents(ir_path, ir_text));

  return ir_path;
}

// Simulates the Verilog file representing a function and returns the results.
absl::StatusOr<std::vector<dslx::InterpValue>> SimulateFunction(
    const std::filesystem::path& verilog_path,
    const std::filesystem::path& module_sig_path,
    const std::filesystem::path& testvector_path, const SampleOptions& options,
    const std::filesystem::path& run_dir,
    const SampleRunner::Commands& commands) {
  std::optional<SampleRunner::Commands::Callable> command =
      commands.simulate_module_main;
  if (!command.has_value()) {
    command = CallableFromExecutable(kBinary.simulate_module_main);
  }

  std::vector<std::string> simulator_args = {
      absl::StrCat("--signature_file=", module_sig_path.string()),
      absl::StrCat("--testvector_textproto=", testvector_path.string()),
  };
  XLS_RETURN_IF_ERROR(CheckSimulator(options.simulator()));
  if (!options.simulator().empty()) {
    simulator_args.push_back(
        absl::StrCat("--verilog_simulator=", options.simulator()));
  }
  simulator_args.push_back(verilog_path.string());

  XLS_ASSIGN_OR_RETURN(
      std::string results_text,
      RunCommand(absl::StrCat("Simulating Verilog ", verilog_path.string()),
                 *command, simulator_args, run_dir, options));
  XLS_RETURN_IF_ERROR(SetFileContents(
      absl::StrCat(verilog_path.string(), ".results"), results_text));
  return ParseValues(results_text);
}

// Compares a set of results as for equality.
//
// Each entry in the map is sequence of Values generated from some source
// (e.g., interpreting the optimized IR). Each sequence of Values is compared
// for equality.
absl::Status CompareResultsProc(
    const absl::flat_hash_map<
        std::string, absl::flat_hash_map<std::string, std::vector<Value>>>&
        results) {
  if (results.empty()) {
    return absl::OkStatus();
  }

  std::deque<std::string> stages;
  for (const auto& [stage, _] : results) {
    stages.push_back(stage);
  }
  std::sort(stages.begin(), stages.end());

  std::string reference = stages.front();
  stages.pop_front();

  const absl::flat_hash_map<std::string, std::vector<Value>>&
      all_channel_values_ref = results.at(reference);

  for (std::string_view name : stages) {
    const absl::flat_hash_map<std::string, std::vector<Value>>&
        all_channel_values = results.at(name);
    if (all_channel_values_ref.size() != all_channel_values.size()) {
      std::vector<std::string> ref_channel_names;
      for (const auto& [channel_name, _] : all_channel_values_ref) {
        ref_channel_names.push_back(channel_name);
      }
      std::vector<std::string> channel_names;
      for (const auto& [channel_name, _] : all_channel_values) {
        channel_names.push_back(channel_name);
      }
      constexpr auto quote_formatter = [](std::string* out,
                                          const std::string& s) {
        absl::StrAppend(out, "'", s, "'");
      };
      return absl::InvalidArgumentError(absl::StrFormat(
          "Results for %s has %d channel(s), %s has %d "
          "channel(s). The IR channel names in %s are: [%s]. "
          "The IR channel names in %s are: [%s].",
          reference, all_channel_values_ref.size(), name,
          all_channel_values.size(), reference,
          absl::StrJoin(ref_channel_names, ", ", quote_formatter), name,
          absl::StrJoin(channel_names, ", ", quote_formatter)));
    }

    for (const auto& [channel_name_ref, channel_values_ref] :
         all_channel_values_ref) {
      auto it = all_channel_values.find(channel_name_ref);
      if (it == all_channel_values.end()) {
        return absl::InvalidArgumentError(
            absl::StrFormat("A channel named %s is present in %s, but it is "
                            "not present in %s.",
                            channel_name_ref, reference, name));
      }
      const std::vector<Value>& channel_values = it->second;
      if (channel_values_ref.size() != channel_values.size()) {
        return absl::InvalidArgumentError(absl::StrFormat(
            "In %s, channel '%s' has %d entries. However, in %s, channel "
            "'%s' "
            "has %d entries.",
            reference, channel_name_ref, channel_values_ref.size(), name,
            channel_name_ref, channel_values.size()));
      }
      for (int i = 0; i < channel_values_ref.size(); ++i) {
        if (channel_values[i] != channel_values_ref[i]) {
          return absl::InvalidArgumentError(absl::StrFormat(
              "In %s, at position %d channel '%s' has value "
              "%s. However, in %s, the value is %s.",
              reference, i, channel_name_ref,
              dslx::ValueToInterpValue(channel_values_ref[i])->ToString(), name,
              dslx::ValueToInterpValue(channel_values[i])->ToString()));
        }
      }
    }
  }

  return absl::OkStatus();
}

absl::StatusOr<absl::flat_hash_map<std::string, std::vector<dslx::InterpValue>>>
RunProc(dslx::Proc* proc, dslx::ImportData& import_data,
        const dslx::TypecheckedModule& tm, const ArgsBatch& args_batch,
        int64_t proc_ticks) {
  XLS_ASSIGN_OR_RETURN(dslx::TypeInfo * proc_type_info,
                       tm.type_info->GetTopLevelProcTypeInfo(proc));

  std::string module_name = proc->owner()->name();
  XLS_ASSIGN_OR_RETURN(
      std::unique_ptr<dslx::ProcHierarchyInterpreter> hierarchy_interpreter,
      dslx::ProcHierarchyInterpreter::Create(&import_data, proc_type_info,
                                             proc));

  // Positional indexes of the input and output channels in the config function.
  std::vector<int64_t> out_chan_indexes;
  std::vector<int64_t> in_chan_indexes;
  // The mapping of the channels in the output_channel_names follow the mapping
  // of out_chan_indexes. For example, out_channel_names[i] refers to same
  // channel at out_chan_indexes[i].
  std::vector<std::string> out_ir_channel_names;
  for (int64_t index = 0; index < hierarchy_interpreter->GetInterfaceSize();
       ++index) {
    if (hierarchy_interpreter->GetInterfaceChannelDirection(index) ==
        dslx::ChannelDirection::kIn) {
      in_chan_indexes.push_back(index);
    } else {
      out_chan_indexes.push_back(index);
      out_ir_channel_names.push_back(absl::StrCat(
          module_name, "__", proc->config().params().at(index)->identifier()));
    }
  }

  // Feed the inputs to the channels.
  for (const std::vector<dslx::InterpValue>& arg_batch : args_batch) {
    XLS_RET_CHECK_EQ(in_chan_indexes.size(), arg_batch.size());
    for (int64_t i = 0; i < arg_batch.size(); ++i) {
      dslx::InterpValueChannel& channel =
          hierarchy_interpreter->GetInterfaceChannel(in_chan_indexes[i]);
      const dslx::Type* payload_type =
          hierarchy_interpreter->GetInterfaceChannelPayloadType(
              in_chan_indexes[i]);
      XLS_ASSIGN_OR_RETURN(dslx::InterpValue payload_value,
                           SignConvertValue(*payload_type, arg_batch[i]));
      channel.Write(payload_value);
    }
  }

  for (int i = 0; i < proc_ticks; i++) {
    XLS_RETURN_IF_ERROR(hierarchy_interpreter->Tick());
  }

  absl::flat_hash_map<std::string, std::vector<dslx::InterpValue>>
      all_channel_values;
  for (int64_t index = 0; index < out_chan_indexes.size(); ++index) {
    dslx::InterpValueChannel& channel =
        hierarchy_interpreter->GetInterfaceChannel(out_chan_indexes[index]);
    all_channel_values[out_ir_channel_names[index]] =
        std::vector<dslx::InterpValue>();
    while (!channel.IsEmpty()) {
      all_channel_values[out_ir_channel_names[index]].push_back(channel.Read());
    }
  }
  return all_channel_values;
}

// Interprets a DSLX module with proc as the top, returning the resulting
// Values.
absl::StatusOr<absl::flat_hash_map<std::string, std::vector<Value>>>
InterpretDslxProc(std::string_view text, std::string_view top_name,
                  const ArgsBatch& args_batch, int tick_count,
                  const std::filesystem::path& run_dir) {
  dslx::ImportData import_data = dslx::CreateImportData(
      GetDefaultDslxStdlibPath(),
      /*additional_search_paths=*/{}, dslx::kDefaultWarningsSet,
      std::make_unique<dslx::RealFilesystem>());
  XLS_ASSIGN_OR_RETURN(
      dslx::TypecheckedModule tm,
      dslx::ParseAndTypecheck(text, "sample.x", "sample", &import_data));

  std::optional<dslx::ModuleMember*> module_member =
      tm.module->FindMemberWithName(top_name);
  XLS_RET_CHECK(module_member.has_value());
  dslx::ModuleMember* member = module_member.value();
  XLS_RET_CHECK(std::holds_alternative<dslx::Proc*>(*member));
  dslx::Proc* proc = std::get<dslx::Proc*>(*member);

  absl::flat_hash_map<std::string, std::vector<dslx::InterpValue>> dslx_results;
  XLS_ASSIGN_OR_RETURN(dslx_results,
                       RunProc(proc, import_data, tm, args_batch, tick_count));

  absl::flat_hash_map<std::string, std::vector<Value>> ir_channel_values;
  for (const auto& [key, values] : dslx_results) {
    XLS_ASSIGN_OR_RETURN(ir_channel_values[key],
                         dslx::InterpValue::ConvertValuesToIr(values));
  }
  XLS_RETURN_IF_ERROR(SetFileContents(
      run_dir / "sample.x.results", ChannelValuesToString(ir_channel_values)));

  return ir_channel_values;
}

absl::StatusOr<std::filesystem::path> DslxToIrProc(
    const std::filesystem::path& dslx_path, const SampleOptions& options,
    const std::filesystem::path& run_dir,
    const SampleRunner::Commands& commands) {
  std::optional<SampleRunner::Commands::Callable> command =
      commands.ir_converter_main;
  if (!command.has_value()) {
    command = CallableFromExecutable(kBinary.ir_converter_main);
  }

  std::vector<std::string> args;
  absl::c_copy(options.ir_converter_args(), std::back_inserter(args));
  args.push_back("--warnings_as_errors=false");
  args.push_back(dslx_path);
  XLS_ASSIGN_OR_RETURN(
      std::string ir_text,
      RunCommand("Converting DSLX to IR", *command, args, run_dir, options));
  VLOG(3) << "Unoptimized IR:\n" << ir_text;
  std::filesystem::path ir_path = run_dir / "sample.ir";
  XLS_RETURN_IF_ERROR(SetFileContents(ir_path, ir_text));
  return ir_path;
}

absl::StatusOr<absl::flat_hash_map<std::string, std::vector<Value>>>
EvaluateIrProc(const std::filesystem::path& ir_path, int64_t tick_count,
               const std::filesystem::path& testvector_proto_path, bool use_jit,
               const SampleOptions& options,
               const std::filesystem::path& run_dir,
               const SampleRunner::Commands& commands) {
  std::optional<SampleRunner::Commands::Callable> command =
      commands.eval_proc_main;
  if (!command.has_value()) {
    command = CallableFromExecutable(kBinary.eval_proc_main);
  }

  std::string_view evaluation_type = use_jit ? "JIT" : "interpreter";
  std::string desc =
      absl::StrFormat("Evaluating IR file (%s): %s", evaluation_type, ir_path);
  std::string_view backend_type = use_jit ? "serial_jit" : "ir_interpreter";
  std::vector<std::string> args = {
      absl::StrCat("--testvector_textproto=", testvector_proto_path.string()),
      absl::StrCat("--ticks=", tick_count),
      absl::StrCat("--backend=", backend_type),
      ir_path,
  };
  XLS_ASSIGN_OR_RETURN(std::string results_text,
                       RunCommand(desc, *command, args, run_dir, options));
  XLS_RETURN_IF_ERROR(SetFileContents(
      absl::StrCat(ir_path.string(), ".results"), results_text));
  absl::btree_map<std::string, std::vector<Value>> ir_channel_values;
  XLS_ASSIGN_OR_RETURN(ir_channel_values,
                       ParseChannelValues(results_text, tick_count));
  absl::flat_hash_map<std::string, std::vector<xls::Value>>
      unordered_ir_channel_values;
  unordered_ir_channel_values.reserve(ir_channel_values.size());
  absl::c_move(std::move(ir_channel_values),
               std::inserter(unordered_ir_channel_values,
                             unordered_ir_channel_values.end()));
  return unordered_ir_channel_values;
}

// Returns a output-channel-count map from an output-channel-values map.
absl::flat_hash_map<std::string, int64_t> GetOutputChannelCounts(
    const absl::flat_hash_map<std::string, std::vector<Value>>&
        output_channel_values) {
  absl::flat_hash_map<std::string, int64_t> output_channel_counts;
  for (const auto& [channel_name, channel_values] : output_channel_values) {
    output_channel_counts[channel_name] = channel_values.size();
  }
  return output_channel_counts;
}

// Returns a string representation of the output-channel-count map.
//
// The string format is output_channel_name=count for each entry in the map. The
// entries of the map are comma separated. For example, given an
// output-channel-count map:
//
//   {{foo, 42}, {bar,64}}
//
// the string representation is:
//
//   foo=42,bar=64
std::string GetOutputChannelToString(
    const absl::flat_hash_map<std::string, int64_t>& output_channel_counts) {
  std::vector<std::string> output_channel_counts_strings;
  for (const auto& [channel_name, count] : output_channel_counts) {
    output_channel_counts_strings.push_back(
        absl::StrCat(channel_name, "=", count));
  }
  return absl::StrJoin(output_channel_counts_strings, ",");
}

// Simulates the Verilog file representing a proc and returns the resulting
// Values.
absl::StatusOr<absl::flat_hash_map<std::string, std::vector<Value>>>
SimulateProc(const std::filesystem::path& verilog_path,
             const std::filesystem::path& module_sig_path,
             const std::filesystem::path& testvector_path,
             std::string_view output_channel_counts,
             const SampleOptions& options, const std::filesystem::path& run_dir,
             const SampleRunner::Commands& commands) {
  std::optional<SampleRunner::Commands::Callable> command =
      commands.simulate_module_main;
  if (!command.has_value()) {
    command = CallableFromExecutable(kBinary.simulate_module_main);
  }

  std::vector<std::string> simulator_args = {
      absl::StrCat("--signature_file=", module_sig_path.string()),
      absl::StrCat("--testvector_textproto=", testvector_path.string()),
      absl::StrCat("--output_channel_counts=", output_channel_counts),
  };
  if (!options.simulator().empty()) {
    XLS_RETURN_IF_ERROR(CheckSimulator(options.simulator()));
    simulator_args.push_back(
        absl::StrCat("--verilog_simulator=", options.simulator()));
  }
  simulator_args.push_back(verilog_path);

  XLS_ASSIGN_OR_RETURN(
      std::string results_text,
      RunCommand(absl::StrCat("Simulating Verilog ", verilog_path.string()),
                 *command, simulator_args, run_dir, options));
  XLS_RETURN_IF_ERROR(SetFileContents(
      absl::StrCat(verilog_path.string(), ".results"), results_text));

  absl::btree_map<std::string, std::vector<Value>> channel_values;
  XLS_ASSIGN_OR_RETURN(channel_values, ParseChannelValues(results_text));
  absl::flat_hash_map<std::string, std::vector<xls::Value>>
      unordered_channel_values;
  unordered_channel_values.reserve(channel_values.size());
  absl::c_move(
      std::move(channel_values),
      std::inserter(unordered_channel_values, unordered_channel_values.end()));
  return unordered_channel_values;
}

}  // namespace

absl::Status SampleRunner::Run(const Sample& sample) {
  std::filesystem::path input_path = run_dir_;
  if (sample.options().input_is_dslx()) {
    input_path /= "sample.x";
  } else {
    input_path /= "sample.ir";
  }
  XLS_RETURN_IF_ERROR(SetFileContents(input_path, sample.input_text()));

  std::filesystem::path options_path = run_dir_ / "options.pbtxt";
  XLS_RETURN_IF_ERROR(SetTextProtoFile(options_path, sample.options().proto()));

  std::filesystem::path testvector_path = run_dir_ / "testvector.pbtxt";
  XLS_RETURN_IF_ERROR(SetTextProtoFile(testvector_path, sample.testvector()));

  return RunFromFiles(input_path, options_path, testvector_path);
}

absl::Status SampleRunner::RunFromFiles(
    const std::filesystem::path& input_path,
    const std::filesystem::path& options_path,
    const std::filesystem::path& testvector_path) {
  VLOG(1) << "Running sample in directory " << run_dir_;
  VLOG(1) << "Reading sample files.";

  XLS_ASSIGN_OR_RETURN(std::string options_text, GetFileContents(options_path));
  XLS_ASSIGN_OR_RETURN(SampleOptions options,
                       SampleOptions::FromPbtxt(options_text));

  XLS_RETURN_IF_ERROR(
      SetFileContents(run_dir_ / "revision.txt", GetRevision()));

  absl::Status status;
  switch (options.sample_type()) {
    case fuzzer::SampleType::SAMPLE_TYPE_FUNCTION:
      status = RunFunction(input_path, options, testvector_path);
      break;
    case fuzzer::SampleType::SAMPLE_TYPE_PROC:
      status = RunProc(input_path, options, testvector_path);
      break;
    default:
      status = absl::InvalidArgumentError(
          "Unsupported sample type: " +
          fuzzer::SampleType_Name(options.sample_type()));
      break;
  }
  if (!status.ok()) {
    LOG(ERROR) << "Exception when running sample: " << status.ToString();
    XLS_RETURN_IF_ERROR(
        SetFileContents(run_dir_ / "exception.txt", status.ToString()));
  }
  if (status.code() == absl::StatusCode::kFailedPrecondition) {
    LOG(ERROR)
        << "Precondition failed, sample is not valid in the fuzz domain due to "
        << status;
    status = absl::OkStatus();
  }
  return status;
}

absl::Status SampleRunner::RunFunction(
    const std::filesystem::path& input_path, const SampleOptions& options,
    const std::filesystem::path& testvector_path) {
  XLS_ASSIGN_OR_RETURN(std::string input_text, GetFileContents(input_path));

  std::optional<ArgsBatch> args_batch = std::nullopt;
  {
    XLS_RET_CHECK(!testvector_path.empty());
    testvector::SampleInputsProto sample_inputs;
    XLS_RETURN_IF_ERROR(ParseTextProtoFile(testvector_path, &sample_inputs));
    ArgsBatch extracted;
    XLS_RETURN_IF_ERROR(
        Sample::ExtractArgsBatch(options, sample_inputs, extracted));
    args_batch = std::move(extracted);
  }

  // Results from various ways of interpretation.
  absl::flat_hash_map<std::string, std::vector<dslx::InterpValue>> results;

  std::filesystem::path ir_path;
  if (options.input_is_dslx()) {
    if (args_batch.has_value()) {
      VLOG(1) << "Interpreting DSLX file.";
      Stopwatch t;
      XLS_ASSIGN_OR_RETURN(
          results["interpreted DSLX"],
          InterpretDslxFunction(input_text, "main", *args_batch, run_dir_));
      absl::Duration elapsed = t.GetElapsedTime();
      VLOG(1) << "Interpreting DSLX complete, elapsed: " << elapsed;
      timing_.set_interpret_dslx_ns(absl::ToInt64Nanoseconds(elapsed));
    }

    if (!options.convert_to_ir()) {
      return absl::OkStatus();
    }

    Stopwatch t;
    XLS_ASSIGN_OR_RETURN(
        ir_path, DslxToIrFunction(input_path, options, run_dir_, commands_));
    timing_.set_convert_ir_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));
  } else {
    ir_path = run_dir_ / "sample.ir";
    XLS_RETURN_IF_ERROR(SetFileContents(ir_path, input_text));
  }

  if (args_batch.has_value()) {
    Stopwatch t;

    // Unconditionally evaluate with the interpreter even if using the JIT. This
    // exercises the interpreter and serves as a reference.
    XLS_ASSIGN_OR_RETURN(results["evaluated unopt IR (interpreter)"],
                         EvaluateIrFunction(ir_path, testvector_path, false,
                                            options, run_dir_, commands_));
    timing_.set_unoptimized_interpret_ir_ns(
        absl::ToInt64Nanoseconds(t.GetElapsedTime()));

    if (options.use_jit()) {
      XLS_ASSIGN_OR_RETURN(results["evaluated unopt IR (JIT)"],
                           EvaluateIrFunction(ir_path, testvector_path, true,
                                              options, run_dir_, commands_));
      timing_.set_unoptimized_jit_ns(
          absl::ToInt64Nanoseconds(t.GetElapsedTime()));
    }
  }

  if (options.translate_ir_to_mlir()) {
    Stopwatch t;
    XLS_ASSIGN_OR_RETURN(
        std::filesystem::path mlir_path,
        TranslateIrToMlir(ir_path, options, run_dir_, commands_));
    timing_.set_translate_to_mlir_ns(
        absl::ToInt64Nanoseconds(t.GetElapsedTime()));

    if (options.translate_mlir_to_ir()) {
      t.Reset();
      XLS_ASSIGN_OR_RETURN(
          std::filesystem::path roundtrip_ir_path,
          TranslateMlirToIr(mlir_path, options, run_dir_, commands_));
      timing_.set_translate_from_mlir_ns(
          absl::ToInt64Nanoseconds(t.GetElapsedTime()));

      if (args_batch.has_value()) {
        t.Reset();
        XLS_ASSIGN_OR_RETURN(
            results["evaluated MLIR-roundtripped IR (interpreter)"],
            EvaluateIrFunction(roundtrip_ir_path, testvector_path, false,
                               options, run_dir_, commands_));
        timing_.set_mlir_roundtrip_interpret_ns(
            absl::ToInt64Nanoseconds(t.GetElapsedTime()));
      }
    }
  }

  if (options.optimize_ir()) {
    Stopwatch t;
    XLS_ASSIGN_OR_RETURN(std::filesystem::path opt_ir_path,
                         OptimizeIr(ir_path, options, run_dir_, commands_));
    timing_.set_optimize_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));

    if (args_batch.has_value()) {
      if (options.use_jit()) {
        t.Reset();
        XLS_ASSIGN_OR_RETURN(
            results["evaluated opt IR (JIT)"],
            EvaluateIrFunction(opt_ir_path, testvector_path, true, options,
                               run_dir_, commands_));
        timing_.set_optimized_jit_ns(
            absl::ToInt64Nanoseconds(t.GetElapsedTime()));
      }
      t.Reset();
      XLS_ASSIGN_OR_RETURN(
          results["evaluated opt IR (interpreter)"],
          EvaluateIrFunction(opt_ir_path, testvector_path, false, options,
                             run_dir_, commands_));
      timing_.set_optimized_interpret_ir_ns(
          absl::ToInt64Nanoseconds(t.GetElapsedTime()));
    }

    if (options.translate_opt_ir_to_mlir()) {
      Stopwatch t;
      XLS_ASSIGN_OR_RETURN(
          std::filesystem::path mlir_path,
          TranslateIrToMlir(opt_ir_path, options, run_dir_, commands_));
      timing_.set_translate_opt_to_mlir_ns(
          absl::ToInt64Nanoseconds(t.GetElapsedTime()));

      if (options.translate_mlir_to_ir()) {
        t.Reset();
        XLS_ASSIGN_OR_RETURN(
            std::filesystem::path roundtrip_ir_path,
            TranslateMlirToIr(mlir_path, options, run_dir_, commands_));
        timing_.set_translate_opt_from_mlir_ns(
            absl::ToInt64Nanoseconds(t.GetElapsedTime()));

        if (args_batch.has_value()) {
          t.Reset();
          XLS_ASSIGN_OR_RETURN(
              results["evaluated MLIR-roundtripped optimized IR (interpreter)"],
              EvaluateIrFunction(roundtrip_ir_path, testvector_path, false,
                                 options, run_dir_, commands_));
          timing_.set_opt_mlir_roundtrip_interpret_ns(
              absl::ToInt64Nanoseconds(t.GetElapsedTime()));
        }
      }
    }

    if (options.codegen()) {
      t.Reset();
      XLS_ASSIGN_OR_RETURN(std::filesystem::path verilog_path,
                           Codegen(opt_ir_path, options.codegen_args(), options,
                                   run_dir_, commands_));
      timing_.set_codegen_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));

      if (options.simulate()) {
        XLS_RET_CHECK(args_batch.has_value());
        t.Reset();
        XLS_ASSIGN_OR_RETURN(
            results["simulated"],
            SimulateFunction(verilog_path, "module_sig.textproto",
                             testvector_path, options, run_dir_, commands_));
        timing_.set_simulate_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));
      }
    }

    if (options.codegen_ng()) {
      t.Reset();
      XLS_ASSIGN_OR_RETURN(
          std::filesystem::path verilog_path,
          Codegen(opt_ir_path, options.codegen_args(), options, run_dir_,
                  commands_, /*use_codegen_ng=*/true));
      timing_.set_codegen_ng_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));

      if (options.simulate()) {
        XLS_RET_CHECK(args_batch.has_value());
        t.Reset();
        XLS_ASSIGN_OR_RETURN(
            results["simulated_ng"],
            SimulateFunction(verilog_path, "module_sig.ng.textproto",
                             testvector_path, options, run_dir_, commands_));
        timing_.set_simulate_ng_ns(
            absl::ToInt64Nanoseconds(t.GetElapsedTime()));
      }
    }
  }

  absl::flat_hash_map<std::string, absl::Span<const dslx::InterpValue>>
      results_spans(results.begin(), results.end());
  return CompareResultsFunction(
      results_spans, args_batch.has_value() ? &*args_batch : nullptr);
}

absl::Status SampleRunner::RunProc(
    const std::filesystem::path& input_path, const SampleOptions& options,
    const std::filesystem::path& testvector_path) {
  XLS_ASSIGN_OR_RETURN(std::string input_text, GetFileContents(input_path));

  std::optional<ArgsBatch> args_batch = std::nullopt;
  std::optional<std::vector<std::string>> ir_channel_names = std::nullopt;

  {
    testvector::SampleInputsProto sample_inputs;
    XLS_RETURN_IF_ERROR(ParseTextProtoFile(testvector_path, &sample_inputs));
    ArgsBatch extracted_args;
    std::vector<std::string> extracted_channel_names;
    XLS_RETURN_IF_ERROR(Sample::ExtractArgsBatch(
        options, sample_inputs, extracted_args, &extracted_channel_names));
    args_batch = std::move(extracted_args);
    ir_channel_names = std::move(extracted_channel_names);
  }

  // Special case: When there no inputs for a proc, typically when there are no
  // channels for a proc, tick_count results to 0. Set the tick_count to a
  // non-zero value to execute in the eval proc main (bypasses a restriction on
  // the number of ticks in eval proc main).
  int64_t tick_count =
      args_batch.has_value() ? std::max<int64_t>(args_batch->size(), 1) : 1;

  // Note the data is structure with a nested dictionary. The key of the
  // dictionary is the name of the XLS stage being evaluated. The value of the
  // dictionary is another dictionary where the key is the IR channel name. The
  // value of the nested dictionary is a sequence of values corresponding to the
  // channel.
  absl::flat_hash_map<std::string,
                      absl::flat_hash_map<std::string, std::vector<Value>>>
      results;
  std::optional<absl::flat_hash_map<std::string, std::vector<Value>>> reference;

  std::filesystem::path ir_path;
  if (options.input_is_dslx()) {
    if (args_batch.has_value()) {
      VLOG(1) << "Interpreting DSLX file.";
      Stopwatch t;
      XLS_ASSIGN_OR_RETURN(results["interpreted DSLX"],
                           InterpretDslxProc(input_text, "main", *args_batch,
                                             tick_count, run_dir_));
      reference = results["interpreted DSLX"];
      absl::Duration elapsed = t.GetElapsedTime();
      VLOG(1) << "Interpreting DSLX complete, elapsed: " << elapsed;
      timing_.set_interpret_dslx_ns(absl::ToInt64Nanoseconds(elapsed));
    }

    if (!options.convert_to_ir()) {
      return absl::OkStatus();
    }

    Stopwatch t;
    XLS_ASSIGN_OR_RETURN(
        ir_path, DslxToIrProc(input_path, options, run_dir_, commands_));
    timing_.set_convert_ir_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));
  } else {
    ir_path = run_dir_ / "sample.ir";
    XLS_RETURN_IF_ERROR(SetFileContents(ir_path, input_text));
  }

  if (args_batch.has_value()) {
    // Unconditionally evaluate with the interpreter even if using the JIT. This
    // exercises the interpreter and serves as a reference.
    Stopwatch t;
    XLS_ASSIGN_OR_RETURN(results["evaluated unopt IR (interpreter)"],
                         EvaluateIrProc(ir_path, tick_count, testvector_path,
                                        false, options, run_dir_, commands_));
    if (!reference.has_value()) {
      reference = results["evaluated unopt IR (interpreter)"];
    }
    timing_.set_unoptimized_interpret_ir_ns(
        absl::ToInt64Nanoseconds(t.GetElapsedTime()));

    if (options.use_jit()) {
      t.Reset();
      XLS_ASSIGN_OR_RETURN(results["evaluated unopt IR (JIT)"],
                           EvaluateIrProc(ir_path, tick_count, testvector_path,
                                          true, options, run_dir_, commands_));
      timing_.set_unoptimized_jit_ns(
          absl::ToInt64Nanoseconds(t.GetElapsedTime()));
    }
  }

  if (options.translate_ir_to_mlir()) {
    Stopwatch t;
    XLS_ASSIGN_OR_RETURN(
        std::filesystem::path mlir_path,
        TranslateIrToMlir(ir_path, options, run_dir_, commands_));
    timing_.set_translate_to_mlir_ns(
        absl::ToInt64Nanoseconds(t.GetElapsedTime()));

    if (options.translate_mlir_to_ir()) {
      t.Reset();
      XLS_ASSIGN_OR_RETURN(
          std::filesystem::path roundtrip_ir_path,
          TranslateMlirToIr(mlir_path, options, run_dir_, commands_));
      timing_.set_translate_from_mlir_ns(
          absl::ToInt64Nanoseconds(t.GetElapsedTime()));

      if (args_batch.has_value()) {
        t.Reset();
        XLS_ASSIGN_OR_RETURN(
            results["evaluated MLIR-roundtripped IR (interpreter)"],
            EvaluateIrProc(roundtrip_ir_path, tick_count, testvector_path,
                           false, options, run_dir_, commands_));
        timing_.set_mlir_roundtrip_interpret_ns(
            absl::ToInt64Nanoseconds(t.GetElapsedTime()));
      }
    }
  }

  std::optional<std::filesystem::path> opt_ir_path = std::nullopt;
  if (options.optimize_ir()) {
    Stopwatch t;
    XLS_ASSIGN_OR_RETURN(opt_ir_path,
                         OptimizeIr(ir_path, options, run_dir_, commands_));
    timing_.set_optimize_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));

    if (args_batch.has_value()) {
      if (options.use_jit()) {
        t.Reset();
        XLS_ASSIGN_OR_RETURN(
            results["evaluated opt IR (JIT)"],
            EvaluateIrProc(*opt_ir_path, tick_count, testvector_path, true,
                           options, run_dir_, commands_));
        timing_.set_optimized_jit_ns(
            absl::ToInt64Nanoseconds(t.GetElapsedTime()));
      }

      t.Reset();
      XLS_ASSIGN_OR_RETURN(
          results["evaluated opt IR (interpreter)"],
          EvaluateIrProc(*opt_ir_path, tick_count, testvector_path, false,
                         options, run_dir_, commands_));
      timing_.set_optimized_interpret_ir_ns(
          absl::ToInt64Nanoseconds(t.GetElapsedTime()));

      if (options.translate_opt_ir_to_mlir()) {
        Stopwatch t;
        XLS_ASSIGN_OR_RETURN(
            std::filesystem::path mlir_path,
            TranslateIrToMlir(*opt_ir_path, options, run_dir_, commands_));
        timing_.set_translate_opt_to_mlir_ns(
            absl::ToInt64Nanoseconds(t.GetElapsedTime()));

        if (options.translate_mlir_to_ir()) {
          t.Reset();
          XLS_ASSIGN_OR_RETURN(
              std::filesystem::path roundtrip_ir_path,
              TranslateMlirToIr(mlir_path, options, run_dir_, commands_));
          timing_.set_translate_from_mlir_ns(
              absl::ToInt64Nanoseconds(t.GetElapsedTime()));

          if (args_batch.has_value()) {
            t.Reset();
            XLS_ASSIGN_OR_RETURN(
                results
                    ["evaluated MLIR-roundtripped optimized IR (interpreter)"],
                EvaluateIrProc(roundtrip_ir_path, tick_count, testvector_path,
                               false, options, run_dir_, commands_));
            timing_.set_opt_mlir_roundtrip_interpret_ns(
                absl::ToInt64Nanoseconds(t.GetElapsedTime()));
          }
        }
      }

      if (options.codegen()) {
        t.Reset();
        XLS_ASSIGN_OR_RETURN(std::filesystem::path verilog_path,
                             Codegen(*opt_ir_path, options.codegen_args(),
                                     options, run_dir_, commands_));
        timing_.set_codegen_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));

        if (options.simulate()) {
          t.Reset();
          XLS_RET_CHECK(reference.has_value());
          absl::flat_hash_map<std::string, int64_t> output_channel_counts =
              GetOutputChannelCounts(*reference);
          std::string output_channel_counts_str =
              GetOutputChannelToString(output_channel_counts);
          XLS_ASSIGN_OR_RETURN(
              results["simulated"],
              SimulateProc(verilog_path, "module_sig.textproto",
                           testvector_path, output_channel_counts_str, options,
                           run_dir_, commands_));
          timing_.set_simulate_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));
        }
      }

      if (options.codegen_ng()) {
        t.Reset();
        XLS_ASSIGN_OR_RETURN(
            std::filesystem::path verilog_path,
            Codegen(*opt_ir_path, options.codegen_args(), options, run_dir_,
                    commands_, /*use_codegen_ng=*/true));
        timing_.set_codegen_ng_ns(absl::ToInt64Nanoseconds(t.GetElapsedTime()));

        if (options.simulate()) {
          t.Reset();
          XLS_RET_CHECK(reference.has_value());
          absl::flat_hash_map<std::string, int64_t> output_channel_counts =
              GetOutputChannelCounts(*reference);
          std::string output_channel_counts_str =
              GetOutputChannelToString(output_channel_counts);
          XLS_ASSIGN_OR_RETURN(
              results["simulated_ng"],
              SimulateProc(verilog_path, "module_sig.ng.textproto",
                           testvector_path, output_channel_counts_str, options,
                           run_dir_, commands_));
          timing_.set_simulate_ng_ns(
              absl::ToInt64Nanoseconds(t.GetElapsedTime()));
        }
      }
    }
  }

  return CompareResultsProc(results);
}

}  // namespace xls
