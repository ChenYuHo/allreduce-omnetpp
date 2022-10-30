//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef ALLREDUCECOORDINATOR_H_
#define ALLREDUCECOORDINATOR_H_
#include <omnetpp.h>
using namespace omnetpp;

namespace allreduce {

class AllReduceCoordinator: public cSimpleModule {
public:
    bool halvingdoubling_report_recv();
private:
    int n_workers;
    int n_received;
    int halving_doubling_phase = 1;
    int halving_doubling_n_phases;
    bool halving_doubling_reduce_scatter_phase = true;
    bool defer = true;
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

} /* namespace allreduce */

#endif /* ALLREDUCECOORDINATOR_H_ */
