/*
 * =====================================================================================
 *
 *       Filename:  topNetEthWifi.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  17/10/2017 19:55:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Francisco Helder (FHC), helderhdw@gmail.com
 *   Organization:  UFC-Quixadá
 *
 * =====================================================================================
 */


#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include <iostream>
#include <string>


// Default Network Topology
//
// Number of wifi or csma nodes can be increased up to 250
//                          |
//                 Rank 0   |   Rank 1
// -------------------------|----------------------------
//	  
//    Wifi 10.4.3.0 
// 	|---------------|  	
//	  *	   
//   n6            AP
//         *       *
//        n5       |    10.4.1.0
//   *             n0 -------------- n1   n2   n3   n4
//  n7               point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.4.2.0
//                                     

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("TopNetEthWifiExample");

void CourseChange (std::string context, Ptr<const MobilityModel> model){
		Vector position = model->GetPosition ();
  	NS_LOG_UNCOND (context <<" x = " << position.x << ", y = " << position.y);
}

int main (int argc, char *argv[]){
		bool verbose = true;
		uint32_t nCsma = 3;
  	uint32_t nWifi = 3;
  	bool tracing = false;
		bool showip = false;
		std::string rho="5.0";
		uint32_t payloadLength = 256;
  	uint32_t  payloadSize = 100;
  //	double datarate = 2.4;
    uint32_t AppStartTime = 0;
		Address serverAddress;
		bool OutputPosition = true;
		double simulationTime = 60;
  	ApplicationContainer serverApp;
		string TrafficInterval="0.1";	




		CommandLine cmd;
  	cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  	cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  	cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  	cmd.AddValue ("showip", "Show IPs address", showip);


  	cmd.Parse (argc,argv);

  	// Check for valid number of csma or wifi nodes
  	// 250 should be enough, otherwise IP addresses 
  	// soon become an issue
  	if (nWifi > 250 || nCsma > 250){
      	std::cout << "Too many wifi or csma nodes, no more than 250 each." << std::endl;
      	return(1);
    }

  	if (verbose){
      	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

		NS_LOG_INFO ("Create nodes.");
  	NodeContainer p2pNodes; 
  	p2pNodes.Create (2);

		NS_LOG_INFO ("Create channels.");  	
  	PointToPointHelper pointToPoint;
  	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ns"));

  	NetDeviceContainer p2pDevices;
  	p2pDevices = pointToPoint.Install (p2pNodes);

  	NodeContainer csmaNodes;
  	csmaNodes.Add (p2pNodes.Get (1));
  	csmaNodes.Create (nCsma);

  	CsmaHelper csma;
  	csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  	NetDeviceContainer csmaDevices;
  	csmaDevices = csma.Install (csmaNodes);

  	NodeContainer wifiStaNodes;
  	wifiStaNodes.Create (nWifi);
  	NodeContainer wifiApNode = p2pNodes.Get (0);

  	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  	phy.SetChannel (channel.Create ());

  	WifiHelper wifi;
  	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  	WifiMacHelper mac;
  	Ssid ssid = Ssid ("ns-3-ssid");
  	mac.SetType ("ns3::StaWifiMac", 
					"Ssid", SsidValue (ssid),
    				"ActiveProbing", BooleanValue (false));

  	NetDeviceContainer staDevices;
  	staDevices = wifi.Install (phy, mac, wifiStaNodes);

  	mac.SetType ("ns3::ApWifiMac",
    				"Ssid", SsidValue (ssid));

  	NetDeviceContainer apDevices;
  	apDevices = wifi.Install (phy, mac, wifiApNode);


  	// mobility.
   	MobilityHelper mobility;
  	mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                       "X", StringValue ("10.0"),
                                       "Y", StringValue ("10.0"),
                                       "rho", StringValue (rho));
  	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  	mobility.Install(wifiStaNodes);

  	MobilityHelper mobilityAp;
  	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  	positionAlloc->Add (Vector (20.0, 20.0, 0.0));
  	mobilityAp.SetPositionAllocator (positionAlloc);
  	mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  	mobilityAp.Install(wifiApNode);

  //	AnimationInterface::SetConstantPosition (p2pNodes.Get (1), 10, 30); 
  //	AnimationInterface::SetConstantPosition (csmaNodes.Get (1), 10, 33); 


	
		NS_LOG_INFO ("Create IPv4 Internet Stack");
  	InternetStackHelper stack;
  	stack.Install (csmaNodes);
  	stack.Install (wifiApNode);
  	stack.Install (wifiStaNodes);

		NS_LOG_INFO ("Create IPv4 Address"); 
 		Ipv4AddressHelper ipv4Address;

  	ipv4Address.SetBase ("10.4.1.0", "255.255.255.0");
  	Ipv4InterfaceContainer p2pInterfaces;
  	p2pInterfaces = ipv4Address.Assign (p2pDevices);

  	ipv4Address.SetBase ("10.4.2.0", "255.255.255.0");
  	Ipv4InterfaceContainer csmaInterfaces;
  	csmaInterfaces = ipv4Address.Assign (csmaDevices);
    serverAddress = csmaInterfaces.GetAddress(nCsma);

  	ipv4Address.SetBase ("10.4.3.0", "255.255.255.0");
   	Ipv4InterfaceContainer wifiInterfaces, wifiApInterface;
  	wifiApInterface = ipv4Address.Assign (apDevices);
  	wifiInterfaces = ipv4Address.Assign (staDevices);

		if (showip == true){
				cout<<endl;
				cout << "--------------------------" << endl;
				cout << "     Ethernet Address     " << endl;
				cout << "--------------------------" << endl;
				for(uint32_t i=0;i<=nCsma;i++)
					if(i!=nCsma)
						cout << "eNode_" << csmaNodes.Get (i)->GetId () << " addressIP: " << csmaInterfaces.GetAddress(i) << " | ";
					else	
						cout << "eNode_" << csmaNodes.Get (i)->GetId () << " addressIP: " << csmaInterfaces.GetAddress(i);

				cout << endl;
				cout << endl;
				cout << endl;
	
				cout << "--------------------------" << endl;
				cout << "       Wifi Address       " << endl;
				cout << "--------------------------" << endl;
				cout << "ApNote_" << wifiApNode.Get (0)->GetId () << " addressIP: " << wifiApInterface.GetAddress(0);
				
				for(uint32_t i=0;i<nWifi;i++)
					cout << " | wNode_" << wifiStaNodes.Get (i)->GetId () << " addressIP: " << wifiInterfaces.GetAddress(i);
				cout << endl;
				cout << endl;
				cout << endl;
    }
 

		//Application start time
    double randomInterval =stod (TrafficInterval,nullptr);
    Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable> ();
    
		//UDP flow
    UdpServerHelper myServer (9);
		serverApp = myServer.Install (csmaNodes.Get(nCsma));
    serverApp.Start (Seconds (0));
    serverApp.Stop (Seconds (simulationTime+1)); // serverApp stops when simulator stop
    //Set traffic start time for each station
    
    UdpClientHelper myClient (csmaInterfaces.GetAddress (nCsma), 9); //address of remote node
    myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
    myClient.SetAttribute ("Interval", TimeValue (Time (TrafficInterval))); //packets/s
    myClient.SetAttribute ("PacketSize", UintegerValue (payloadLength));
    
		for (uint16_t i = 0; i < nWifi; i++){
        double randomStart = m_rv->GetValue (0, randomInterval);
        cout << "Generate traffic in " << randomStart << "seconds, station " << double(i) << endl;
        
        ApplicationContainer clientApp = myClient.Install (wifiStaNodes.Get(i));
        clientApp.Start (Seconds (1 + randomStart));
        clientApp.Stop (Seconds (simulationTime+1));
    }
    
		AppStartTime=Simulator::Now ().GetSeconds () + 1; //clientApp start time
    //Simulator::Stop (Seconds (simulationTime+1)); //set stop time until no packet in queue



/*  
	NS_LOG_INFO ("Create UDP Application"); 
  	UdpEchoServerHelper echoServer (9);

  	ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  	serverApps.Start (Seconds (1.0));
  	serverApps.Stop (Seconds (10.0));

  	UdpEchoClientHelper echoClient (serverAddress, 9);
  	echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  	ApplicationContainer clientApps_1 = echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  	clientApps_1.Start (Seconds (2.0));
  	clientApps_1.Stop (Seconds (60.0));
	
	ApplicationContainer clientApps_2 = echoClient.Install (wifiStaNodes.Get (nWifi - 2));
  	clientApps_2.Start (Seconds (3.0));
  	clientApps_2.Stop (Seconds (20.0));

	ApplicationContainer clientApps_3 = echoClient.Install (wifiStaNodes.Get (nWifi - 3));
  	clientApps_3.Start (Seconds (4.0));
  	clientApps_3.Stop (Seconds (30.0));

  */

  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  	Simulator::Stop (Seconds (simulationTime + 1));

  	if  (OutputPosition){
    	uint32_t i =0;
      while (i < nWifi){
      	Ptr<MobilityModel> mobility = wifiStaNodes.Get (i)->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();
        cout << "Sta node#" << i << ", " << "position = " << position << endl;
        i++;
			}
          
			Ptr<MobilityModel> mobility1 = wifiApNode.Get (0)->GetObject<MobilityModel>();
      Vector position = mobility1->GetPosition();
      cout << "AP node, position = " << position << endl;
    }
  	
   /*  if (tracing == true){
      	pointToPoint.EnablePcapAll ("topologyIoT");
      	phy.EnablePcap ("topologyIoT", apDevices.Get (0));
      	csma.EnablePcap ("topologyIoT", csmaDevices.Get (0), true);
	
		std::cout << "eNode_" << csmaNodes.Get (nCsma)->GetId () << std::endl;
		std::cout << "wNode_" << wifiStaNodes.Get (nWifi-1)->GetId () << std::endl;
	
		std::ostringstream oss;

		oss << "/NodeList/" << wifiStaNodes.Get (nWifi - 1)->GetId () << "/$ns3::MobilityModel/CourseChange";
  	*/	//oss << "/NodeList/*/$ns3::MobilityModel/CourseChange";
  	/*  *	Config::Connect (oss.str (), MakeCallback (&CourseChange));
    }*/

	
    double throughput = 0;
    //UDP
	  uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
    throughput = totalPacketsThrough * payloadSize * 8 / ((simulationTime - AppStartTime) * 1000000.0);
	
		cout << "Nr total of Packets " << totalPacketsThrough << endl;
  	
    int totalPacketsLost = DynamicCast<UdpServer> (serverApp.Get(0))->GetLost();
	
  	cout << "Nr of packets received " << totalPacketsThrough << endl;
  	cout << "Nr of packets lost " << totalPacketsLost << endl;
	
		//float percPacketsLoss = totalPacketsLost/totalPacketsThrough;
		cout << "time Sumlation " << simulationTime << endl;
    cout << "Simulator start at " << AppStartTime << " s, end at " << simulationTime << " s"  << endl;
    
    cout << "Throughput" << endl;
    cout << throughput << " Mbit/s" << endl;

  	/*AnimationInterface anim ("TopWireless-animation.xml"); // Mandatory
  	for (uint32_t i = 0; i < wifiStaNodes.GetN (); ++i){
      anim.UpdateNodeDescription (wifiStaNodes.Get (i), "STA"); // Optional
      anim.UpdateNodeColor (wifiStaNodes.Get (i), 255, 0, 0); // Optional
    }
  	for (uint32_t i = 0; i < wifiApNode.GetN (); ++i){
      anim.UpdateNodeDescription (wifiApNode.Get (i), "AP"); // Optional
      anim.UpdateNodeColor (wifiApNode.Get (i), 0, 255, 0); // Optional
    }
  	for (uint32_t i = 0; i < csmaNodes.GetN (); ++i){
      anim.UpdateNodeDescription (csmaNodes.Get (i), "CSMA"); // Optional
      anim.UpdateNodeColor (csmaNodes.Get (i), 0, 0, 255); // Optional 
    }*/

  	//anim.EnablePacketMetadata (); // Optional
  	//anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  	//anim.EnableWifiMacCounters (Seconds (0), Seconds (10)); //Optional
  	//anim.EnableWifiPhyCounters (Seconds (0), Seconds (10)); //Optional

		Simulator::Run ();
		Simulator::Destroy ();
 
		return(0);
}

