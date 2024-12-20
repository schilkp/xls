// Copyright 2022 The XLS Authors
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

#include "xls/passes/unroll_proc.h"

#include <cstdint>
#include <optional>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "xls/common/status/status_macros.h"
#include "xls/ir/node.h"
#include "xls/ir/nodes.h"
#include "xls/ir/proc.h"
#include "xls/ir/topo_sort.h"

namespace xls {

absl::StatusOr<ProcUnrollInfo> UnrollProc(Proc* proc, int64_t num_iterations) {
  ProcUnrollInfo unroll_info;

  std::vector<Node*> original_nodes;
  for (Node* node : TopoSort(proc)) {
    original_nodes.push_back(node);
    unroll_info.original_node_map[node] = node;
    unroll_info.iteration_map[node] = 0;
  }

  std::vector<Node*> state_nodes(proc->NextState().begin(),
                                 proc->NextState().end());

  auto find_state = [proc](Node* node) -> std::optional<int64_t> {
    if (node->Is<StateRead>()) {
      return proc->MaybeGetStateElementIndex(
          node->As<StateRead>()->state_element());
    }
    return std::nullopt;
  };

  for (int64_t iter = 1; iter < num_iterations; ++iter) {
    absl::flat_hash_map<Node*, Node*> clone_map;
    for (Node* node : original_nodes) {
      if (node->Is<StateRead>()) {
        clone_map[node] = node;
        continue;
      }
      std::vector<Node*> operands;
      operands.reserve(node->operands().size());
      for (Node* operand : node->operands()) {
        if (operand->Is<StateRead>()) {
          if (std::optional<int64_t> state_idx = find_state(operand);
              state_idx.has_value()) {
            operands.push_back(state_nodes[*state_idx]);
            continue;
          }
        }
        operands.push_back(clone_map.at(operand));
      }
      XLS_ASSIGN_OR_RETURN(Node * cloned, node->Clone(operands));
      cloned->SetName(absl::StrFormat("%s_iter_%d", node->GetName(), iter));
      clone_map[node] = cloned;
    }
    for (int64_t i = 0; i < proc->StateElements().size(); ++i) {
      state_nodes[i] = clone_map.at(proc->NextState()[i]);
    }
    for (const auto& [original, cloned] : clone_map) {
      unroll_info.original_node_map[cloned] = original;
      unroll_info.iteration_map[cloned] = iter;
    }
  }

  for (Node* node : original_nodes) {
    if (node->Is<StateRead>()) {
      continue;
    }
    node->SetName(absl::StrFormat("%s_iter_0", node->GetName()));
  }

  for (int64_t i = 0; i < proc->StateElements().size(); ++i) {
    XLS_RETURN_IF_ERROR(proc->SetNextStateElement(i, state_nodes[i]));
  }

  return unroll_info;
}

}  // namespace xls
