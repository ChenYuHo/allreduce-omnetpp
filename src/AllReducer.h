#ifndef RINGALLREDUCER_H_
#define RINGALLREDUCER_H_

#include <omnetpp.h>
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/common/packet/ChunkQueue.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include <unordered_map>

using namespace omnetpp;
using namespace inet;

namespace allreduce {

class AllReducer: public ApplicationBase, public TcpSocket::ICallback {
protected:
    std::vector<TcpSocket> sockets; // pair to pair connections
    std::unordered_map<int, size_t> socket_index { };
    ChunkQueue queue;
    int n_workers;
    int rank;
    int tensor_size;
    int sockets_established = 0;
    int sendPortBase;
    int recvPortBase;

    // statistics
    int packetsSent;
    int packetsRcvd;
    int bytesSent;
    int bytesRcvd;
    simtime_t allReduceStart;
    simsignal_t allReduceTime;
    simsignal_t ATE_s;

    virtual void start_allreduce();
    virtual bool receive_complete_message(TcpSocket*, const GenericAppMsg*);
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override {
        return NUM_INIT_STAGES;
    }
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;
    virtual void refreshDisplay() const override;

    /* Utility functions */
    virtual void connect();
    virtual void close();

//    virtual void handleTimer(cMessage *msg);

    /* TcpSocket::ICallback callback methods */
    virtual void socketDataArrived(TcpSocket *socket, Packet *msg, bool urgent)
            override;
    virtual void socketAvailable(TcpSocket *socket,
            TcpAvailableInfo *availableInfo) override {
        socket->accept(availableInfo->getNewSocketId());
    }
    virtual void socketEstablished(TcpSocket *socket) override;
    virtual void socketPeerClosed(TcpSocket *socket) override;
    virtual void socketClosed(TcpSocket *socket) override;
    virtual void socketFailure(TcpSocket *socket, int code) override;
    virtual void socketStatusArrived(TcpSocket *socket, TcpStatusInfo *status)
            override {
    }
    virtual void socketDeleted(TcpSocket *socket) override {
    }

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;
};

} /* namespace allreduce */

#endif /* RINGALLREDUCER_H_ */
