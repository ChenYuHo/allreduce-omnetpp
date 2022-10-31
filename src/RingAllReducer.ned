package allreduce;

import inet.applications.contract.IApp;

simple AllReducer like IApp
{
    parameters:
        @signal[allReduceTime](type=simtime_t);
        @signal[ATE_s](type=double);
        
        int sendPortBase = default(1000);  // local port
        int recvPortBase = default(2000);
        int tensorSize @unit(B) = default(1MiB);
        int nWorkers = default(2);
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        @display("i=block/app");
        @lifecycleSupport;
    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
        inout controlPort;
}

simple AllReduceCoordinator
{
    parameters:
        int nWorkers = default(2);
}

simple Ring extends AllReducer
{
    parameters:
        @signal[allReduceTime](type=simtime_t);
        @signal[ATE_s](type=double);
        @class(Ring);
}

simple HalvingDoubling extends AllReducer
{
    parameters:
        @signal[allReduceTime](type=simtime_t);
        @signal[ATE_s](type=double);
        @class(HalvingDoubling);
}



simple RingAllReducer like IApp
{
    parameters:
        @signal[allReduceTime](type=simtime_t);
        @signal[ATE_s](type=double);
        
        int sendPort = default(1000);  // local port
        int recvPort = default(1001);
        int tensorSize @unit(B) = default(1MiB);
        int nWorkers = default(2);
        int timeToLive = default(-1); // if not -1, set the TTL (IPv4) or Hop Limit (IPv6) field of sent packets to this value
        int dscp = default(-1); // if not -1, set the DSCP (IPv4/IPv6) field of sent packets to this value
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        @display("i=block/app");
        @lifecycleSupport;
    gates:
        input socketIn @labels(TcpCommand/up);
        output socketOut @labels(TcpCommand/down);
}
