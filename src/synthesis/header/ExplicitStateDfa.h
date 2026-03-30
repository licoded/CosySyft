#ifndef EXPLICIT_STATE_DFA_H
#define EXPLICIT_STATE_DFA_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "VarMgr.h"

namespace Syft {

/**
 * \brief A DFA with explicit states and symbolic transitions.
 *
 * States are 0-indexed. The transition function is represented as a vector of
 * CUDD ADDs, one per state: transition_function_[s] maps an assignment of
 * alphabet variables to the index of the successor state.
 */
class ExplicitStateDfa {
 private:
  std::shared_ptr<VarMgr> var_mgr_;

  size_t initial_state_;
  size_t state_count_;
  std::vector<size_t> final_states_;
  std::vector<CUDD::ADD> transition_function_;

  ExplicitStateDfa(std::shared_ptr<VarMgr> var_mgr);

  /**
   * \brief Recursively builds a CUDD ADD from an explicit minterm->state map.
   *
   * Uses Shannon expansion over variable_names[var_idx]. Bit j of the minterm
   * bitmask corresponds to variable_names[j] (bit set = variable true).
   */
  static CUDD::ADD build_add_from_minterm_map(
      const std::shared_ptr<VarMgr>& var_mgr,
      const std::vector<std::string>& variable_names,
      const std::unordered_map<uint64_t, size_t>& transition_map,
      int var_idx,
      int num_vars,
      uint64_t current_prefix);

 public:

  /**
   * \brief Constructs an ExplicitStateDfa from a complete explicit transition table.
   *
   * \param var_mgr          The variable manager (variables will be registered).
   * \param variable_names   Ordered alphabet variables; position j corresponds
   *                         to bit j in the minterm bitmask.
   * \param initial_state    Index of the initial state (0-based).
   * \param state_count      Total number of states.
   * \param final_states     Indices of accepting states.
   * \param transition_table transition_table[s][minterm] = next_state.
   *                         Minterm is a bitmask with bit j set iff
   *                         variable_names[j] = 1 in that transition.
   */
  static ExplicitStateDfa from_explicit_table(
      std::shared_ptr<VarMgr> var_mgr,
      const std::vector<std::string>& variable_names,
      size_t initial_state,
      size_t state_count,
      const std::vector<size_t>& final_states,
      const std::vector<std::unordered_map<uint64_t, size_t>>& transition_table);

  /**
   * \brief Complements a DFA by swapping accepting and non-accepting states.
   */
  static ExplicitStateDfa complement_dfa(ExplicitStateDfa& d);

  std::shared_ptr<VarMgr> var_mgr() const;
  std::size_t initial_state() const;
  std::size_t state_count() const;
  std::vector<std::size_t> final_states() const;

  /**
   * \brief Returns the transition function as a vector of ADDs.
   *
   * transition_function()[s] is an ADD over alphabet variables whose leaves
   * are successor state indices.
   */
  std::vector<CUDD::ADD> transition_function() const;

  void dump_dot(const std::string& filename) const;
};

}  // namespace Syft

#endif  // EXPLICIT_STATE_DFA_H
