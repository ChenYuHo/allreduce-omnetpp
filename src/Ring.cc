#include "Ring.h"
namespace allreduce {

Define_Module(Ring);

void Ring::start_allreduce() {
    Enter_Method_Silent();
    segment_sizes.resize(n_workers, tensor_size / n_workers);
    for (size_t i = 0; i < tensor_size % n_workers; ++i) {
        segment_sizes[i]++;
    }

    EV_DEBUG << "-------segment_sizes: ";
    for (auto ss : segment_sizes) {
        EV_DEBUG << ss << ", ";
    }
    EV_DEBUG << "\b\n";
    n_transmissions_needed = 2 * (n_workers - 1);
    send_to_rank = rank == n_workers - 1 ? 0 : rank + 1;
    send_segment();
}

void Ring::send_segment() {
    auto segment_index = (rank - (n_transmissions_sent % (n_workers - 1))
            + n_workers) % n_workers;
    auto segment_size = segment_sizes[segment_index];
    const auto &payload = makeShared<GenericAppMsg>();
    payload->setChunkLength(B(segment_size));
    Packet *packet = new Packet("data");
    packet->insertAtBack(payload);
    int numBytes = packet->getByteLength();
    sockets[send_to_rank].send(packet);
    packetsSent++;
    bytesSent += numBytes;
    n_transmissions_sent++;
    EV_INFO << "[RingAllReducer" << rank << "] sent " << n_transmissions_sent
                   << " chunks to rank " << send_to_rank << " , this one "
                   << numBytes << " bytes\n";
}

bool Ring::receive_complete_message(TcpSocket*, const GenericAppMsg *appmsg) {
    n_transmissions_received++;
    EV_DEBUG << "[AllReducer" << rank << "] received "
                    << n_transmissions_received << " complete messages "
                    << B(appmsg->getChunkLength()).get() << endl;
    if (n_transmissions_received < n_transmissions_needed) {
        send_segment();
    } else {
        // done allreduce
        return true;
    }
    return false;
}

} /* namespace allreduce */
