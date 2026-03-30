#include "ExplicitStateDfa.h"

#include <algorithm>

namespace Syft {

ExplicitStateDfa::ExplicitStateDfa(std::shared_ptr<VarMgr> var_mgr)
    : var_mgr_(std::move(var_mgr)) {}

CUDD::ADD ExplicitStateDfa::build_add_from_minterm_map(
    const std::shared_ptr<VarMgr>& var_mgr,
    const std::vector<std::string>& variable_names,
    const std::unordered_map<uint64_t, size_t>& transition_map,
    int var_idx,
    int num_vars,
    uint64_t current_prefix) {
  if (var_idx == num_vars) {
    auto it = transition_map.find(current_prefix);
    if (it != transition_map.end()) {
      return var_mgr->cudd_mgr()->constant(it->second);
    }
    // Incomplete DFA: missing transition defaults to state 0.
    return var_mgr->cudd_mgr()->constant(0);
  }

  uint64_t bit = 1ULL << var_idx;

  CUDD::ADD low_child = build_add_from_minterm_map(
      var_mgr, variable_names, transition_map, var_idx + 1, num_vars,
      current_prefix);
  CUDD::ADD high_child = build_add_from_minterm_map(
      var_mgr, variable_names, transition_map, var_idx + 1, num_vars,
      current_prefix | bit);

  // CUDD shares identical subtrees automatically; also skip nodes where both
  // children are equal (variable doesn't affect result).
  if (low_child == high_child) {
    return low_child;
  }

  CUDD::BDD var_bdd = var_mgr->name_to_variable(variable_names[var_idx]);
  return var_bdd.Add().Ite(high_child, low_child);
}

ExplicitStateDfa ExplicitStateDfa::from_explicit_table(
    std::shared_ptr<VarMgr> var_mgr,
    const std::vector<std::string>& variable_names,
    size_t initial_state,
    size_t state_count,
    const std::vector<size_t>& final_states,
    const std::vector<std::unordered_map<uint64_t, size_t>>& transition_table) {
  var_mgr->create_named_variables(variable_names);
  int num_vars = static_cast<int>(variable_names.size());

  std::vector<CUDD::ADD> transition_function(state_count);
  for (size_t s = 0; s < state_count; ++s) {
    const auto& tmap = transition_table[s];
    transition_function[s] = build_add_from_minterm_map(
        var_mgr, variable_names, tmap, 0, num_vars, 0);
  }

  ExplicitStateDfa dfa(std::move(var_mgr));
  dfa.initial_state_ = initial_state;
  dfa.state_count_ = state_count;
  dfa.final_states_ = final_states;
  dfa.transition_function_ = std::move(transition_function);
  return dfa;
}

ExplicitStateDfa ExplicitStateDfa::complement_dfa(ExplicitStateDfa& d) {
  std::vector<size_t> final_states;
  std::vector<size_t> current_final = d.final_states();
  for (size_t i = 0; i < d.state_count(); ++i) {
    if (std::find(current_final.begin(), current_final.end(), i) ==
        current_final.end()) {
      final_states.push_back(i);
    }
  }

  ExplicitStateDfa dfa(d.var_mgr());
  dfa.initial_state_ = d.initial_state();
  dfa.state_count_ = d.state_count();
  dfa.final_states_ = std::move(final_states);
  dfa.transition_function_ = d.transition_function();
  return dfa;
}

std::shared_ptr<VarMgr> ExplicitStateDfa::var_mgr() const { return var_mgr_; }
std::size_t ExplicitStateDfa::initial_state() const { return initial_state_; }
std::size_t ExplicitStateDfa::state_count() const { return state_count_; }
std::vector<std::size_t> ExplicitStateDfa::final_states() const { return final_states_; }
std::vector<CUDD::ADD> ExplicitStateDfa::transition_function() const { return transition_function_; }

void ExplicitStateDfa::dump_dot(const std::string& filename) const {
  std::vector<std::string> labels(state_count_);
  for (std::size_t i = 0; i < state_count_; ++i) {
    labels[i] = "S" + std::to_string(i);
  }
  var_mgr_->dump_dot(transition_function_, labels, filename);
}

}  // namespace Syft
