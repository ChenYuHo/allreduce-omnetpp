#ifndef HALVINGDOUBLING_H_
#define HALVINGDOUBLING_H_

#include "AllReducer.h"
#include "AllReduceCoordinator.h"
#include <stack>

namespace allreduce {

class HalvingDoubling: public AllReducer {
public:
    void send_data(int s = -1);
private:
    void start_allreduce() override;
    bool receive_complete_message(TcpSocket*, const GenericAppMsg*) override;
    AllReduceCoordinator *coordinator;
};

} /* namespace allreduce */

#endif /* HALVINGDOUBLING_H_ */
