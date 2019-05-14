/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */
#include "ns3/radio-bearer-stats-calculator.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <string>
//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

int
main (int argc, char *argv[])
{

  uint16_t numberOfNodes = 1;
  double simTime = 50.2;     //Sec
  double distance = 60.0;
  double interPacketIntervaldl = 1 *1000;
  double interPacketIntervalul = 1*1000;   // 1Sseconds = 1000 Milliseconds
  bool useCa = false;
  uint16_t tc = 1;
  uint32_t packetsize = 64;
  uint8_t maxmcs = 9;

  int32_t t3324 = 50*1024;  // Milliseconds
  int64_t t3412 = 10*1024;  // Milliseconds
  int32_t ptw = 5*1024;  // Milliseconds
  uint16_t rrc_release_timer = 10*1000; // Milliseconds
  bool psm_enable = true;
  uint16_t EnableRandom = 7;


  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketIntervaldl", "DL Inter packet interval [ms])", interPacketIntervaldl);
  cmd.AddValue("interPacketIntervalul", "UL Inter packet interval [ms])", interPacketIntervalul);
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue("tc", "Test case number.", tc);
  cmd.AddValue("packetsize", "Packet size", packetsize);
  cmd.AddValue("t3324", " DRX timer", t3324); //<TODO: To connect maxmcs to lte-amc.cc>
  cmd.AddValue("t3412", "PSM timer", t3412);
  cmd.AddValue("ptw", "Paging window", ptw);
  cmd.AddValue("rrc_release_timer", "Connected timer", rrc_release_timer);
  cmd.AddValue("psm_enable", "PSM enabling flag", psm_enable);
  cmd.AddValue("maxmcs", "Maximum MCS", maxmcs);
  cmd.AddValue("EnableRandom","EnableRandom", EnableRandom);
  cmd.Parse(argc, argv);


  //Config::SetDefault ("ns3::UeManager::m_rrcreleaseinterval", UintegerValue(20));


  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));

   }

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 1; i < 1 + numberOfNodes; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");
  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes, t3324, t3412, ptw, rrc_release_timer, psm_enable);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes, t3324, t3412,ptw);



  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfNodes; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
        // side effect: the default EPS bearer will be activated
      }


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketIntervaldl)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      dlClient.SetAttribute ("PacketSize", UintegerValue(packetsize));
      dlClient.SetAttribute ("EnableRandom", UintegerValue (EnableRandom));
      dlClient.SetAttribute ("Percent", UintegerValue (100));
      dlClient.SetAttribute ("EnableDiagnostic", UintegerValue (0));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketIntervalul)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      ulClient.SetAttribute ("PacketSize", UintegerValue(packetsize));
      ulClient.SetAttribute ("EnableRandom", UintegerValue (EnableRandom));
      ulClient.SetAttribute ("Percent", UintegerValue (100));
      ulClient.SetAttribute ("EnableDiagnostic", UintegerValue (0));



      clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
  }

  std::string str;
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));
  //lteHelper->EnableTraces();
 
  lteHelper->EnableUlRxPhyTraces();
  Ptr<PhyRxStatsCalculator> p1 = lteHelper->GetUlRxPhyStats();
  str = "UlRxPhy_";
  str.append(std::to_string(tc));
  str.append(".txt");
  p1->SetAttribute("UlRxOutputFilename",StringValue( str));

  lteHelper->EnableUlTxPhyTraces();
  str = "UlTxPhy_";
  str.append(std::to_string(tc));
  str.append(".txt");
  Ptr<PhyTxStatsCalculator> p2 = lteHelper->GetUlTxPhyStats();
  p2->SetAttribute("UlTxOutputFilename",StringValue(str));

  lteHelper->EnableMacTraces();
  Ptr<MacStatsCalculator> m = lteHelper->GetMacStats();
  str = "UlMac_";
  str.append(std::to_string(tc));
  str.append(".txt");
  m->SetAttribute("UlOutputFilename",StringValue(str));
  str = "DlMac_";
  str.append(std::to_string(tc));
  str.append(".txt");
  m->SetAttribute("DlOutputFilename",StringValue(str));

  lteHelper->EnablePdcpTraces();
  Ptr<RadioBearerStatsCalculator> p = lteHelper->GetPdcpStats();  
  str = "UlPdcp_";
  str.append(std::to_string(tc));
  str.append(".txt");
  p->SetAttribute("UlPdcpOutputFilename",StringValue(str));
  str = "DlPdcp_";
  str.append(std::to_string(tc));
  str.append(".txt");
  p->SetAttribute("DlPdcpOutputFilename",StringValue(str));
  
  lteHelper->EnableRlcTraces();
  Ptr<RadioBearerStatsCalculator> r = lteHelper->GetRlcStats();
  str = "UlRlc_";
  str.append(std::to_string(tc));
  str.append(".txt");
  r->SetAttribute("UlRlcOutputFilename",StringValue(str));
  str = "DlRlc_";
  str.append(std::to_string(tc));
  str.append(".txt");
  r->SetAttribute("DlRlcOutputFilename",StringValue(str));
   




  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}

