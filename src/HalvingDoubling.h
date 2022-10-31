#ifndef HALVINGDOUBLING_H_
#define HALVINGDOUBLING_H_

#include "AllReducer.h"
#include <stack>

namespace allreduce {

class HalvingDoubling: public AllReducer {
    friend AllReduceCoordinator;
private:
    void send_data(int s = -1);
    void start_allreduce() override;
    bool receive_complete_message(TcpSocket*, const GenericAppMsg*) override;
};

} /* namespace allreduce */

#endif /* HALVINGDOUBLING_H_ */
