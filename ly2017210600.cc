/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//
// This program configures a grid (default 5x5) of nodes on an
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to node 1.
//
// The default layout is like this, on a 2-D grid.
//
// n20  n21  n22  n23  n24
// n15  n16  n17  n18  n19
// n10  n11  n12  n13  n14
// n5   n6   n7   n8   n9
// n0   n1   n2   n3   n4
//
// the layout is affected by the parameters given to GridPositionAllocator;
// by default, GridWidth is 5 and numNodes is 25..
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc-grid --help"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the ns-3 documentation.
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when distance increases beyond
// the default of 500m.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --distance=500"
// ./waf --run "wifi-simple-adhoc --distance=1000"
// ./waf --run "wifi-simple-adhoc --distance=1500"
//
// The source node and sink node can be changed like this:
//
// ./waf --run "wifi-simple-adhoc --sourceNode=20 --sinkNode=10"
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc-grid --verbose=1"
//
// By default, trace file writing is off-- to enable it, try:
// ./waf --run "wifi-simple-adhoc-grid --tracing=1"
//
// When you are done tracing, you will notice many pcap trace files
// in your directory.  If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-grid-0-0.pcap -nn -tt
//
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/olsr-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/temp.h"
#include "ns3/energy-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

void TxCallback (Ptr<CounterCalculator<uint32_t> > datac,
                 std::string path, Ptr<const Packet> packet) {
  NS_LOG_INFO ("Sent frame counted in " <<
               datac->GetKey ());
  datac->Update ();
  // end TxCallback
}


int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double distance =1000;  // m
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 1;
  uint32_t numNodes = 100;  // by default, 5x5
  uint32_t sinkNode = 0;
  uint32_t sourceNode = 90;
  double interval = 1.0; // seconds
  bool verbose = false;
  bool tracing = false;
  //添加变量声明
  string format ("omnet");
  string experiment ("wifi-distance-test");//实验名称，按需命名
  string strategy ("wifi-default");//要检查的代码或参数，本例中固定
  string input;//是具体问题，本例中是两个节点之间的距离
  string runID;//本次实验唯一标识符，其信息在以后的分析中被标记，以>提供识别

  {
    stringstream sstr;
    sstr << "run-" << time (NULL);
    runID = sstr.str ();//本实验的标识符
  }


  CommandLine cmd;
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("numNodes", "number of nodes", numNodes);
  cmd.AddValue ("sinkNode", "Receiver node number", sinkNode);
  cmd.AddValue ("sourceNode", "Sender node number", sourceNode);
//New cmd.AddValue
  cmd.AddValue ("format", "Format to use for data output.",
                format);
  cmd.AddValue ("experiment", "Identifier for experiment.",
                experiment);
  cmd.AddValue ("strategy", "Identifier for strategy.",
                strategy);
  cmd.AddValue ("run", "Identifier for run.",
                runID);

  cmd.Parse (argc, argv);

   #ifndef STATS_HAS_SQLITE3
  if (format == "db") {
      NS_LOG_ERROR ("sqlite support not compiled in.");
      return -1;
    }
  #endif

  {
    stringstream sstr ("");
    sstr << distance;//本实验具体问题是两个节点之间的距离
    input = sstr.str ();
  }


  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  //------------------------------------------------------------
  //-- Create nodes and network stacks
  //--------------------------------------------

  NodeContainer c;
  c.Create (numNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10.0) );
  wifiPhy.Set ("TxGain", DoubleValue (0.0) );
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-62.0)); 

  wifiPhy.Set ("TxPowerStart", DoubleValue(16.0206));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue(0.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(16.0206));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-101.0));

  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
 // wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
//		  "ReferenceDistance",DoubleValue(100.0),
//		  "ReferenceLoss",DoubleValue(-86.6779));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add an upper mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (distance),
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (10),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
 
 // mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
