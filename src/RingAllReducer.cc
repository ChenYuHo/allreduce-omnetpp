#include "RingAllReducer.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"

using namespace inet;

namespace allreduce {

Define_Module(RingAllReducer);

void RingAllReducer::initialize(int stage) {
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        n_workers = par("nWorkers");
        rank = getParentModule()->getIndex();

        packetsSent = packetsRcvd = bytesSent = bytesRcvd = 0;
        WATCH(packetsSent);
        WATCH(packetsRcvd);
        WATCH(bytesSent);
        WATCH(bytesRcvd);
        allReduceTime = registerSignal("allReduceTime");
        ATE_s = registerSignal("ATE_s");
    } else if (stage == INITSTAGE_APPLICATION_LAYER) {
        connect();
    }
}

void RingAllReducer::connect() {
    auto local = fmt::format("10.0.0.{}", rank + 1);
    auto next_neighbor = fmt::format("10.0.0.{}",
            rank == n_workers - 1 ? 1 : rank + 2);
    const char *localAddress = local.c_str();
//    const char *connectAddress = par("connectAddress");
    const char *connectAddress = next_neighbor.c_str();
    int sendPort = par("sendPort");
    int recvPort = par("recvPort");

    auto l3_address =
            *localAddress ?
                    L3AddressResolver().resolve(localAddress) : L3Address();
    EV_DEBUG << "--------------- " << *localAddress << " , "
                    << L3AddressResolver().resolve(localAddress) << " , "
                    << L3Address() << endl;
    recv_socket.bind(l3_address, recvPort);
    recv_socket.setCallback(this);
    recv_socket.setOutputGate(gate("socketOut"));
    recv_socket.listenOnce();

    send_socket.bind(l3_address, sendPort);
    send_socket.setCallback(this);
    send_socket.setOutputGate(gate("socketOut"));

    int timeToLive = par("timeToLive");
    if (timeToLive != -1) {
        send_socket.setTimeToLive(timeToLive);
        recv_socket.setTimeToLive(timeToLive);
    }

    int dscp = par("dscp");
    if (dscp != -1) {
        send_socket.setDscp(dscp);
        recv_socket.setDscp(dscp);
    }

    int tos = par("tos");
    if (tos != -1) {
        send_socket.setTos(tos);
        recv_socket.setTos(tos);
    }
    // connect
    L3Address destination;
    L3AddressResolver().tryResolve(connectAddress, destination);
    if (destination.isUnspecified()) {
        EV_ERROR << "[RingAllReducer" << rank << "] Connecting to "
                        << connectAddress << " port=" << recvPort
                        << ": cannot resolve destination address\n";
    } else {
        EV_INFO << "[RingAllReducer" << rank << "] Connecting to "
                       << connectAddress << "(" << destination << ") port="
                       << recvPort << endl;

        send_socket.connect(destination, recvPort);
    }
}

void RingAllReducer::allreduce() {
    tensor_size = par("tensorSize");
    int n_workers = par("nWorkers");

    segment_size = tensor_size / n_workers;
    segment_sizes.resize(n_workers, segment_size);

    const size_t residual = tensor_size % n_workers;
    for (size_t i = 0; i < residual; ++i) {
        segment_sizes[i]++;
    }

    EV_DEBUG << "-------segment_sizes: ";
    for (auto ss : segment_sizes) {
        EV_DEBUG << ss << ", ";
    }
    EV_DEBUG << "\b\n";

    n_transmissions_needed = 2 * (n_workers - 1);

    allReduceStart = simTime();

    sendSegment();
}

void RingAllReducer::handleMessageWhenUp(cMessage *msg) {
    if (msg->isSelfMessage()) {

    } else if (send_socket.belongsToSocket(msg)) {
        EV_DEBUG << "[RingAllReducer" << rank << "] send_socket processMessage"
                        << endl;
        send_socket.processMessage(msg);
    } else { //if (serverSocket.belongsToSocket(msg)) {
        EV_DEBUG << "[RingAllReducer" << rank << "] recv_socket processMessage"
                        << endl;
        recv_socket.processMessage(msg);
    }
}

void RingAllReducer::close() {
    EV_INFO << "[RingAllReducer" << rank << "] issuing CLOSE command\n";
    send_socket.close();
}

void RingAllReducer::sendSegment() {
    auto segment_index = (rank - (n_transmissions_sent % (n_workers - 1))
            + n_workers) % n_workers;
    auto segment_size = segment_sizes[segment_index];
    const auto &payload = makeShared<GenericAppMsg>();
    payload->setChunkLength(B(segment_size));
    Packet *packet = new Packet("data");
    packet->insertAtBack(payload);
    int numBytes = packet->getByteLength();
    send_socket.send(packet);
    packetsSent++;
    bytesSent += numBytes;
    n_transmissions_sent++;
    EV_INFO << "[RingAllReducer" << rank << "] sent " << n_transmissions_sent
                   << " chunks , this one " << numBytes << " bytes\n";
}

