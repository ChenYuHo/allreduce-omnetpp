#ifndef RING_H_
#define RING_H_

#include "AllReducer.h"

namespace allreduce {

class Ring: public AllReducer {
private:
    void start_allreduce() override;
    bool receive_complete_message(TcpSocket*, const GenericAppMsg*) override;
    std::vector<size_t> segment_sizes;
    int n_transmissions_needed;
    int n_transmissions_sent = 0;
    int n_transmissions_received = 0;
    int send_to_rank;
    void send_segment();
};

} /* namespace allreduce */

#endif /* RING_H_ */