//		  "Bounds",RectangleValue(Rectangle(0.0,9000.0,0.0,9000)),
//		  "Speed",StringValue("ns3::UniformRandomVariable[Min=100|Max=200]")
//		  );
  mobility.Install (c);

   /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (30));//初始电量
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install (c);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model 无线发射电流
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (devices, sources);


  //********************OLSR协议****************************
  // Enable OLSR
  
  OlsrHelper olsr;//Creating a Olsr
  Ipv4StaticRoutingHelper staticRouting;//Create a StaticRouting

  Ipv4ListRoutingHelper list;//Create a routinglist
  list.Add (staticRouting, 0);//set the staticrouting priority to 0
  list.Add (olsr, 10);//set the olsr priority to 10

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  //internet.SetRoutingHelper;//another method to set the "olsr" routing
  internet.Install (c);
  
 
 /* 
  AodvHelper aodv;
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (aodv, 10);
  InternetStackHelper internet;
  internet.SetRoutingHelper (list);
  internet.Install (c);
*/
  

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

 // TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
 // Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (sinkNode), tid);
 // InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
 // recvSink->Bind (local);
 // recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

 // Ptr<Socket> source = Socket::CreateSocket (c.Get (sourceNode), tid);
 // InetSocketAddress remote = InetSocketAddress (i.GetAddress (sinkNode, 0), 80);
 // source->Connect (remote);

  if (tracing == true)
    {
      AsciiTraceHelper ascii;
      wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
      wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
      // Trace routing tables
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
    //  aodv.PrintRoutingTableAllEvery (Seconds (2), routingStream);
     olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.neighbors", std::ios::out);
    //  aodv.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);
     olsr.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

      // To do-- enable an IP-level trace that shows forwarding events only
    }

  // Give OLSR time to converge-- 30 seconds perhaps
