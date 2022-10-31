#include "AllReduceCoordinator.h"
#include "HalvingDoubling.h"
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
    n_workers = par("nWorkers");
    halving_doubling_n_phases = log2(n_workers);
}

void AllReduceCoordinator::handleMessage(cMessage *msg) {
    delete msg;
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
        n_received = 0;
        if (halving_doubling_reduce_scatter_phase) {
            // reduce-scatter
            auto mod = (1 << (halving_doubling_phase + 1));
            auto rhs = (1 << halving_doubling_phase) - 1;
            auto offset = (1 << halving_doubling_phase);
            EV_DEBUG << "find rank % " << mod << " == " << rhs << " -> rank "
                            << offset << endl;
            for (int rank = 0; rank < n_workers; ++rank) {
                if (rank % mod == rhs) {
                    auto app = (HalvingDoubling*) getModuleByPath(
                            fmt::format("^.hosts[{}].app[0]", rank).c_str());
                    auto rank_to_send = rank + offset;
                    app->send_data(rank_to_send);
                    EV_DEBUG
                                    << fmt::format(
                                            "[AllReduceCoordinator] reduce-scatter phase {} {} -> {}\n",
                                            halving_doubling_phase, rank,
                                            rank_to_send);
                }
            }
            halving_doubling_phase++;
            if (halving_doubling_phase == halving_doubling_n_phases) {
                halving_doubling_reduce_scatter_phase = false;
            }
        } else {
            if (defer) {
                defer = false;
            } else {
                if (!--halving_doubling_phase) {
                    EV_DEBUG << "AllReduce done!\n";
                    return true;
                }
            }
            auto mod = (1 << halving_doubling_phase);
            auto rhs = (1 << halving_doubling_phase) - 1;
            auto offset = -(1 << (halving_doubling_phase - 1));
            EV_DEBUG << "find rank % " << mod << " == " << rhs << " -> rank "
                            << offset << endl;
            for (int rank = 0; rank < n_workers; ++rank) {
                if (rank % mod == rhs) {
                    auto app = (HalvingDoubling*) getModuleByPath(
                            fmt::format("^.hosts[{}].app[0]", rank).c_str());
                    auto rank_to_send = rank + offset;
                    app->send_data(rank_to_send);
                    EV_DEBUG
                                    << fmt::format(
                                            "[AllReduceCoordinator] allgather phase {} {} -> {}\n",
                                            halving_doubling_phase, rank,
                                            rank_to_send);
                }
            }
        }

    }
    return false;
}

} /* namespace allreduce */
