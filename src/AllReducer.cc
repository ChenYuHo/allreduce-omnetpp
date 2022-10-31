#include "AllReducer.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/socket/SocketTag_m.h"
#define FMT_HEADER_ONLY
#include "fmt/format.h"

using namespace inet;

namespace allreduce {

Define_Module(AllReducer);

void AllReducer::initialize(int stage) {
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        n_workers = par("nWorkers");
        rank = getParentModule()->getIndex();
        tensor_size = par("tensorSize");
        sockets.resize(n_workers * 2); // send & recv
        for (size_t i = 0; i < sockets.size(); ++i) {
            socket_index[sockets[i].getSocketId()] = i;
        }

        packetsSent = packetsRcvd = bytesSent = bytesRcvd = 0;
//        WATCH(packetsSent);
//        WATCH(packetsRcvd);
//        WATCH(bytesSent);
//        WATCH(bytesRcvd);
        allReduceTime = registerSignal("allReduceTime");
        ATE_s = registerSignal("ATE_s");
        coordinator = (AllReduceCoordinator*) getModuleByPath(
                "^.^.coordinator");
    } else if (stage == INITSTAGE_APPLICATION_LAYER) {
        connect();
    }
}

void AllReducer::connect() {
    // Assume Ipv4NetworkConfigurator uses 10.0.0.x subnets
    auto local = fmt::format("10.0.0.{}", rank + 1);
    const char *localAddress = local.c_str();
    auto l3_address =
            *localAddress ?
                    L3AddressResolver().resolve(localAddress) : L3Address();
    sendPortBase = par("sendPortBase");
    recvPortBase = par("recvPortBase");
    int timeToLive = par("timeToLive");
    int dscp = par("dscp");
    int tos = par("tos");
    auto out_gate = gate("socketOut");
    for (int dest_rank = 0; dest_rank < n_workers; ++dest_rank) {
        // this->rank sendPort connect to rank recvPort
        if (dest_rank == this->rank)
            continue;
        auto peer = fmt::format("10.0.0.{}", dest_rank + 1);
        const char *connectAddress = peer.c_str();
        int localPort = sendPortBase + dest_rank;
        int remotePort = recvPortBase + this->rank;
        auto &recv_socket = sockets[dest_rank + n_workers];
        recv_socket.bind(l3_address, recvPortBase + dest_rank); // wait for connection
        recv_socket.setCallback(this);
        recv_socket.setOutputGate(out_gate);
        recv_socket.listenOnce();
        auto &send_socket = sockets[dest_rank];
        send_socket.bind(l3_address, localPort);
        send_socket.setCallback(this);
        send_socket.setOutputGate(out_gate);
        if (timeToLive != -1) {
            send_socket.setTimeToLive(timeToLive);
            recv_socket.setTimeToLive(timeToLive);
        }
        if (dscp != -1) {
            send_socket.setDscp(dscp);
            recv_socket.setDscp(dscp);
        }
        if (tos != -1) {
            send_socket.setTos(tos);
            recv_socket.setTos(tos);
        }
        L3Address destination;
        L3AddressResolver().tryResolve(connectAddress, destination);
        if (destination.isUnspecified()) {
            EV_ERROR << "[AllReducer" << this->rank << "] Connecting to "
                            << connectAddress << " port=" << remotePort
                            << ": cannot resolve destination address\n";
        } else {
            EV_INFO << "[AllReducer" << this->rank << "] Connecting to "
                           << connectAddress << "(" << destination << ") port="
                           << remotePort << " local address " << localAddress
                           << " local port=" << localPort << endl;
            send_socket.connect(destination, remotePort);
        }
    }
}

void AllReducer::start_allreduce() {
    EV_FATAL << "start_allreduce Not implemented" << endl;
}

void AllReducer::handleMessageWhenUp(cMessage *msg) {
    if (!msg->isSelfMessage()) {
        auto conn_id = check_and_cast<ITaggedObject*>(msg)->getTags().findTag<
                SocketInd>()->getSocketId();
        auto index = socket_index[conn_id];

        EV_DEBUG << index << " " << conn_id << " [AllReducer" << rank << "] "
                        << ((index >= n_workers) ? "recv" : "send")
                        << "_socket from rank " << index % n_workers
                        << " processMessage" << endl;
        sockets[index].processMessage(msg);
    }
}

