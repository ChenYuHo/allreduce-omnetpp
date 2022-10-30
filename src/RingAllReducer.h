#ifndef RINGALLREDUCER_H_
#define RINGALLREDUCER_H_

#include <omnetpp.h>
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/common/packet/ChunkQueue.h"

using namespace omnetpp;
using namespace inet;

namespace allreduce {

class RingAllReducer: public ApplicationBase, public TcpSocket::ICallback {
protected:
    TcpSocket send_socket;
    TcpSocket recv_socket;
    int n_transmissions_needed;
    int n_transmissions_sent = 0;
    simsignal_t allReduceTime;
    simsignal_t ATE_s;
    simtime_t allReduceStart;
    size_t segment_size;
    std::vector<size_t> segment_sizes;
    int n_workers;
    int rank;
    int n_transmissions_received = 0;
    int tensor_size;

    // statistics
    int packetsSent;
    int packetsRcvd;
    int bytesSent;
    int bytesRcvd;

    // statistics:
    static simsignal_t connectSignal;
    ChunkQueue queue;

protected:
    virtual void allreduce();
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
    virtual void sendSegment();

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
public:
};

}
/* namespace allreduce */

#endif /* RINGALLREDUCER_H_ */
