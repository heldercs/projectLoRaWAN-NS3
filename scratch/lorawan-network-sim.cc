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

#include <algorithm>
#include <ctime>
#include "ns3/lora-phy.h" 
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
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("LoRaWanNetworkSimulator");

// Network settings
int nDevices = 2000;
int gatewayRings = 1;
int nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
double radius = 7500;
double gatewayRadius = 7500/((gatewayRings-1)*2+1);
double simulationTime = 600;
int appPeriodSeconds = 600;
string fileDelay = "./scratch/delay.dat";

int noMoreReceivers = 0;
int interfered = 0;
int underSensitivity = 0;
int received = 0;
int sent = 0;
int packSucc = 0;
int cntDevices=0;
int cntDelay=0;

// sum Time on Air
Time sumToA=Seconds(0);
Time sumDelay=Seconds(0);

// Channel model
bool shadowingEnabled = true;
bool buildingsEnabled = true;

// Output control
bool printEDs = true;
bool printBuildings = true;
bool printDelay = true;
time_t oldtime = time (0);

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
	uint32_t packFlag;
	Time sndTime;
	Time rcvTime;
	Time duration; 
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
								if (status.packFlag){
									Time uDelay = (status.rcvTime - status.sndTime)-status.duration;
									//NS_LOG_INFO("ToA:" << status.duration);
									NS_LOG_INFO ("Delay for device " << uDelay);if(printDelay){
  											ofstream myfile;
  											myfile.open (fileDelay, ios::out | ios::app);
        									myfile << ", " << uDelay.GetNanoSeconds();
    										myfile.close();
										}
									if(cntDelay < nDevices){
										sumDelay += uDelay;
										NS_LOG_DEBUG("sumDely:" << sumDelay);
										cntDelay++;
										
									}
									packSucc += 1;
									status.packFlag = 0;
								}
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

void TransmissionCallback (Ptr<Packet> packet, LoraTxParameters txParams, uint32_t systemId){
	NS_LOG_INFO ("Transmitted a packet from device " << systemId);
	
  	// Create a packetStatus
	PacketStatus status;
	status.packet = packet;
	status.senderId = systemId;
	status.packFlag = 1;
	status.sndTime = Simulator::Now();
	status.rcvTime = Time::Max();
	status.outcomeNumber = 0;
	status.outcomes = std::vector<enum PacketOutcome> (nGateways, UNSET);
	status.duration = LoraPhy::GetOnAirTime (packet, txParams);	
	sent += 1;

	if(cntDevices < nDevices){	
 		sumToA += LoraPhy::GetOnAirTime (packet, txParams);
		NS_LOG_DEBUG("sumToA: " << sumToA.GetSeconds());
		cntDevices++;
  	}

  	packetTracker.insert (std::pair<Ptr<Packet const>, PacketStatus> (packet, status));
}