void RingAllReducer::refreshDisplay() const {
    ApplicationBase::refreshDisplay();
    getDisplayString().setTagArg("t", 0,
            TcpSocket::stateName(send_socket.getState()));
}

void RingAllReducer::socketEstablished(TcpSocket *socket) {
    // *redefine* to perform or schedule first sending
    EV_INFO << "[RingAllReducer" << rank << "] connected\n";
    if (socket == &send_socket) {
        allreduce();
    }
}

void RingAllReducer::socketDataArrived(TcpSocket*, Packet *msg, bool) {
    EV_DEBUG << "[RingAllReducer" << rank << "] ByteLength: "
                    << msg->getByteLength() << endl;
    packetsRcvd++;
    bytesRcvd += msg->getByteLength();
    auto chunk = msg->peekDataAt(B(0), msg->getTotalLength());
    queue.push(chunk);
    while (queue.has<GenericAppMsg>(b(-1))) {
        const auto &appmsg = queue.pop<GenericAppMsg>(b(-1));
        n_transmissions_received++;
        EV_DEBUG << "[RingAllReducer" << rank << "] received "
                        << n_transmissions_received << " complete messages "
                        << B(appmsg->getChunkLength()).get() << endl;

        if (n_transmissions_received < n_transmissions_needed) {
            sendSegment();
        } else {
            // done
            auto allreduce_time = simTime() - allReduceStart;
            emit(allReduceTime, allreduce_time);
            emit(ATE_s, tensor_size / 4 / allreduce_time.dbl());
        }

//        msgsRcvd++;
//        bytesRcvd += B(appmsg->getChunkLength()).get();
//        B requestedBytes = appmsg->getExpectedReplyLength();
//        simtime_t msgDelay = appmsg->getReplyDelay();
//        if (msgDelay > maxMsgDelay)
//            maxMsgDelay = msgDelay;

//        if (requestedBytes > B(0)) {
//            Packet *outPacket = new Packet(msg->getName(), TCP_C_SEND);
//            outPacket->addTag<SocketReq>()->setSocketId(connId);
//            const auto &payload = makeShared<GenericAppMsg>();
//            payload->setChunkLength(requestedBytes);
//            payload->setExpectedReplyLength(B(0));
//            payload->setReplyDelay(0);
//            payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
//            outPacket->insertAtBack(payload);
//            sendOrSchedule(outPacket, delay + msgDelay);
//        }
//        if (appmsg->getServerClose()) {
//            doClose = true;
//            break;
//        }
    }
    delete msg;

//    if (doClose) {
//        auto request = new Request("close", TCP_C_CLOSE);
//        TcpCommand *cmd = new TcpCommand();
//        request->addTag<SocketReq>()->setSocketId(connId);
//        request->setControlInfo(cmd);
//        sendOrSchedule(request, delay + maxMsgDelay);
//    }
}

void RingAllReducer::socketPeerClosed(TcpSocket *socket_) {
    ASSERT(socket_ == &send_socket);
    // close the connection (if not already closed)
    if (send_socket.getState() == TcpSocket::PEER_CLOSED) {
        EV_INFO << "[RingAllReducer" << rank
                       << "] remote TCP closed, closing here as well\n";
        close();
    }
}

void RingAllReducer::socketClosed(TcpSocket*) {
    // *redefine* to start another session etc.
    EV_INFO << "[RingAllReducer" << rank << "] connection closed\n";
}

void RingAllReducer::socketFailure(TcpSocket*, int code) {
    // subclasses may override this function, and add code try to reconnect after a delay.
    EV_WARN << "[RingAllReducer" << rank << "] connection broken\n";
//    numBroken++;
}

void RingAllReducer::finish() {
    std::string modulePath = getFullPath();

//    EV_INFO << modulePath << ": opened " << numSessions << " sessions\n";
    EV_INFO << "[RingAllReducer" << rank << "] " << modulePath << ": sent "
                   << bytesSent << " bytes in " << packetsSent << " packets\n";
    EV_INFO << "[RingAllReducer" << rank << "] " << modulePath << ": received "
                   << bytesRcvd << " bytes in " << packetsRcvd << " packets\n";
}

void RingAllReducer::handleStartOperation(LifecycleOperation *operation) {
//    if (simTime() <= tOpen) {
//        timeoutMsg->setKind(MSGKIND_CONNECT);
//        scheduleAt(tOpen, timeoutMsg);
//    }
}

void RingAllReducer::handleStopOperation(LifecycleOperation *operation) {
//    cancelEvent(timeoutMsg);
//    if (socket.isOpen())
//        close();
//    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void RingAllReducer::handleCrashOperation(LifecycleOperation *operation) {
//    cancelEvent(timeoutMsg);
//    if (operation->getRootModule() != getContainingNode(this))
//        socket.destroy();
}

} /* namespace allreduce */