void AllReducer::close() {
//    EV_INFO << "[AllReducer" << rank << "] issuing CLOSE command\n";
//    send_socket.close();
}

void AllReducer::refreshDisplay() const {
//    ApplicationBase::refreshDisplay();
//    getDisplayString().setTagArg("t", 0,
//            TcpSocket::stateName(send_socket.getState()));
}

void AllReducer::socketEstablished(TcpSocket *socket) {
    if (socket_index[socket->getSocketId()] < n_workers) {
        // send socket
        EV_INFO << "[AllReducer" << rank << "] connected to rank "
                       << socket_index[socket->getSocketId()] << "\n";
    } else {
        // recv socket
        EV_INFO << "[AllReducer" << rank << "] got connected from rank "
                       << socket_index[socket->getSocketId()] << "\n";
    }
    coordinator->report_socket_established();
}

bool AllReducer::receive_complete_message(TcpSocket*, const GenericAppMsg*) {
    EV_FATAL << "receive_complete_message Not implemented" << endl;
    return false;
}

void AllReducer::socketDataArrived(TcpSocket *socket, Packet *msg, bool) {
    EV_DEBUG << "[AllReducer" << rank << "] ByteLength: "
                    << msg->getByteLength() << endl;
    packetsRcvd++;
    bytesRcvd += msg->getByteLength();
    auto chunk = msg->peekDataAt(B(0), msg->getTotalLength());
    queue.push(chunk);
    while (queue.has<GenericAppMsg>()) {
        const auto &appmsg = queue.pop<GenericAppMsg>(b(-1));
        if (receive_complete_message(socket, appmsg.get())) {
            // allreduce done
            auto allreduce_time = simTime() - allReduceStart;
            emit(allReduceTime, allreduce_time);
            emit(ATE_s, tensor_size / 4 / allreduce_time.dbl());
            EV_DEBUG << "EMITTTT" << endl;
        }
    }
    delete msg;
}

void AllReducer::socketPeerClosed(TcpSocket *socket_) {
//    ASSERT(socket_ == &send_socket);
//    // close the connection (if not already closed)
//    if (send_socket.getState() == TcpSocket::PEER_CLOSED) {
//        EV_INFO << "[AllReducer" << rank << "] remote TCP closed, closing here as well\n";
//        close();
//    }
}

void AllReducer::socketClosed(TcpSocket*) {
    // *redefine* to start another session etc.
    EV_INFO << "[AllReducer" << rank << "] connection closed\n";
}

void AllReducer::socketFailure(TcpSocket*, int code) {
    // subclasses may override this function, and add code try to reconnect after a delay.
    EV_WARN << "[AllReducer" << rank << "] connection broken\n";
//    numBroken++;
}

void AllReducer::finish() {
    std::string modulePath = getFullPath();

//    EV_INFO << modulePath << ": opened " << numSessions << " sessions\n";
    EV_INFO << "[AllReducer" << rank << "] " << modulePath << ": sent "
                   << bytesSent << " bytes in " << packetsSent << " packets\n";
    EV_INFO << "[AllReducer" << rank << "] " << modulePath << ": received "
                   << bytesRcvd << " bytes in " << packetsRcvd << " packets\n";
}

void AllReducer::handleStartOperation(LifecycleOperation *operation) {
//    if (simTime() <= tOpen) {
//        timeoutMsg->setKind(MSGKIND_CONNECT);
//        scheduleAt(tOpen, timeoutMsg);
//    }
}

void AllReducer::handleStopOperation(LifecycleOperation *operation) {
//    cancelEvent(timeoutMsg);
//    if (socket.isOpen())
//        close();
//    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void AllReducer::handleCrashOperation(LifecycleOperation *operation) {
//    cancelEvent(timeoutMsg);
//    if (operation->getRootModule() != getContainingNode(this))
//        socket.destroy();
}

} /* namespace allreduce */
