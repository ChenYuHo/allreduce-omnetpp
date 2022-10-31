#include "HalvingDoubling.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"
namespace allreduce {

Define_Module(HalvingDoubling);

void HalvingDoubling::send_data(int send_to_rank) {
    Enter_Method_Silent();
    EV_DEBUG << "[HalvingDoubling " << rank << "] send to " << send_to_rank
                    << endl;
    const auto &payload = makeShared<GenericAppMsg>();
    payload->setChunkLength(B(tensor_size));
    Packet *packet = new Packet("data");
    packet->insertAtBack(payload);
    int numBytes = packet->getByteLength();
    sockets[send_to_rank].send(packet);
    packetsSent++;
    bytesSent += numBytes;
}

void HalvingDoubling::start_allreduce() {
}

bool HalvingDoubling::receive_complete_message(TcpSocket *socket,
        const GenericAppMsg *appmsg) {
    int sending_rank = socket->getLocalPort() - recvPortBase;
    EV_DEBUG << "[HalvingDoubling " << rank << "] received from "
                    << sending_rank << endl;
    return coordinator->halvingdoubling_report_recv();
}

} /* namespace allreduce */