//  Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                     source, packetSize, numPackets, interPacketInterval);

  // Output what we are doing
 // NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  //------------------------------------------------------------
  //-- Create a custom traffic source and sink
  //-------------------------------------------
  //0-90-
  NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource0 = NodeList::GetNode (0);
  Ptr<Sender> sender0 = CreateObject<Sender>();//发送器sender
  appSource0->AddApplication (sender0);
  sender0->SetStartTime (Seconds (1));

  Ptr<Node> appSink0 = NodeList::GetNode (90);
  Ptr<Receiver> receiver0 = CreateObject<Receiver>();//接收器receiver
  appSink0->AddApplication (receiver0);
  receiver0->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/0/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.91"));
  //Config::Set可以更改节点发送数据包的目的地，本例中默认设置为广播
  
  //1-91
  NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource = NodeList::GetNode (1);
  Ptr<Sender> sender = CreateObject<Sender>();//发送器sender
  appSource->AddApplication (sender);
  sender->SetStartTime (Seconds (3));

  Ptr<Node> appSink = NodeList::GetNode (91);
  Ptr<Receiver> receiver = CreateObject<Receiver>();//接收器receiver
  appSink->AddApplication (receiver);
  receiver->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/1/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.92"));
  //Config::Set可以更改节点发送数据包的目的地，本例中默认设置为广播


  //2-92
   NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource2 = NodeList::GetNode (2);
  Ptr<Sender> sender2 = CreateObject<Sender>();//发送器sender
  appSource2->AddApplication (sender2);
  sender2->SetStartTime (Seconds (5));

  Ptr<Node> appSink2 = NodeList::GetNode (92);
  Ptr<Receiver> receiver2 = CreateObject<Receiver>();//接收器receiver
  appSink2->AddApplication (receiver2);
  receiver2->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/2/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.93"));


  //3-93
 NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource3 = NodeList::GetNode (3);
  Ptr<Sender> sender3 = CreateObject<Sender>();//发送器sender
  appSource3->AddApplication (sender3);
  sender3->SetStartTime (Seconds (8));

  Ptr<Node> appSink3 = NodeList::GetNode (93);
  Ptr<Receiver> receiver3 = CreateObject<Receiver>();//接收器receiver
  appSink3->AddApplication (receiver3);
  receiver3->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/3/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.94"));

  //4-94
 NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource4 = NodeList::GetNode (4);
  Ptr<Sender> sender4 = CreateObject<Sender>();//发送器sender
  appSource4->AddApplication (sender4);
  sender4->SetStartTime (Seconds (11));

  Ptr<Node> appSink4 = NodeList::GetNode (94);
  Ptr<Receiver> receiver4 = CreateObject<Receiver>();//接收器receiver
  appSink4->AddApplication (receiver4);
  receiver4->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/4/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.95"));

  //5-95
 NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource5 = NodeList::GetNode (5);
  Ptr<Sender> sender5 = CreateObject<Sender>();//发送器sender
  appSource5->AddApplication (sender5);
  sender5->SetStartTime (Seconds (14));

  Ptr<Node> appSink5 = NodeList::GetNode (95);
  Ptr<Receiver> receiver5 = CreateObject<Receiver>();//接收器receiver
  appSink5->AddApplication (receiver5);
  receiver5->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/5/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.96"));

  //6-96
 NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource6 = NodeList::GetNode (6);
  Ptr<Sender> sender6 = CreateObject<Sender>();//发送器sender
  appSource6->AddApplication (sender6);
  sender6->SetStartTime (Seconds (17));

  Ptr<Node> appSink6 = NodeList::GetNode (96);
  Ptr<Receiver> receiver6 = CreateObject<Receiver>();//接收器receiver
  appSink6->AddApplication (receiver6);
  receiver6->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/6/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.97"));

  //7-97
 NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource7 = NodeList::GetNode (7);
  Ptr<Sender> sender7 = CreateObject<Sender>();//发送器sender
  appSource7->AddApplication (sender7);
  sender7->SetStartTime (Seconds (20));

  Ptr<Node> appSink7 = NodeList::GetNode (97);
  Ptr<Receiver> receiver7 = CreateObject<Receiver>();//接收器receiver
  appSink7->AddApplication (receiver7);
  receiver7->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/7/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.98"));

  //8-98
 NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource8 = NodeList::GetNode (8);
  Ptr<Sender> sender8 = CreateObject<Sender>();//发送器sender
  appSource8->AddApplication (sender8);
  sender8->SetStartTime (Seconds (23));

  Ptr<Node> appSink8 = NodeList::GetNode (98);
  Ptr<Receiver> receiver8 = CreateObject<Receiver>();//接收器receiver
  appSink8->AddApplication (receiver8);
  receiver8->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/8/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.99"));

  //9-99
 NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<Node> appSource9 = NodeList::GetNode (9);
  Ptr<Sender> sender9 = CreateObject<Sender>();//发送器sender
  appSource9->AddApplication (sender9);
  sender9->SetStartTime (Seconds (26));

  Ptr<Node> appSink9 = NodeList::GetNode (99);
  Ptr<Receiver> receiver9 = CreateObject<Receiver>();//接收器receiver
  appSink9->AddApplication (receiver9);
  receiver9->SetStartTime (Seconds (0));

  Config::Set ("/NodeList/9/ApplicationList/*/$Sender/Destination",
               Ipv4AddressValue ("10.1.1.100"));



  //------------------------------------------------------------
  //-- Setup stats and data collection
  //--------------------------------------------

  // Create a DataCollector object to hold information about this run.
  DataCollector data;
  data.DescribeRun (experiment,//实验名词
                    strategy,//策略
                    input,//输入（距离）
                    runID);//运行

  // Add any information we wish to record about this run.
  data.AddMetadata ("author", "2017210600-liyi");//


