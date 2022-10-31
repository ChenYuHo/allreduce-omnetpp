#include "AllReduceCoordinator.h"
#include "HalvingDoubling.h"
#include "Ring.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"

namespace allreduce {

Define_Module(AllReduceCoordinator);

int log2(int num) {
    int log = 0;
    while (num >>= 1)
        ++log;
    return log;
}

void AllReduceCoordinator::initialize() {
    EV_DEBUG << "typename: "
                    << getModuleByPath(
                            fmt::format("^.hosts[{}].app[0]", 0).c_str())->getNedTypeName()
                    << endl;
    n_workers = par("nWorkers");
    halving_doubling_n_phases = log2(n_workers);
}

void AllReduceCoordinator::handleMessage(cMessage *msg) {
    delete msg;
}

void AllReduceCoordinator::report_socket_established() {
    if (++n_socket_established == (n_workers * (n_workers - 1))) {
        std::string algo(
                getModuleByPath("^.hosts[0].app[0]")->getNedTypeName());
        if (algo.find("Ring") != std::string::npos) {
            for (int rank = 0; rank < n_workers; ++rank) {
                auto app = (Ring*) getModuleByPath(
                        fmt::format("^.hosts[{}].app[0]", rank).c_str());
                app->start_allreduce();
            }
        } else if (algo.find("HalvingDoubling") != std::string::npos) {
            for (int rank = 0; rank < n_workers; ++rank) {
                if (rank % 2 == 0) {
                    auto app = (HalvingDoubling*) getModuleByPath(
                            fmt::format("^.hosts[{}].app[0]", rank).c_str());
                    auto rank_to_send = rank + 1;
                    EV_DEBUG
                                    << fmt::format(
                                            "[AllReduceCoordinator] all-gather phase {} {} -> {}\n",
                                            halving_doubling_phase, rank,
                                            rank_to_send);
                    app->send_data(rank_to_send);
                }
            }
            halving_doubling_phase = 1;
        }

    }
}

bool AllReduceCoordinator::halvingdoubling_report_recv() {
    n_received++;
    EV_DEBUG << "halving_doubling_phase " << halving_doubling_phase
                    << " reduce_scatter "
                    << halving_doubling_reduce_scatter_phase << " "
                    << n_received << " == "
                    << (n_workers / (1 << halving_doubling_phase)) << endl;
    if (n_received == (n_workers / (1 << halving_doubling_phase))) {
        // done this phase
        if (halving_doubling_reduce_scatter_phase) {
            // reduce-scatter
            if (n_received == 1) {
                halving_doubling_reduce_scatter_phase = false;
                // do first allgather phase
                auto rank = n_workers - 1;
                auto app = (HalvingDoubling*) getModuleByPath(
                        fmt::format("^.hosts[{}].app[0]", rank).c_str());
                auto rank_to_send = rank - n_workers / 2;
                EV_DEBUG
                                << fmt::format(
                                        "[AllReduceCoordinator] all-gather phase {} {} -> {}\n",
                                        halving_doubling_phase, rank,
                                        rank_to_send);
                app->send_data(rank_to_send);
            } else {
                auto mod = (1 << (halving_doubling_phase + 1));
                auto rhs = (1 << halving_doubling_phase) - 1;
                auto offset = (1 << halving_doubling_phase);
                EV_DEBUG << "find rank % " << mod << " == " << rhs
                                << " -> rank " << offset << endl;
                for (int rank = 0; rank < n_workers; ++rank) {
                    if (rank % mod == rhs) {
                        auto app =
                                (HalvingDoubling*) getModuleByPath(
                                        fmt::format("^.hosts[{}].app[0]", rank).c_str());
                        auto rank_to_send = rank + offset;
                        EV_DEBUG
                                        << fmt::format(
                                                "[AllReduceCoordinator] reduce-scatter phase {} {} -> {}\n",
                                                halving_doubling_phase, rank,
                                                rank_to_send);
                        app->send_data(rank_to_send);
                    }
                }
                halving_doubling_phase++;
            }
        } else {
            // all-gather

            if (n_received == n_workers / 2) {
                EV_DEBUG << "AllReduce done!\n";
                return true;
            }

//            if (defer) {
//                defer = false;
//            } else {
//                if (!--halving_doubling_phase) {
//                    EV_DEBUG << "AllReduce done!\n";
//                    return true;
//                }
//            }
            --halving_doubling_phase;
            auto mod = (1 << halving_doubling_phase);
            auto rhs = mod - 1;
            auto offset = -(1 << (halving_doubling_phase - 1));
            EV_DEBUG << "find rank % " << mod << " == " << rhs << " -> rank "
                            << offset << endl;
            for (int rank = 0; rank < n_workers; ++rank) {
                if (rank % mod == rhs) {
                    auto app = (HalvingDoubling*) getModuleByPath(
                            fmt::format("^.hosts[{}].app[0]", rank).c_str());
                    auto rank_to_send = rank + offset;
                    EV_DEBUG
                                    << fmt::format(
                                            "[AllReduceCoordinator] all-gather phase {} {} -> {}\n",
                                            halving_doubling_phase, rank,
                                            rank_to_send);
                    app->send_data(rank_to_send);
                }
            }

        }
        n_received = 0;
    }
    return false;
}

} /* namespace allreduce */
