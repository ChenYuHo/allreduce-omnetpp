#ifndef ALLREDUCECOORDINATOR_H_
#define ALLREDUCECOORDINATOR_H_
#include <omnetpp.h>
using namespace omnetpp;

namespace allreduce {

class AllReduceCoordinator: public cSimpleModule {
public:
    bool halvingdoubling_report_recv();
    void report_socket_established();
private:
    int n_workers;
    int n_received = 0;
    int n_socket_established = 0;
    int halving_doubling_phase = 0;
    int halving_doubling_n_phases;
    bool halving_doubling_reduce_scatter_phase = true;
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

} /* namespace allreduce */

#endif /* ALLREDUCECOORDINATOR_H_ */