// Create a counter to track how many frames are generated.  Updates
  // are triggered by the trace signal generated by the WiFi MAC model
  // object.  Here we connect the counter to the signal via the simple
  // TxCallback() glue function defined above.
  Ptr<CounterCalculator<uint32_t> > totalTx0 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx0->SetKey ("wifi-tx-frames");
  totalTx0->SetContext ("node[0]");
  Config::Connect ("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx0));
  data.AddDataCalculator (totalTx0);


  Ptr<CounterCalculator<uint32_t> > totalTx =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx->SetKey ("wifi-tx-frames");
  totalTx->SetContext ("node[1]");
  Config::Connect ("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx));
  data.AddDataCalculator (totalTx);


  //2-92
Ptr<CounterCalculator<uint32_t> > totalTx2 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx2->SetKey ("wifi-tx-frames");
  totalTx2->SetContext ("node[2]");
  Config::Connect ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx2));
  data.AddDataCalculator (totalTx2);

  //3-93
Ptr<CounterCalculator<uint32_t> > totalTx3 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx3->SetKey ("wifi-tx-frames");
  totalTx3->SetContext ("node[3]");
  Config::Connect ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx3));
  data.AddDataCalculator (totalTx3);

  //4-94
Ptr<CounterCalculator<uint32_t> > totalTx4 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx4->SetKey ("wifi-tx-frames");
  totalTx4->SetContext ("node[4]");
  Config::Connect ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx4));
  data.AddDataCalculator (totalTx4);

  //5-95
