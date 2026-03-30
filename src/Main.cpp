#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Stopwatch.h"
#include "ExplicitStateDfa.h"
#include "ReachabilityMaxSetSynthesizer.h"
#include "InputOutputPartition.h"
#include <CLI/CLI.hpp>

namespace {

struct DfaFileData {
    std::vector<std::string> variable_names;
    size_t initial_state = 0;
    size_t state_count = 0;
    std::vector<size_t> final_states;
    // transition_table[s][minterm_bitmask] = next_state
    // Bit j of the bitmask = value of variable_names[j]
    std::vector<std::unordered_map<uint64_t, size_t>> transition_table;
};

/**
 * Parse a .dfa file.
 *
 * Format:
 *   .vars: X1 X2 Y1 Y2        -- alphabet variables in order
 *   .states: 4                 -- total state count (optional; inferred if absent)
 *   .initial: 0                -- initial state index
 *   .accepting: 2 3            -- accepting state indices (space-separated)
 *   .transitions:              -- one transition per line below:
 *   0 0000 1                   -- from_state  minterm  to_state
 *
 * Minterm is a binary string of length |vars| where character at position j
 * encodes the value (0 or 1) of variable_names[j].
 * Lines starting with '#' and blank lines are ignored.
 */
DfaFileData parse_dfa_file(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) {
        throw std::runtime_error("Cannot open DFA file: " + filename);
    }

    DfaFileData data;
    std::string line;
    bool in_transitions = false;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line.rfind(".vars:", 0) == 0) {
            std::istringstream iss(line.substr(6));
            std::string var;
            while (iss >> var) data.variable_names.push_back(var);
            in_transitions = false;
        } else if (line.rfind(".states:", 0) == 0) {
            data.state_count = std::stoul(line.substr(8));
            in_transitions = false;
        } else if (line.rfind(".initial:", 0) == 0) {
            data.initial_state = std::stoul(line.substr(9));
            in_transitions = false;
        } else if (line.rfind(".accepting:", 0) == 0) {
            std::istringstream iss(line.substr(11));
            size_t s;
            while (iss >> s) data.final_states.push_back(s);
            in_transitions = false;
        } else if (line == ".transitions:") {
            in_transitions = true;
        } else if (in_transitions) {
            std::istringstream iss(line);
            size_t from_state;
            std::string minterm_str;
            size_t to_state;
            if (!(iss >> from_state >> minterm_str >> to_state)) continue;

            // Build bitmask: bit j set iff minterm_str[j] == '1'
            uint64_t minterm = 0;
            for (size_t j = 0; j < minterm_str.size(); ++j) {
                if (minterm_str[j] == '1') {
                    minterm |= (1ULL << j);
                }
            }

            if (from_state >= data.transition_table.size()) {
                data.transition_table.resize(from_state + 1);
            }
            data.transition_table[from_state][minterm] = to_state;
        }
    }

    // If .states: was not given, infer from max state index seen
    if (data.state_count == 0) {
        size_t max_state = data.initial_state;
        for (size_t s : data.final_states) max_state = std::max(max_state, s);
        for (auto& tmap : data.transition_table) {
            for (auto& [m, t] : tmap) max_state = std::max(max_state, t);
        }
        data.state_count = max_state + 1;
    }

    data.transition_table.resize(data.state_count);
    return data;
}

}  // namespace


int main(int argc, char** argv) {
    CLI::App app{"Syft: reactive synthesis from a DFA game"};

    std::string dfa_file, partition_file;
    app.add_option("-d,--dfa-file", dfa_file, "DFA specification file (.dfa)")
        ->required()->check(CLI::ExistingFile);
    app.add_option("-p,--partition-file", partition_file, "Input/output partition file (.part)")
        ->required()->check(CLI::ExistingFile);

    bool env_start = false;
    app.add_flag("-e,--environment", env_start,
                 "Environment moves first (default: Agent moves first)");

    bool maxset = false;
    app.add_flag("-m,--maxset", maxset, "Compute maximally permissive strategy");

    bool fanin_encoding = false;
    app.add_flag("--fanin", fanin_encoding, "Use fanin state encoding");

    bool fanout_encoding = false;
    app.add_flag("--fanout", fanout_encoding, "Use fanout state encoding");

    CLI11_PARSE(app, argc, argv);

    Syft::Stopwatch total_time_stopwatch;
    total_time_stopwatch.start();

    Syft::Player starting_player =
        env_start ? Syft::Player::Environment : Syft::Player::Agent;
    Syft::Player protagonist_player = Syft::Player::Agent;

    Syft::InputOutputPartition partition =
        Syft::InputOutputPartition::read_from_file(partition_file);

    std::shared_ptr<Syft::VarMgr> var_mgr = std::make_shared<Syft::VarMgr>();

    Syft::Stopwatch aut_time_stopwatch;
    aut_time_stopwatch.start();

    DfaFileData dfa_data = parse_dfa_file(dfa_file);
    Syft::ExplicitStateDfa explicit_dfa = Syft::ExplicitStateDfa::from_explicit_table(
        var_mgr,
        dfa_data.variable_names,
        dfa_data.initial_state,
        dfa_data.state_count,
        dfa_data.final_states,
        dfa_data.transition_table);

    Syft::SymbolicStateDfa symbolic_dfa =
        fanout_encoding
            ? Syft::SymbolicStateDfa::from_explicit_fanout_encoding(explicit_dfa)
        : fanin_encoding
            ? Syft::SymbolicStateDfa::from_explicit_fanin_encoding(explicit_dfa)
        : Syft::SymbolicStateDfa::from_explicit(explicit_dfa);

    auto aut_time = aut_time_stopwatch.stop();
    std::cout << "DFA construction time: " << aut_time.count() << " ms\n";
    std::cout << "BDD nodes (total / transitions / final states): "
              << symbolic_dfa.bdd_nodes_num() << " / "
              << symbolic_dfa.bdd_nodes_num_transitions() << " / "
              << symbolic_dfa.bdd_nodes_num_final_states() << "\n";

    var_mgr->partition_variables(partition.input_variables,
                                 partition.output_variables);

    Syft::Stopwatch syn_time_stopwatch;
    syn_time_stopwatch.start();

    Syft::ReachabilityMaxSetSynthesizer synthesizer(
        symbolic_dfa, starting_player, protagonist_player,
        symbolic_dfa.final_states(), var_mgr->cudd_mgr()->bddOne());

    Syft::SynthesisResult result = synthesizer.run();

    if (result.realizability) {
        std::cout << "REALIZABLE\n";

        if (!maxset) {
            Syft::Stopwatch strategy_stopwatch;
            strategy_stopwatch.start();
            auto transducer = synthesizer.AbstractSingleStrategy(std::move(result));
            // transducer->dump_dot("strategy.dot");
            std::cout << "Single strategy extraction time: "
                      << strategy_stopwatch.stop().count() << " ms\n";
        } else {
            Syft::Stopwatch strategy_stopwatch;
            strategy_stopwatch.start();
            Syft::MaxSet maxset_result = synthesizer.AbstractMaxSet(std::move(result));
            std::cout << "MaxSet extraction time: "
                      << strategy_stopwatch.stop().count() << " ms\n";
        }
    } else {
        std::cout << "UNREALIZABLE\n";
    }

    std::cout << "Synthesis time: " << syn_time_stopwatch.stop().count() << " ms\n";
    std::cout << "Total time:     " << total_time_stopwatch.stop().count() << " ms\n";

    return 0;
}