void PacketReceptionCallback (Ptr<Packet const> packet, uint32_t systemId){
	// Remove the successfully received packet from the list of sent ones
  	NS_LOG_INFO ("A packet was successfully received at gateway " << systemId);

  	std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  	(*it).second.outcomes.at (systemId - nDevices) = RECEIVED;
  	(*it).second.outcomeNumber += 1;
	(*it).second.rcvTime = Simulator::Now(); 

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

// Periodically print simulation time
void PrintSimulationTime (void){
  	NS_LOG_INFO ("Time: " << Simulator::Now().GetHours());
 	cout << "Simulated time: " << Simulator::Now ().GetSeconds () << " seconds" << endl;
  	cout << "Real time from last call: " << std::time (0) - oldtime << " seconds" << endl;
  	oldtime = time (0);
  	Simulator::Schedule (Minutes (30), &PrintSimulationTime);
}

void PrintEndDevices (NodeContainer endDevices, NodeContainer gateways, std::string filename){
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

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  buildingHandler
 *  Description:  
 * =====================================================================================
 */
void buildingHandler ( NodeContainer endDevices, NodeContainer gateways ){
	double xLength = 130;
  	double deltaX = 32;
  	double yLength = 64;
  	double deltaY = 17;
  	int gridWidth = 2*radius/(xLength+deltaX);
  	int gridHeight = 2*radius/(yLength+deltaY);
  	
	if (buildingsEnabled == false){
    	gridWidth = 0;
      	gridHeight = 0;
    }
  	
	Ptr<GridBuildingAllocator> gridBuildingAllocator;
  	gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  	gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (gridWidth));
  	gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (xLength));
  	gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (yLength));
  	gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (deltaX));
  	gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (deltaY));
  	gridBuildingAllocator->SetAttribute ("Height", DoubleValue (6));
  	gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (2));
  	gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (4));
  	gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (2));
  	gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (-gridWidth*(xLength+deltaX)/2+deltaX/2));
  	gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (-gridHeight*(yLength+deltaY)/2+deltaY/2));
  	BuildingContainer bContainer = gridBuildingAllocator->Create (gridWidth * gridHeight);

  	BuildingsHelper::Install (endDevices);
  	BuildingsHelper::Install (gateways);
  	BuildingsHelper::MakeMobilityModelConsistent ();
	
	if(printBuildings){
		std::ofstream myfile;
    	myfile.open ("buildings.dat");
    	std::vector<Ptr<Building> >::const_iterator it;
    	int j = 1;
    	for (it = bContainer.Begin (); it != bContainer.End (); ++it, ++j){
    		Box boundaries = (*it)->GetBoundaries ();
        	myfile << "set object " << j << " rect from " << boundaries.xMin << "," << boundaries.yMin << " to " << boundaries.xMax << "," << boundaries.yMax << std::endl;
    	}
    	myfile.close();
	}
}/* -----  end of function buildingHandler  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main (int argc, char *argv[]){

 	string fileMetric="./scratch/mac-sta-inf.txt";
  	string fileData="./scratch/mac-sta-dat.dat";
 	string pcapfile="./scratch/mac-s1g-slots";
	string endDevFile="./TestResult/test";
	int trial=1, packLoss=0;
	double angle=0, sAngle=0;
	float G=0, S=0;
	Time avgDelay = NanoSeconds(0);

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
  	cmd.AddValue ("file3", "files containing result data", fileDelay);
  	cmd.AddValue ("trial", "files containing result data", trial);
  	cmd.Parse (argc, argv);
	
	endDevFile += to_string(trial) + "/endDevices" + to_string(nDevices) + ".dat";
	
	ofstream myfile;
	myfile.open (fileDelay, ios::out | ios::app);
	myfile << nDevices << ":\n";
	myfile.close();
	
  	// Set up logging
  	//LogComponentEnable ("LoRaWanNetworkSimulator", LOG_LEVEL_ALL);
  	//LogComponentEnable ("NetworkServer", LOG_LEVEL_ALL);
	//LogComponentEnable ("NetworkController", LOG_LEVEL_ALL);
  	//LogComponentEnable ("NetworkControllerComponent", LOG_LEVEL_ALL);
 	//LogComponentEnable ("NetworkScheduler", LOG_LEVEL_ALL);
 	//LogComponentEnable ("NetworkStatus", LOG_LEVEL_ALL);
  	//LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
  	//LogComponentEnable("CorrelatedShadowingPropagationLossModel", LOG_LEVEL_INFO);
  	//LogComponentEnable("BuildingPenetrationLoss", LOG_LEVEL_INFO);
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
        	                       "X", DoubleValue (0.0),
            	                   "Y", DoubleValue (0.0));
  	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  	/************************
  	*  Create the channel  *
  	************************/

  	// Create the lora channel object
  	Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
	loss->SetPathLossExponent (3.76);
  	loss->SetReference (1, 7.7);

  	if(shadowingEnabled){
    	// Create the correlated shadowing component
    	Ptr<CorrelatedShadowingPropagationLossModel> shadowing = CreateObject<CorrelatedShadowingPropagationLossModel> ();

    	// Aggregate shadowing to the logdistance loss
    	loss->SetNext(shadowing);

    	// Add the effect to the channel propagation loss
    	Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss> ();

    	shadowing->SetNext(buildingLoss);
    }

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
  	//double x=100.0, y=100.0;
  	for (NodeContainer::Iterator j = endDevices.Begin ();
    	j != endDevices.End (); ++j){
      	Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      	Vector position = mobility->GetPosition ();
		//position.x = x;
		//position.y = y;
		//x +=200;
		//y +=200;
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
	//	Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac>();
	//	mac->SetMType (LoraMacHeader::CONFIRMED_DATA_UP);
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
	
	/**********************
   	*  Handle buildings  *
   	**********************/
	buildingHandler(endDevices, gateways);	

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

  	//macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
  	//macHelper.SetSpreadingFactorsUp (endDevices);
  	uint8_t count=5;
	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j){
		Ptr<Node> object = *j;
    	Ptr<NetDevice> netDevice = object->GetDevice (0);
      	Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      	NS_ASSERT (loraNetDevice != 0);
      	Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      	NS_ASSERT (mac != 0);
    	
		mac->SetDataRate(count);
		//count -=1;
	}

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
	NS_LOG_DEBUG("sTime:" << appStartTime << "  pTime:" << appStopTime);
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

  	Simulator::Stop (appStopTime + Hours(1));

  	// PrintSimulationTime ();
  	Simulator::Run ();
  	Simulator::Destroy ();

  	/*************
  	*  Results  *
  	*************/
  	double throughput = 0;
  	packLoss = sent - packSucc;
  	uint32_t totalPacketsThrough = packSucc;
  	throughput = totalPacketsThrough * 19 * 8 / ((simulationTime - appStartTime) * 1000.0);

 	NS_LOG_DEBUG("sumT: " << sumToA.GetSeconds() << " int: " << appPeriodSeconds );
  	
	//normalized offered traffic	
	G =  (double)sumToA.GetSeconds()/appPeriodSeconds;

  	double probSucc = (double(packSucc)/sent);
  	double probLoss = (double(packLoss)/sent)*100;
	double probInte = (double(interfered)/sent)*100;
	double probNoMo = (double(noMoreReceivers)/sent)*100;
	double probUSen = (double(underSensitivity)/sent)*100;

	//normalized throughput
  	S = G*probSucc;  
	
	NS_LOG_DEBUG("pSucc: " << probSucc << " G: " << G << " S: " << S);
	
	avgDelay = sumDelay/nDevices;
  	NS_LOG_DEBUG("avgDelay: " << avgDelay);
 
 	probSucc = probSucc * 100;
  
	myfile.open (fileDelay, ios::out | ios::app);
	myfile << "\n\n";
	myfile.close();
	
   	cout << nDevices << ", " << throughput << ", " << probSucc << ", " << probLoss << ", " << probInte << ", " << probNoMo << ", " << probUSen << ", " << avgDelay.GetNanoSeconds() << ", " << G << ", " << S << endl;


  	myfile.open (fileMetric, ios::out | ios::app);
  	myfile << nDevices << ", " << throughput << ", " << probSucc << ", " <<  probLoss << ", " << probInte << ", " << probNoMo << ", " << probUSen << ", " <<  ", " << avgDelay.GetNanoSeconds() << ", " << G << ", " << S << "\n";
  	myfile.close();  
  

 	cout << endl << endl << "numDev:" << nDevices << " numGW:" << nGateways << " simTime:" << simulationTime << " avgDelay:" << avgDelay.GetNanoSeconds() << " throughput:" << throughput << endl;
  	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
  	cout << "sent:" << sent << " succ:" << packSucc << " drop:"<< packLoss << " rec:" << received << " interf:" << interfered << " noMoreRec:" << noMoreReceivers << " underSens:" << underSensitivity << endl;
  	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;


  	myfile.open (fileData, ios::out | ios::app);
  	myfile << "sent: " << sent << " succ: " << packSucc << " drop: "<< packLoss << " rec: " << received << " interf: " << interfered << " noMoreRec: " << noMoreReceivers << " underSens: " << underSensitivity << "\n";
  	myfile << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << "\n";
  	myfile << "numDev: " << nDevices << " numGat: " << nGateways << " simTime: " << simulationTime << " throughput: " << throughput<< "\n";
  	myfile << "#######################################################################" << "\n\n";
  	myfile.close();  
 
  	return(0);
}/* ----------  end of function main  ---------- */