Ptr<CounterCalculator<uint32_t> > totalTx5 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx5->SetKey ("wifi-tx-frames");
  totalTx5->SetContext ("node[5]");
  Config::Connect ("/NodeList/5/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx5));
  data.AddDataCalculator (totalTx5);

  //6-96
Ptr<CounterCalculator<uint32_t> > totalTx6 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx6->SetKey ("wifi-tx-frames");
  totalTx6->SetContext ("node[6]");
  Config::Connect ("/NodeList/6/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx6));
  data.AddDataCalculator (totalTx6);

  //7-97
Ptr<CounterCalculator<uint32_t> > totalTx7 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx7->SetKey ("wifi-tx-frames");
  totalTx7->SetContext ("node[7]");
  Config::Connect ("/NodeList/7/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx7));
  data.AddDataCalculator (totalTx7);

  //8-98
Ptr<CounterCalculator<uint32_t> > totalTx8 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx8->SetKey ("wifi-tx-frames");
  totalTx8->SetContext ("node[8]");
  Config::Connect ("/NodeList/8/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx8));
  data.AddDataCalculator (totalTx8);

  //9-99
Ptr<CounterCalculator<uint32_t> > totalTx9 =
    CreateObject<CounterCalculator<uint32_t> >();//计数器totalTx-发>送frames
  totalTx9->SetKey ("wifi-tx-frames");
  totalTx9->SetContext ("node[9]");
  Config::Connect ("/NodeList/9/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx",
                   MakeBoundCallback (&TxCallback, totalTx9));
  data.AddDataCalculator (totalTx9);


   // This is similar, but creates a counter to track how many frames
  // are received.  Instead of our own glue function, this uses a
  // method of an adapter class to connect a counter directly to the
  // trace signal generated by the WiFi MAC.
  Ptr<PacketCounterCalculator> totalRx0 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx0->SetKey ("wifi-rx-frames");
  totalRx0->SetContext ("node[90]");
  Config::Connect ("/NodeList/90/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx0));
  data.AddDataCalculator (totalRx0);


  Ptr<PacketCounterCalculator> totalRx =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx->SetKey ("wifi-rx-frames");
  totalRx->SetContext ("node[91]");
  Config::Connect ("/NodeList/91/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx));
  data.AddDataCalculator (totalRx);


  //2-92
Ptr<PacketCounterCalculator> totalRx2 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx2->SetKey ("wifi-rx-frames");
  totalRx2->SetContext ("node[92]");
  Config::Connect ("/NodeList/92/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx2));
  data.AddDataCalculator (totalRx2);

  //3-93
Ptr<PacketCounterCalculator> totalRx3 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx3->SetKey ("wifi-rx-frames");
  totalRx3->SetContext ("node[93]");
  Config::Connect ("/NodeList/93/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx3));
  data.AddDataCalculator (totalRx3);

  //4-94
Ptr<PacketCounterCalculator> totalRx4 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx4->SetKey ("wifi-rx-frames");
  totalRx4->SetContext ("node[94]");
  Config::Connect ("/NodeList/94/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx4));
  data.AddDataCalculator (totalRx4);

  //5-95
Ptr<PacketCounterCalculator> totalRx5 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx5->SetKey ("wifi-rx-frames");
  totalRx5->SetContext ("node[95]");
  Config::Connect ("/NodeList/95/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx5));
  data.AddDataCalculator (totalRx5);

  //6-96
Ptr<PacketCounterCalculator> totalRx6 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx6->SetKey ("wifi-rx-frames");
  totalRx6->SetContext ("node[96]");
  Config::Connect ("/NodeList/96/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx6));
  data.AddDataCalculator (totalRx6);

  //7-97
Ptr<PacketCounterCalculator> totalRx7 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx7->SetKey ("wifi-rx-frames");
  totalRx7->SetContext ("node[97]");
  Config::Connect ("/NodeList/97/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx7));
  data.AddDataCalculator (totalRx7);

  //8-98
Ptr<PacketCounterCalculator> totalRx8 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx8->SetKey ("wifi-rx-frames");
  totalRx8->SetContext ("node[98]");
  Config::Connect ("/NodeList/98/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx8));
  data.AddDataCalculator (totalRx8);

  //9-99
Ptr<PacketCounterCalculator> totalRx9 =
    CreateObject<PacketCounterCalculator>();//totalRx-接受frames
  totalRx9->SetKey ("wifi-rx-frames");
  totalRx9->SetContext ("node[99]");
  Config::Connect ("/NodeList/99/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 totalRx9));
  data.AddDataCalculator (totalRx9);


 // This counter tracks how many packets---as opposed to frames---are
  // generated.  This is connected directly to a trace signal provided
  // by our Sender class.
  Ptr<PacketCounterCalculator> appTx0 =
    CreateObject<PacketCounterCalculator>();
  appTx0->SetKey ("sender-tx-packets");
  appTx0->SetContext ("node[0]");
  Config::Connect ("/NodeList/0/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx0));
  data.AddDataCalculator (appTx0);

  Ptr<PacketCounterCalculator> appTx =
    CreateObject<PacketCounterCalculator>();
  appTx->SetKey ("sender-tx-packets");
  appTx->SetContext ("node[1]");
  Config::Connect ("/NodeList/1/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx));
  data.AddDataCalculator (appTx);

  //2-92
 Ptr<PacketCounterCalculator> appTx2 =
    CreateObject<PacketCounterCalculator>();
  appTx2->SetKey ("sender-tx-packets");
  appTx2->SetContext ("node[2]");
  Config::Connect ("/NodeList/2/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx2));
  data.AddDataCalculator (appTx2);

  //3-93
 Ptr<PacketCounterCalculator> appTx3 =
    CreateObject<PacketCounterCalculator>();
  appTx3->SetKey ("sender-tx-packets");
  appTx3->SetContext ("node[3]");
  Config::Connect ("/NodeList/3/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx3));
  data.AddDataCalculator (appTx3);

  //4-94
 Ptr<PacketCounterCalculator> appTx4 =
    CreateObject<PacketCounterCalculator>();
  appTx4->SetKey ("sender-tx-packets");
  appTx4->SetContext ("node[4]");
  Config::Connect ("/NodeList/4/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx4));
  data.AddDataCalculator (appTx4);

  //5-95
 Ptr<PacketCounterCalculator> appTx5 =
    CreateObject<PacketCounterCalculator>();
  appTx5->SetKey ("sender-tx-packets");
  appTx5->SetContext ("node[5]");
  Config::Connect ("/NodeList/5/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx5));
  data.AddDataCalculator (appTx5);

  //6-96
 Ptr<PacketCounterCalculator> appTx6 =
    CreateObject<PacketCounterCalculator>();
  appTx6->SetKey ("sender-tx-packets");
  appTx6->SetContext ("node[6]");
  Config::Connect ("/NodeList/6/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx6));
  data.AddDataCalculator (appTx6);

  //7-97
 Ptr<PacketCounterCalculator> appTx7 =
    CreateObject<PacketCounterCalculator>();
  appTx7->SetKey ("sender-tx-packets");
  appTx7->SetContext ("node[7]");
  Config::Connect ("/NodeList/7/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx7));
  data.AddDataCalculator (appTx7);

  //8-98
 Ptr<PacketCounterCalculator> appTx8 =
    CreateObject<PacketCounterCalculator>();
  appTx8->SetKey ("sender-tx-packets");
  appTx8->SetContext ("node[8]");
  Config::Connect ("/NodeList/8/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx8));
  data.AddDataCalculator (appTx8);

  //9-99
 Ptr<PacketCounterCalculator> appTx9 =
    CreateObject<PacketCounterCalculator>();
  appTx9->SetKey ("sender-tx-packets");
  appTx9->SetContext ("node[9]");
  Config::Connect ("/NodeList/9/ApplicationList/*/$Sender/Tx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate,
                                 appTx9));
  data.AddDataCalculator (appTx9);


  // Here a counter for received packets is directly manipulated by
  // one of the custom objects in our simulation, the Receiver
  // Application.  The Receiver object is given a pointer to the
  // counter and calls its Update() method whenever a packet arrives.
  Ptr<CounterCalculator<> > appRx0 =
    CreateObject<CounterCalculator<> >();
  appRx0->SetKey ("receiver-rx-packets");
  appRx0->SetContext ("node[90]");
  receiver0->SetCounter (appRx0);//Receiver::SetCounter
  data.AddDataCalculator (appRx0);


  Ptr<CounterCalculator<> > appRx =
    CreateObject<CounterCalculator<> >();
  appRx->SetKey ("receiver-rx-packets");
  appRx->SetContext ("node[91]");
  receiver->SetCounter (appRx);//Receiver::SetCounter
  data.AddDataCalculator (appRx);



  //2-92
   Ptr<CounterCalculator<> > appRx2 =
    CreateObject<CounterCalculator<> >();
  appRx2->SetKey ("receiver-rx-packets");
  appRx2->SetContext ("node[92]");
  receiver2->SetCounter (appRx2);//Receiver::SetCounter
  data.AddDataCalculator (appRx2);


  //3-93
 Ptr<CounterCalculator<> > appRx3 =
    CreateObject<CounterCalculator<> >();
  appRx3->SetKey ("receiver-rx-packets");
  appRx3->SetContext ("node[93]");
  receiver3->SetCounter (appRx3);//Receiver::SetCounter
  data.AddDataCalculator (appRx3);

  //4-94
 Ptr<CounterCalculator<> > appRx4 =
    CreateObject<CounterCalculator<> >();
  appRx4->SetKey ("receiver-rx-packets");
  appRx4->SetContext ("node[94]");
  receiver4->SetCounter (appRx4);//Receiver::SetCounter
  data.AddDataCalculator (appRx4);

  //5-95
 Ptr<CounterCalculator<> > appRx5 =
    CreateObject<CounterCalculator<> >();
  appRx5->SetKey ("receiver-rx-packets");
  appRx5->SetContext ("node[95]");
  receiver5->SetCounter (appRx5);//Receiver::SetCounter
  data.AddDataCalculator (appRx5);

  //6-96
 Ptr<CounterCalculator<> > appRx6 =
    CreateObject<CounterCalculator<> >();
  appRx6->SetKey ("receiver-rx-packets");
  appRx6->SetContext ("node[96]");
  receiver6->SetCounter (appRx6);//Receiver::SetCounter
  data.AddDataCalculator (appRx6);

  //7-97
 Ptr<CounterCalculator<> > appRx7 =
    CreateObject<CounterCalculator<> >();
  appRx7->SetKey ("receiver-rx-packets");
  appRx7->SetContext ("node[97]");
  receiver7->SetCounter (appRx7);//Receiver::SetCounter
  data.AddDataCalculator (appRx7);

  //8-98
 Ptr<CounterCalculator<> > appRx8 =
    CreateObject<CounterCalculator<> >();
  appRx8->SetKey ("receiver-rx-packets");
  appRx8->SetContext ("node[98]");
  receiver8->SetCounter (appRx8);//Receiver::SetCounter
  data.AddDataCalculator (appRx8);

  //9-99
 Ptr<CounterCalculator<> > appRx9 =
    CreateObject<CounterCalculator<> >();
  appRx9->SetKey ("receiver-rx-packets");
  appRx9->SetContext ("node[99]");
  receiver9->SetCounter (appRx9);//Receiver::SetCounter
  data.AddDataCalculator (appRx9);


   // This DataCalculator connects directly to the transmit trace
  // provided by our Sender Application.  It records some basic
  // statistics about the sizes of the packets received (min, max,
  // avg, total # bytes), although in this scenaro they're fixed.
  Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts0 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts0->SetKey ("tx-pkt-size");
  appTxPkts0->SetContext ("node[0]");
  Config::Connect ("/NodeList/0/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts0));
  data.AddDataCalculator (appTxPkts0);

   Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts->SetKey ("tx-pkt-size");
  appTxPkts->SetContext ("node[1]");
  Config::Connect ("/NodeList/1/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts));
  data.AddDataCalculator (appTxPkts);



  //2-92
Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts2 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts2->SetKey ("tx-pkt-size");
  appTxPkts2->SetContext ("node[2]");
  Config::Connect ("/NodeList/2/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts2));
  data.AddDataCalculator (appTxPkts2);

  //3-93
Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts3 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts3->SetKey ("tx-pkt-size");
  appTxPkts3->SetContext ("node[3]");
  Config::Connect ("/NodeList/3/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts3));
  data.AddDataCalculator (appTxPkts3);

  //4-94
Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts4 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts4->SetKey ("tx-pkt-size");
  appTxPkts4->SetContext ("node[4]");
  Config::Connect ("/NodeList/4/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts4));
  data.AddDataCalculator (appTxPkts4);

  //5-95
Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts5 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts5->SetKey ("tx-pkt-size");
  appTxPkts5->SetContext ("node[5]");
  Config::Connect ("/NodeList/5/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts5));
  data.AddDataCalculator (appTxPkts5);

  //6-96
Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts6 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts6->SetKey ("tx-pkt-size");
  appTxPkts6->SetContext ("node[6]");
  Config::Connect ("/NodeList/6/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts6));
  data.AddDataCalculator (appTxPkts6);

  //7-97
Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts7 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts7->SetKey ("tx-pkt-size");
  appTxPkts7->SetContext ("node[7]");
  Config::Connect ("/NodeList/7/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts7));
  data.AddDataCalculator (appTxPkts7);

  //8-98
Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts8 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts8->SetKey ("tx-pkt-size");
  appTxPkts8->SetContext ("node[8]");
  Config::Connect ("/NodeList/8/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts8));
  data.AddDataCalculator (appTxPkts8);

  //9-99
Ptr<PacketSizeMinMaxAvgTotalCalculator> appTxPkts9 =
    CreateObject<PacketSizeMinMaxAvgTotalCalculator>();
  appTxPkts9->SetKey ("tx-pkt-size");
  appTxPkts9->SetContext ("node[9]");
  Config::Connect ("/NodeList/9/ApplicationList/*/$Sender/Tx",
                   MakeCallback
                     (&PacketSizeMinMaxAvgTotalCalculator::PacketUpdate,
                     appTxPkts9));
  data.AddDataCalculator (appTxPkts9);

 // Here we directly manipulate another DataCollector tracking min,
  // max, total, and average propagation delays.  Check out the Sender
  // and Receiver classes to see how packets are tagged with
  // timestamps to do this.
  Ptr<TimeMinMaxAvgTotalCalculator> delayStat0 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat0->SetKey ("delay0");
  delayStat0->SetContext (".");
  receiver0->SetDelayTracker (delayStat0);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat0);

  Ptr<TimeMinMaxAvgTotalCalculator> delayStat =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat->SetKey ("delay1");
  delayStat->SetContext (".");
  receiver->SetDelayTracker (delayStat);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat);

  //2
Ptr<TimeMinMaxAvgTotalCalculator> delayStat2 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat2->SetKey ("delay2");
  delayStat2->SetContext (".");
  receiver2->SetDelayTracker (delayStat2);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat2);
  //3
Ptr<TimeMinMaxAvgTotalCalculator> delayStat3 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat3->SetKey ("delay3");
  delayStat3->SetContext (".");
  receiver3->SetDelayTracker (delayStat3);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat3);
  //4
Ptr<TimeMinMaxAvgTotalCalculator> delayStat4 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat4->SetKey ("delay4");
  delayStat4->SetContext (".");
  receiver4->SetDelayTracker (delayStat4);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat4);
  //5
Ptr<TimeMinMaxAvgTotalCalculator> delayStat5 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat5->SetKey ("delay5");
  delayStat5->SetContext (".");
  receiver5->SetDelayTracker (delayStat5);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat5);
  //6
Ptr<TimeMinMaxAvgTotalCalculator> delayStat6 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat6->SetKey ("delay6");
  delayStat6->SetContext (".");
  receiver6->SetDelayTracker (delayStat6);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat6);
  //7
Ptr<TimeMinMaxAvgTotalCalculator> delayStat7 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat7->SetKey ("delay7");
  delayStat7->SetContext (".");
  receiver7->SetDelayTracker (delayStat7);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat7);
  //8
Ptr<TimeMinMaxAvgTotalCalculator> delayStat8 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat8->SetKey ("delay8");
  delayStat8->SetContext (".");
  receiver8->SetDelayTracker (delayStat8);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat8);
  //9
 Ptr<TimeMinMaxAvgTotalCalculator> delayStat9 =
    CreateObject<TimeMinMaxAvgTotalCalculator>();
  delayStat9->SetKey ("delay9");
  delayStat9->SetContext (".");
  receiver9->SetDelayTracker (delayStat9);//Receiver::SetDelayTracker
  data.AddDataCalculator (delayStat9);





  AnimationInterface anim("ly4-3289.xml");
  anim.SetMaxPktsPerTraceFile(99999999999999);
  Simulator::Stop (Seconds (33.0));
  Simulator::Run ();

    //------------------------------------------------------------
  //-- Generate statistics output.
  //--------------------------------------------

  // Pick an output writer based in the requested format.
  Ptr<DataOutputInterface> output = 0;
  if (format == "omnet") {
      NS_LOG_INFO ("Creating omnet formatted data output.");
      output = CreateObject<OmnetDataOutput>();
    } else if (format == "db") {
    #ifdef STATS_HAS_SQLITE3
      NS_LOG_INFO ("Creating sqlite formatted data output.");
      output = CreateObject<SqliteDataOutput>();
    #endif
    } else {
      NS_LOG_ERROR ("Unknown output format " << format);
    }

  // Finally, have that writer interrogate the DataCollector and save
  // the results.
  if (output != 0)
    output->Output (data);

  ofstream fout("energy.txt");
//迭代器计算能耗数值
  for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin (); iter != deviceModels.End (); iter ++)
    {
      double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                     << "s) Total energy consumed by radio = " << energyConsumed << "J");
     fout<<energyConsumed<<endl;
      NS_ASSERT (energyConsumed <= 30);
    }
  fout.close();



  Simulator::Destroy ();

  return 0;
}

