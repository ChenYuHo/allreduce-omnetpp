#include "HalvingDoubling.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
namespace allreduce {

Define_Module(HalvingDoubling);

//bool HalvingDoubling::participate_sending() {
//    // rank % 2^(phase+1)   ==   2^phase - 1
//    return rank % (1 << (phase + 1)) == (1 << phase) - 1;
//}
//
//bool HalvingDoubling::participate_receiving() {
//    // rank % 2^(phase+1)   ==   2^phase - 1
//    return rank % (1 << (phase + 1)) == (1 << (phase + 1)) - 1;
//}

//int HalvingDoubling::rank_to_send() {
//    return rank + (1 << phase);
//}

//HalvingDoubling* HalvingDoubling::get_reducer_of_rank(int rank) {
//    return (HalvingDoubling*) getModuleByPath(
//            fmt::format("^.^.hosts[{}].app[0]", rank).c_str());
//}

void HalvingDoubling::send_data(int send_to_rank) {
    Enter_Method_Silent();
//    if (send_to_rank == -1) {
//        send_to_rank = rank_stack.top();
//        rank_stack.pop();
//    }
    EV_DEBUG << "[HalvingDoubling " << rank << "] send to " << send_to_rank
                    << endl;
    const auto &payload = makeShared<GenericAppMsg>();
    payload->setChunkLength(B(tensor_size));
//    payload->setServerClose(upward); // use this bool to indicate upward/downward
    Packet *packet = new Packet("data");
    packet->insertAtBack(payload);
    int numBytes = packet->getByteLength();
    sockets[send_to_rank].send(packet);
    packetsSent++;
    bytesSent += numBytes;
}

void HalvingDoubling::start_allreduce() {
    // 0 --2-->2 4 6    0 to 1, 2 to 3, ...         % 2 == 0    1       recving: 1, 3, 5, ...
    //  1
    //   1  --4--> 5       1 to 3, 5 to 7, ...        % 4 == 1   3        recving: 3, 7, 11, ...
    //    2
    //     3  --8--> 11          3 to 7, 11 to 15, ...    % 8 == 3   7      recving: 7, 15
    //      4
    //       7  --16--> 23           7 to 15                 % 16 == 7
//    if (upward) {
//        phase++;
//    } else {
//        phase--;
//    }
//    if (phase == log2(n_workers) - 1) {
//        // 8 workers, rank 7:
//        upward = false;
//    }

    coordinator = (AllReduceCoordinator*) getModuleByPath("^.^.coordinator");
//    n_transmissions_needed_per_phase =
//    n_phases = log2(n_workers);
//    allreducers.reserve(n_workers);
//    for (int r = 0; r < n_workers; ++r) {
//        allreducers[r] = get_reducer_of_rank(r);
//    }
    if (rank % 2 == 0) {
//        auto send_rank = rank_to_send();
        send_data(rank + 1);
//        advance_phase();
    }
}

//void HalvingDoubling::advance_phase() {
//    if (upward) {
//        if (++phase == n_phases) {
//            upward = false;
//        }
//    } else {
//        --phase;
//    }
//}

bool HalvingDoubling::receive_complete_message(TcpSocket *socket,
        const GenericAppMsg *appmsg) {
//    n_transmissions_received++;
    int sending_rank = socket->getLocalPort() - recvPortBase;
    EV_DEBUG << "[HalvingDoubling " << rank << "] received from "
                    << sending_rank << endl;
//                    << B(appmsg->getChunkLength()).get() << " local port "
//                    << socket->getLocalPort() << " remote port "
//                    << socket->getRemotePort() << endl;

//    advance_phase();
    return coordinator->halvingdoubling_report_recv();

//    if (!stop_push_rank) {
//        rank_stack.push(sending_rank);
//    }
//    if (upward) {
//        if (participate_sending()) {
//            stop_push_rank = true;
//            send_data(rank_to_send());
//            advance_phase();
//        }
//    } else {
//        // distribute phase, pop and send to rank, will only enter once
//        send_data(rank_stack.top());
//        allreducers[sending_rank]->send_data();
//        advance_phase();
//        rank_stack.pop();
//    }

//    if (appmsg->getServerClose()) { // upward phases
//        start_allreduce();
//    } else {
//        // done allreduce
//        return true;
//    }
//    return false;
}

} /* namespace allreduce */
