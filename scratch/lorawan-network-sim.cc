/*
 * =====================================================================================
 *
 *       Filename:  lorawan-network-sim.cc
 *
 *    Description:  This code simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this code is the throughput, loss packet, and delay 
 * of the network.
 *
 *        Version:  1.0
 *        Created:  24/08/2018 10:16:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Francisco Helder Candido (FHC), helderhdw@gmail.com
 *   Organization:  Unicamp/FEEC 
 *
 * =====================================================================================
 */

#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/forwarder-helper.h"
#include "ns3/network-server-helper.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include <algorithm>
#include <ctime>

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("ComplexLorawanNetworkSimulator");

// Network settings
int nDevices = 2000;
int gatewayRings = 1;
int nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
double radius = 7500;
double gatewayRadius = 7500/((gatewayRings-1)*2+1);
double simulationTime = 600;
int appPeriodSeconds = 600;
vector<int> sfQuantity (6);

int noMoreReceivers = 0;
int interfered = 0;
int underSensitivity = 0;
int received = 0;
int sent = 0;

// Output control
bool printEDs = true;
bool buildingsEnabled = false;

/**********************
 *  Global Callbacks  *
 **********************/

enum PacketOutcome {
  RECEIVED,
  INTERFERED,
  NO_MORE_RECEIVERS,
  UNDER_SENSITIVITY,
  UNSET
};

struct PacketStatus {
  Ptr<Packet const> packet;
  uint32_t senderId;
  int outcomeNumber;
  vector<enum PacketOutcome> outcomes;
};

std::map<Ptr<Packet const>, PacketStatus> packetTracker;

void CheckReceptionByAllGWsComplete (std::map<Ptr<Packet const>, PacketStatus>::iterator it){
  // Check whether this packet is received by all gateways
  if ((*it).second.outcomeNumber == nGateways){
			//cout << "receved all gw" << endl;
      // Update the statistics
      PacketStatus status = (*it).second;
      for (int j = 0; j < nGateways; j++){
          switch (status.outcomes.at (j)){
            case RECEIVED:
                received += 1;
            		break;
            case UNDER_SENSITIVITY:
                underSensitivity += 1;
            		break;
            case NO_MORE_RECEIVERS:
                noMoreReceivers += 1;
                break;
            case INTERFERED:
                interfered += 1;
                break;
            case UNSET:
                break;
						default:
								break;
            }
        }
      // Remove the packet from the tracker
      packetTracker.erase (it);
    }
}

void TransmissionCallback (Ptr<Packet const> packet, uint32_t systemId){
  NS_LOG_INFO ("Transmitted a packet from device " << systemId);
  // Create a packetStatus
  PacketStatus status;
  status.packet = packet;
  status.senderId = systemId;
  status.outcomeNumber = 0;
  status.outcomes = std::vector<enum PacketOutcome> (nGateways, UNSET);
 	sent += 1;
 
	packetTracker.insert (std::pair<Ptr<Packet const>, PacketStatus> (packet, status));
}

void PacketReceptionCallback (Ptr<Packet const> packet, uint32_t systemId){
  // Remove the successfully received packet from the list of sent ones
  NS_LOG_INFO ("A packet was successfully received at gateway " << systemId);

  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nDevices) = RECEIVED;
  (*it).second.outcomeNumber += 1;

  CheckReceptionByAllGWsComplete (it);
}

void InterferenceCallback (Ptr<Packet const> packet, uint32_t systemId){
  NS_LOG_INFO ("A packet was lost because of interference at gateway " << systemId);

  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nDevices) = INTERFERED;
  (*it).second.outcomeNumber += 1;

  CheckReceptionByAllGWsComplete (it);
}

void NoMoreReceiversCallback (Ptr<Packet const> packet, uint32_t systemId){
  NS_LOG_INFO ("A packet was lost because there were no more receivers at gateway " << systemId);

  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nDevices) = NO_MORE_RECEIVERS;
  (*it).second.outcomeNumber += 1;

  CheckReceptionByAllGWsComplete (it);
}

void UnderSensitivityCallback (Ptr<Packet const> packet, uint32_t systemId){
  NS_LOG_INFO ("A packet arrived at the gateway under sensitivity at gateway " << systemId);

  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nDevices) = UNDER_SENSITIVITY;
  (*it).second.outcomeNumber += 1;

  CheckReceptionByAllGWsComplete (it);
}

time_t oldtime = time (0);

