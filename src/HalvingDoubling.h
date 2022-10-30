#ifndef HALVINGDOUBLING_H_
#define HALVINGDOUBLING_H_

#include "AllReducer.h"
#include "AllReduceCoordinator.h"
#include <stack>
//#include <vector>

namespace allreduce {

class HalvingDoubling: public AllReducer {
public:
    void send_data(int s = -1);
private:
    void start_allreduce() override;
    bool receive_complete_message(TcpSocket*, const GenericAppMsg*) override;
//    int phase = 0;
//    bool upward = true;
//    bool stop_push_rank = false;
//    bool participate_sending();
//    bool participate_receiving();
//    int n_phases;
//    std::stack<int> rank_stack { };
//    std::vector<HalvingDoubling*> allreducers { };
    AllReduceCoordinator *coordinator;

//    HalvingDoubling* get_reducer_of_rank(int);

//    int rank_to_send();

//    void advance_phase();

//    int n_transmissions_needed_per_phase;
//    int n_transmissions_sent = 0;
//    int n_transmissions_received = 0;
};

} /* namespace allreduce */

#endif /* HALVINGDOUBLING_H_ */
