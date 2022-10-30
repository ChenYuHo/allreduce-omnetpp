//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package allreduce.simulations;

import allreduce.AllReduceCoordinator;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth10M;
import inet.node.ethernet.EthernetSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

network SingleSwitch
{
    parameters:
        @statistic[allReduceTime](record=max,stats,vector); // from all workers
        @statistic[ATE_s](record=min,stats,vector); // from all workers
        int nWorkers = default(8);
    types:
        channelinterface IMyChannel
        {
        }
        channel Ideal extends ned.IdealChannel like IMyChannel
        {
        }
        channel Delay extends ned.DelayChannel like IMyChannel
        {
        }
        channel Datarate extends ned.DatarateChannel like IMyChannel
        {
        }
    submodules:
        hosts[nWorkers]: StandardHost;
        switch: EthernetSwitch;
        configurator: Ipv4NetworkConfigurator;
        coordinator: AllReduceCoordinator;
    connections:
        for i=0..nWorkers-1 {
            switch.ethg++ <--> switch_host_channel: <default("Datarate")> like IMyChannel <--> hosts[i].ethg++;
        }
}

network TwoLayers
{
    parameters:
        @statistic[allReduceTime](record=max,stats,vector); // from all workers
        @statistic[ATE_s](record=min,stats,vector); // from all workers
        int nWorkers = default(8);
        int nToRs = default(2);
    types:
        channelinterface IMyChannel
        {
        }
        channel Ideal extends ned.IdealChannel like IMyChannel
        {
        }
        channel Delay extends ned.DelayChannel like IMyChannel
        {
        }
        channel Datarate extends ned.DatarateChannel like IMyChannel
        {
        }
    submodules:
        hosts[nWorkers]: StandardHost;
        tors[nToRs]: EthernetSwitch;
        core: EthernetSwitch;
        configurator: Ipv4NetworkConfigurator;
        coordinator: AllReduceCoordinator;
    connections:
        for i=0..nWorkers-1 {
            tors[int(i/(nWorkers/nToRs))].ethg++ <--> tor_host_channel: <default("Datarate")> like IMyChannel <--> hosts[i].ethg++;
        }
        for i=0..nToRs-1 {
            tors[i].ethg++ <--> tor_core_channel: <default("Datarate")> like IMyChannel <--> core.ethg++;
        }
}


network ConfiguratorB
{
    @display("bgb=3225.915,1993.86");
    submodules:
        area1lan1host0: StandardHost {
            @display("p=492.195,413.82");
        }
        area1lan1host1: StandardHost {
            @display("p=492.195,147.345");
        }
        area1lan1host2: StandardHost {
            @display("p=865.26,147.345");
        }
        area1switch1: EthernetSwitch {
            @display("p=865.26,410.685");
        }
        area1lan2host1: StandardHost {
            @display("p=410.685,1097.25");
        }
        area1lan2host2: StandardHost {
            @display("p=410.685,1322.97");
        }
        area1lan2host3: StandardHost {
            @display("p=410.685,1583.175");
        }
        area1switch2: EthernetSwitch {
            @display("p=865.26,1094.115");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=150,100");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=150,250");
        }

        area1coreswitch: EthernetSwitch {
            @display("p=1232.055,755.53503");
        }
    connections:
        area1switch2.ethg++ <--> Eth10M <--> area1lan2host1.ethg++;
        area1lan2host2.ethg++ <--> Eth10M <--> area1switch2.ethg++;
        area1switch2.ethg++ <--> Eth10M <--> area1lan2host3.ethg++;
        area1switch1.ethg++ <--> Eth10M <--> area1lan1host0.ethg++;
        area1switch1.ethg++ <--> Eth10M <--> area1lan1host1.ethg++;
        area1switch1.ethg++ <--> Eth10M <--> area1lan1host2.ethg++;
        area1switch1.ethg++ <--> Eth10M <--> area1coreswitch.ethg++;
        area1switch2.ethg++ <--> Eth10M <--> area1coreswitch.ethg++;
}