// Periodically print simulation time
void PrintSimulationTime (void){
  NS_LOG_INFO ("Time: " << Simulator::Now().GetHours());
  cout << "Simulated time: " << Simulator::Now ().GetHours () << " hours" << endl;
  cout << "Real time from last call: " << std::time (0) - oldtime << " seconds" << endl;
  oldtime = time (0);
  Simulator::Schedule (Minutes (30), &PrintSimulationTime);
}

void
PrintEndDevices (NodeContainer endDevices, NodeContainer gateways, std::string filename){
  const char * c = filename.c_str ();
  std::ofstream spreadingFactorFile;
  spreadingFactorFile.open (c);
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j){
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      int sf = int(mac->GetSfFromDataRate(mac->GetDataRate ()));
      Vector pos = position->GetPosition ();
      spreadingFactorFile << pos.x << " " << pos.y << " " << sf << endl;
  }
  // Also print the gateways
  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j){
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      Vector pos = position->GetPosition ();
      spreadingFactorFile << pos.x << " " << pos.y << " GW" << endl;
  }
  spreadingFactorFile.close ();
}

int main (int argc, char *argv[]){

  string fileMetric="./scratch/mac-sta-inf.txt";
  string fileData="./scratch/mac-sta-dat.dat";
 	string pcapfile="./scratch/mac-s1g-slots";
	string endDevFile="./TestResult/test";
	int trial=1;
	double angle=0, sAngle=0;

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("gatewayRings", "Number of gateway rings to include", gatewayRings);
  cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  cmd.AddValue ("gatewayRadius", "The distance between two gateways", gatewayRadius);
  cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds);
  cmd.AddValue ("printEDs", "Whether or not to print a file containing the ED's positions", printEDs);
  cmd.AddValue ("file1", "files containing result information", fileData);
  cmd.AddValue ("file2", "files containing result data", fileMetric);
  cmd.AddValue ("trial", "files containing result data", trial);
  cmd.Parse (argc, argv);
	
	endDevFile += to_string(trial) + "/endDevices" + to_string(nDevices) + ".dat";

  // Set up logging
  //LogComponentEnable ("ComplexLorawanNetworkSimulator", LOG_LEVEL_ALL);
  //LogComponentEnable ("SimpleNetworkServer", LOG_LEVEL_ALL);
  //LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
  //LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
  //LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  //LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
  //LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
  //LogComponentEnable("LoraMac", LOG_LEVEL_ALL);
  //LogComponentEnable("EndDeviceLoraMac", LOG_LEVEL_ALL);
  //LogComponentEnable("GatewayLoraMac", LOG_LEVEL_ALL);
  //LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  //LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
  //LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
  //LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
  //LogComponentEnable("Forwarder", LOG_LEVEL_ALL);
	//LogComponentEnable("DeviceStatus", LOG_LEVEL_ALL);
	//LogComponentEnable("LoraMacHelper", LOG_LEVEL_ALL);
  //LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
  //LogComponentEnable("PeriodicSender", LOG_LEVEL_ALL);
  //LogComponentEnable("LoraMacHeader", LOG_LEVEL_ALL);
  //LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);


  /***********
  *  Setup  *
  ***********/

  // Compute the number of gateways
  nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
  //nGateways = gatewayRings;
  sAngle = (2*M_PI)/(nGateways-1);
  
	// Create the time value from the period
  Time appPeriod = Seconds (appPeriodSeconds);

  // Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "rho", DoubleValue (radius),
                                 "X", DoubleValue (10.0),
                                 "Y", DoubleValue (10.0));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  /************************
  *  Create the channel  *
  ************************/

  // Create the lora channel object
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent (3.76);
  loss->SetReference (1, 8.1);

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  /************************
  *  Create the helpers  *
  ************************/


  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LoraMacHelper
  LoraMacHelper macHelper = LoraMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();

  /************************
  *  Create End Devices  *
  ************************/

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (nDevices);

  // Assign a mobility model to each node
  mobility.Install (endDevices);

  // Create a LoraDeviceAddressGenerator
  uint8_t nwkId = 54;
  uint32_t nwkAddr = 1864;
  Ptr<LoraDeviceAddressGenerator> addrGen = CreateObject<LoraDeviceAddressGenerator> (nwkId,nwkAddr);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j){
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      position.z = 1.2;
      mobility->SetPosition (position);
    }

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  macHelper.SetAddressGenerator (addrGen);
  helper.Install (phyHelper, macHelper, endDevices);

  // Now end devices are connected to the channel

  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j){
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
			Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
      phy->TraceConnectWithoutContext ("StartSending",
                                       MakeCallback (&TransmissionCallback));
//			Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac>();
//			mac->SetMType (LoraMacHeader::CONFIRMED_DATA_UP);
    }

  /*********************
  *  Create Gateways  *
  *********************/

  // Create the gateway nodes (allocate them uniformely on the disc)
  NodeContainer gateways;
  gateways.Create (nGateways);

  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  allocator->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = gateways.Begin ()+1;
       j != gateways.End (); ++j){
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
			position.x = gatewayRadius * cos(angle); 
  		position.y = gatewayRadius * sin(angle); 
      position.z = 15;
      mobility->SetPosition (position);
			angle += sAngle;
    }

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
 	helper.Install (phyHelper, macHelper, gateways);
	
  /************************
  *  Configure Gateways  *
  ************************/
  NS_LOG_DEBUG ("Reception path");

  // Install reception paths on gateways
  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); j++){
      Ptr<Node> object = *j;
      // Get the device
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<GatewayLoraPhy> gwPhy = loraNetDevice->GetPhy ()->GetObject<GatewayLoraPhy> ();

      // Set up height of the gateway
      Ptr<MobilityModel> gwMob = (*j)->GetObject<MobilityModel> ();
      Vector position = gwMob->GetPosition ();
      position.z = 15;
      gwMob->SetPosition (position);

      // Global callbacks (every gateway)
      gwPhy->TraceConnectWithoutContext ("ReceivedPacket",
                                         MakeCallback (&PacketReceptionCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseInterference",
                                         MakeCallback (&InterferenceCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseNoMoreReceivers",
                                         MakeCallback (&NoMoreReceiversCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseUnderSensitivity",
                                         MakeCallback (&UnderSensitivityCallback));
			
			//gwPhy->AddReceptionPath (frequency1);
    }

  /**********************************************
  *  Set up the end device's spreading factor  *
  **********************************************/
  NS_LOG_DEBUG ("Spreading factor");

  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
  //macHelper.SetSpreadingFactorsUp (endDevices, gateways, trial);

	/************************
  * Create Network Server *
  *************************/
  NS_LOG_INFO ("Create Network Server...");

  NodeContainer networkServers;
  networkServers.Create (1);

  // Install the SimpleNetworkServer application on the network server
  NetworkServerHelper networkServer;
  networkServer.SetGateways (gateways);
  networkServer.SetEndDevices (endDevices);
  networkServer.Install (networkServers);

  // Install the Forwarder application on the gateways
  ForwarderHelper forwarderPacket;
  forwarderPacket.Install (gateways);

  NS_LOG_DEBUG ("Completed configuration");

  /*********************************************
  *  Install applications on the end devices  *
  *********************************************/

  Time appStopTime = Seconds(simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (appPeriodSeconds));
  ApplicationContainer appContainer = appHelper.Install (endDevices);
	
	uint32_t appStartTime = Simulator::Now().GetSeconds ();
  appContainer.Start (Seconds(appStartTime));
  appContainer.Stop (appStopTime);

  /**********************
   * Print output files *
   *********************/
  if (printEDs){
      PrintEndDevices (endDevices, gateways,
                       endDevFile);
			PrintSimulationTime ( );
    }

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (appStopTime + Seconds(10));

  // PrintSimulationTime ();
  Simulator::Run ();
  Simulator::Destroy ();

  /*************
  *  Results  *
  *************/
	double throughput = 0;
	uint32_t totalPacketsThrough = received;
  throughput = totalPacketsThrough * 19 * 8 / ((simulationTime - appStartTime) * 1000.0 * nGateways);

	double probSucc = (double(received)/sent)*100;
	//double lossProb = (double(interfered + noMoreReceivers + underSensitivity)/sent)*100/nGateways;
  double probLoss = (double(interfered + noMoreReceivers)/sent)*100;
  
 	ofstream myfile;
  myfile.open (fileMetric, ios::out | ios::app);
	myfile << nDevices << ", " << throughput << ", " << probSucc << ", " << probLoss << "\n";
  myfile.close();  
 

  myfile.open (fileData, ios::out | ios::app);
	myfile << "sent: " << sent << " rec: " << received << " interf: " << interfered << " noMoreRec: " << noMoreReceivers << " underSens: " << underSensitivity << "\n";
	myfile << ">>>>>>>>>>>>>>>>>>" << "\n";
	myfile << "numDev: " << nDevices << " numGat: " << nGateways << " simTime: " << simulationTime << " throughput: " << throughput<< "\n";
 	myfile << "####################################################" << "\n\n";
  myfile.close();  
 
	return(0);
}

