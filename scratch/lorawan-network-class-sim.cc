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
#include "ns3/random-sender-helper.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("LoRaWanNetworkSimulator");

#define MAXRTX	4
#define FLGRTX	0 

// Network settings
int nDevices = 2000;
int nDevicesSF[3] = {0,0,0};
int gatewayRings = 1;
int nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
double radius = 7500;
double gatewayRadius = 7500/((gatewayRings-1)*2+1);
double simulationTime = 600;
int appPeriodSeconds = 600;
//string fileDelay = "./scratch/delay.dat";


vector <vector <uint8_t> > totalTxAmounts (3, vector<uint8_t>(MAXRTX, 0));

#if FLGRTX
vector<Time> sndTimeDelay;
vector<uint8_t> statusRtx;
#endif

typedef struct _Statistics{
	int noMoreReceivers = 0;
	int interfered = 0;
	int underSensitivity = 0;
	int received = 0;
	int sent = 0;
}Statistics;

// sf 7, 8 and 9
Statistics pktSF[3];

// sum Time on Air
//Time sumToA=Seconds(0);
//Time sumDelay=Seconds(0);
Time sumSf7Delay=Seconds(0);

// Channel model
bool shadowingEnabled = false;
bool buildingsEnabled = false;

// Output control
bool printEDs = true;
bool printBuildings = false;
//bool printDelay = false;
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
	uint8_t rtxNum;
	uint8_t sf;
	bool rtxFlag; // used for set retransmission or not
	uint8_t outFlag; // used when having multi gateways for dont count the metrics more the once
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
      	// Update the statistics
      	PacketStatus status = (*it).second;
		//cout << "outF: " << (unsigned)status.outFlag << endl;
      	for (int j = 0; j < nGateways; j++){
          	switch (status.outcomes.at (j)){
					case RECEIVED:	
								if (!status.outFlag){
#if FLGRTX
									if (status.rtxNum < (MAXRTX-1)){
										NS_LOG_DEBUG("Dely:" << (status.rcvTime - sndTimeDelay[status.senderId]).GetMilliSeconds() << " milliSeconds");
										switch ( statusRtx[status.senderId] ) {
												case INTERFERED:
														pktSF[status.sf-7].interfered -= 1;
														break;
												case NO_MORE_RECEIVERS:	
														pktSF[status.sf-7].noMoreReceivers -= 1;
														break;
												case UNDER_SENSITIVITY:	
														pktSF[status.sf-7].underSensitivity -= 1;
														break;
												default:	
														break;
										}				/* -----  end switch  ----- */
										sumSf7Delay += status.rcvTime - sndTimeDelay[status.senderId];
									}else{
										NS_LOG_DEBUG("Dely:" << (status.rcvTime - status.sndTime).GetMilliSeconds() << " milliSeconds");
										sumSf7Delay += status.rcvTime - status.sndTime;
										NS_LOG_DEBUG("sumDely:" << sumSf7Delay.GetMilliSeconds() << " milliSeconds");
									}
#endif
									pktSF[status.sf - 7].received += 1;
									status.outFlag++;
								}
	           	   				NS_LOG_DEBUG("sf: " << (unsigned)status.sf << "receiver: " << pktSF[status.sf-7].received);
            					break;
            		case UNDER_SENSITIVITY:
			        			if (!status.outFlag){
									if(!status.rtxFlag || status.rtxNum == MAXRTX-1){		
	      									pktSF[status.sf-7].underSensitivity += 1;
#if FLGRTX
										statusRtx[status.senderId] = UNDER_SENSITIVITY;  
#endif
									}
									status.outFlag++;
								}
			           	   		NS_LOG_DEBUG("under_sensitivity: " << pktSF[status.sf-7].underSensitivity);
           						break;
            		case NO_MORE_RECEIVERS:
								if (!status.outFlag){
									if(!status.rtxFlag || status.rtxNum == MAXRTX-1){	
										pktSF[status.sf-7].noMoreReceivers += 1;
#if FLGRTX
										statusRtx[status.senderId] = NO_MORE_RECEIVERS;  
#endif
									}
									status.outFlag++;
								}
				           	   	NS_LOG_DEBUG("no_more_receivers: " << pktSF[status.sf-7].noMoreReceivers << " snt: " << pktSF[status.sf-7].sent);	
             					break;
            		case INTERFERED:
			 	      			if (!status.outFlag){
									if(!status.rtxFlag || status.rtxNum == MAXRTX-1){		
										pktSF[status.sf-7].interfered += 1;
#if FLGRTX
										statusRtx[status.senderId] = INTERFERED;  
#endif
									}
									status.outFlag++;
								}
					           	NS_LOG_DEBUG("interfe: " << pktSF[status.sf-7].interfered << " snt: " << pktSF[status.sf-7].sent);
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
	status.rtxNum = txParams.retxLeft;
	status.rtxFlag = txParams.retxFlag;
	status.sf = txParams.sf;
	status.outFlag = 0;
	if(!status.rtxFlag || status.rtxNum == MAXRTX-1)	
		status.sndTime = Simulator::Now();
	status.rcvTime = Time::Max();
	status.outcomeNumber = 0;
	status.outcomes = std::vector<enum PacketOutcome> (nGateways, UNSET);
	status.duration = LoraPhy::GetOnAirTime (packet, txParams);
	pktSF[status.sf-7].sent += 1;
	 	
	NS_LOG_DEBUG("Regular sf:" << (unsigned)txParams.sf << " sndTime: " << status.sndTime.GetMilliSeconds() << " num:" << (unsigned)txParams.retxLeft);	

#if FLGRTX
	if (status.rtxFlag && status.sndTime != Seconds(0)){
		NS_LOG_DEBUG("initialize vector");
		sndTimeDelay[status.senderId] = Seconds(0);
		NS_LOG_DEBUG("insert value in " << status.senderId << " sndT: " << status.sndTime.GetMilliSeconds());
		sndTimeDelay[status.senderId] = status.sndTime;
	}
	//cout << "id: " << systemId << " nRTX: " << (unsigned)status.rtxNum << " T: " << Simulator::Now().GetSeconds() << endl;
	(totalTxAmounts.at(status.sf-7)).at(MAXRTX-status.rtxNum-1)++;
#endif
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
		//NS_LOG_DEBUG("sf: " << sf);
		nDevicesSF[sf-7] += 1;
	
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
 *         Name:  sumReTransmission
 *  Description:  
 * =====================================================================================
 */
int sumReTransmission (uint8_t sf){
    uint8_t total = 0;
/*    cout << "Matrix Rtx: " << endl;
	for(int i=0; i<3; i++){
    	for (int j = 0; j < int(totalTxAmounts[sf-7].size ()); j++){
      		cout << (unsigned)totalTxAmounts[i][j] << " ";
    	}
		cout << endl;
	}
*/
    for (int i = 0; i < int(totalTxAmounts[sf-7].size ()); i++){
      //cout << (unsigned)totalTxAmounts[i] << " ";
      if (i)
		total += totalTxAmounts[sf-7][i];
    }
    //cout << endl;
    return(total);
}       /* -----  end of function printSumTransmission  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main (int argc, char *argv[]){

 	string fileMetric="./TestResult/test";
  	string fileMetric8="./TestResult/test";
  	string fileMetric9="./TestResult/test";
  	string fileData="./scratch/mac-sta-dat.dat";
 	string pcapfile="./scratch/mac-s1g-slots";
	string endDevFile="./TestResult/test";
	int trial=1, packLoss=0, numRTX=0;
  	uint32_t nSeed=1;
	double angle=0, sAngle=0;
	//float G=0, S=0;
	Time avgDelay = NanoSeconds(0);
	double throughput, probSucc_p, probSucc_t, probLoss, probInte, probNoMo, probUSen;

  	CommandLine cmd;
   	cmd.AddValue ("nSeed", "Number of seed to position", nSeed);
  	cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  	cmd.AddValue ("gatewayRings", "Number of gateway rings to include", gatewayRings);
  	cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  	cmd.AddValue ("gatewayRadius", "The distance between two gateways", gatewayRadius);
  	cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  	cmd.AddValue ("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds);
  	cmd.AddValue ("printEDs", "Whether or not to print a file containing the ED's positions", printEDs);
  	cmd.AddValue ("file1", "files containing result information", fileData);
  	//cmd.AddValue ("file2", "files containing result data", fileMetric);
  	//cmd.AddValue ("file3", "files containing result data", fileDelay);
  	cmd.AddValue ("trial", "files containing result data", trial);
  	cmd.Parse (argc, argv);
	
	endDevFile += to_string(trial) + "/endDevices" + to_string(nDevices) + ".dat";

	switch ((int)radius) {
#if FLGRTX
			case 2900:	
#else
			case 4200:	
#endif					
					fileMetric += to_string(trial) + "/traffic-10/result-STAs-SF7.dat";
					break;
#if FLGRTX
			case 3500:	
#else
			case 4900:	
#endif					
					fileMetric += to_string(trial) + "/traffic-10/result-STAs-SF7.dat";
					fileMetric8 += to_string(trial) + "/traffic-10/result-STAs-SF8.dat";
					break;
#if FLGRTX
			case 4200:	
#else
			case 5600:	
#endif					
					fileMetric += to_string(trial) + "/traffic-10/result-STAs-SF7.dat";
					fileMetric8 += to_string(trial) + "/traffic-10/result-STAs-SF8.dat";
					fileMetric9 += to_string(trial) + "/traffic-10/result-STAs-SF9.dat";
					break;
			default:	
					break;
	}				/* -----  end switch  ----- */

	ofstream myfile;
/*  myfile.open (fileDelay, ios::out | ios::app);
	myfile << nDevices << ":\n";
	myfile.close();
*/	
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
   	//LogComponentEnable("RandomSenderHelper", LOG_LEVEL_ALL);
  	//LogComponentEnable("RandomSender", LOG_LEVEL_ALL);
  	//LogComponentEnable("LoraMacHeader", LOG_LEVEL_ALL);
  	//LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);


  	/***********
  	*  Setup  *
  	***********/
  	RngSeedManager::SetSeed(1);
  	RngSeedManager::SetRun(nSeed);
  	
	// Compute the number of gateways
  	//nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
  	nGateways = gatewayRings;
  	sAngle = (2*M_PI)/(nGateways);
  
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
  	//double x=4300.0, y=0.0;
  	for (NodeContainer::Iterator j = endDevices.Begin ();
    	j != endDevices.End (); ++j){
      	Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      	Vector position = mobility->GetPosition ();
		//position.x = x;
		//position.y = y;
		//cout << "pos: " << position << endl; 
		//x +=100;
		//y +=100;
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
		//Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac>();
		//mac->SetMType (LoraMacHeader::CONFIRMED_DATA_UP);
#if FLGRTX
			Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac>();
			mac->SetMType (LoraMacHeader::CONFIRMED_DATA_UP);
			mac->SetMaxNumberOfTransmissions (MAXRTX);
			// initializer sumRtxDelay 
			sndTimeDelay.push_back(Seconds(0));
			statusRtx.push_back(0);
#endif
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
  	for (NodeContainer::Iterator j = gateways.Begin ();
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

#if FLGRTX 
   	macHelper.SetSpreadingFactorsUp (1, endDevices, gateways, channel);
#else
  	macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
#endif

  	//macHelper.SetSpreadingFactorsUp (endDevices);
  	/*  uint8_t count=5;
  	uint8_t count=5;
	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j){
		Ptr<Node> object = *j;
    	Ptr<NetDevice> netDevice = object->GetDevice (0);
      	Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      	NS_ASSERT (loraNetDevice != 0);
      	Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      	NS_ASSERT (mac != 0);
    	cout << "sf: " << (unsigned)phy->GetSpreadingFactor() << endl; 
		//mac->SetDataRate(count);
		//count -=1;
	}*/

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
  	RandomSenderHelper appHelper = RandomSenderHelper ();
  	appHelper.SetMean (Seconds (appPeriodSeconds));
  	ApplicationContainer appContainer = appHelper.Install (endDevices);
    //PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
    //appHelper.SetPeriod (Seconds (appPeriodSeconds));
    //ApplicationContainer appContainer = appHelper.Install (endDevices);
	
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
	//	PrintSimulationTime ( );
 	}

  	/****************
  	*  Simulation  *
  	****************/

  	Simulator::Stop (appStopTime + Hours(1));

  	//PrintSimulationTime ();
  	//oldtime = time (0);
	//cout << "init Time:" << endl;
  	Simulator::Run ();
  	Simulator::Destroy ();
  	//cout << "Real time: " << std::time (0) - oldtime << " seconds" << endl;
  	/*****************************************
  	*  Statistics Results for regular event  *
  	*****************************************/
	
	if (nDevicesSF[0]){
		/***************
		*  Results sf7 *
		***************/
		numRTX = sumReTransmission(7);
		NS_LOG_DEBUG("numRTX-SF7: " << numRTX);
		
		throughput = 0;
		packLoss = pktSF[0].interfered + pktSF[0].noMoreReceivers + pktSF[0].underSensitivity;
		//throughput = pktSF[0].received * 28 * 8 / ((simulationTime - appStartTime) * 1000.0); // throughput in kilo bits por seconds (kps)
		throughput = pktSF[0].received / (simulationTime - appStartTime); // throughput in packets por seconds

		probSucc_p = double(pktSF[0].received)/(pktSF[0].sent-numRTX);
		probSucc_t = double(pktSF[0].received)/pktSF[0].sent;
		probLoss = (double(packLoss)/(pktSF[0].sent-numRTX))*100;
		probInte = (double(pktSF[0].interfered)/(pktSF[0].sent-numRTX))*100;
		probNoMo = (double(pktSF[0].noMoreReceivers)/(pktSF[0].sent-numRTX))*100;
		probUSen = (double(pktSF[0].underSensitivity)/(pktSF[0].sent-numRTX))*100;

		probSucc_p = probSucc_p * 100;
		probSucc_t = probSucc_t * 100;
	
		/*	myfile.open (fileDelay, ios::out | ios::app);
		myfile << "\n\n";
		myfile.close();
		*/

//  		cout << endl << "nDevices7" << ", " << "throughput" << ", " << "probSucc_p" << ", " << "probSucc_t" << ", " << "probLoss" << ", " << "probInte" << ", " << "probNoRec" << ", " << "probUSen" << endl; 
//   		cout << "  " << nDevicesSF[0] << ",     " << throughput << ",     " << probSucc_p << ",     " << probSucc_t << ",     " << probLoss << ",    " << probInte << ", " << probNoMo << ", " << probUSen << endl;

	  	myfile.open (fileMetric, ios::out | ios::app);
  		myfile << nDevices << ", " << throughput << ", " << probSucc_p << ", " << probSucc_t  << ", " <<  probLoss << ", " << probInte << ", " << probNoMo << ", " << probUSen  << "\n";
  		myfile.close();  
  

/*     		cout << endl << "numDev7:" << nDevicesSF[0] << " numGW:" << nGateways << " simTime:" << simulationTime << " throughput:" << throughput << endl;
  		cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
  		cout << "sent:" << pktSF[0].sent << " succ:" << pktSF[0].received << " drop:"<< packLoss << " interf:" << pktSF[0].interfered << " noMoreRec:" << pktSF[0].noMoreReceivers << " underSens:" << pktSF[0].underSensitivity << endl;
  		cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
*/

  		myfile.open (fileData, ios::out | ios::app);
  		myfile << "sent: " << pktSF[0].sent << " reTrans: " << numRTX << " succ: " << pktSF[0].received << " drop: "<< packLoss << " rec: " << pktSF[0].received << " interf: " << pktSF[0].interfered << " noMoreRec: " << pktSF[0].noMoreReceivers << " underSens: " << pktSF[0].underSensitivity << "\n";
  		myfile << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << "\n";
  		myfile << "numDev: " << nDevices << " numGat: " << nGateways << " simTime: " << simulationTime << " throughput: " << throughput<< "\n";
  		myfile << "#######################################################################" << "\n\n";
  		myfile.close();  


	}
	
	if(nDevicesSF[1]){
		/***************
  		*  Results sf8 *
  		***************/
 		numRTX = sumReTransmission(8);
		NS_LOG_DEBUG("numRTX-SF8: " << numRTX);
		
		throughput = 0;
		packLoss = (pktSF[1].sent-numRTX) - pktSF[1].received;
		//throughput = pktSF[1].received * 28 * 8 / ((simulationTime - appStartTime) * 1000.0); // throughput in kilo bits por seconds (kps)
		throughput = pktSF[1].received / (simulationTime - appStartTime); // throughput in packets por seconds


		probSucc_p = double(pktSF[1].received)/(pktSF[1].sent-numRTX);
		probSucc_t = double(pktSF[1].received)/pktSF[1].sent;
		probLoss = (double(packLoss)/(pktSF[1].sent-numRTX))*100;
		probInte = (double(pktSF[1].interfered)/(pktSF[1].sent-numRTX))*100;
		probNoMo = (double(pktSF[1].noMoreReceivers)/(pktSF[1].sent-numRTX))*100;
		probUSen = (double(pktSF[1].underSensitivity)/(pktSF[1].sent-numRTX))*100;

		probSucc_p = probSucc_p * 100;
		probSucc_t = probSucc_t * 100;
	
		/*	myfile.open (fileDelay, ios::out | ios::app);
		myfile << "\n\n";
		myfile.close();
		*/	

//  		cout << endl << "nDevices8" << ", " << "throughput" << ", " << "probSucc_p" << ", " << "probSucc_t" << ", " << "probSucc_t" << ", " << "probLoss" << ", " << "probInte" << ", " << "probNoRec" << ", " << "probUSen" << endl; 
//   		cout << "  " << nDevicesSF[1] << ",     " << throughput << ",     " << probSucc_p << ",     " << probSucc_t << ",     " << probSucc_t << ",     " << probLoss << ",    " << probInte << ", " << probNoMo << ", " << probUSen << endl;

  		myfile.open (fileMetric8, ios::out | ios::app);
  		myfile << nDevices << ", " << throughput << ", " << probSucc_p << ", " << probSucc_t << ", " <<  probLoss << ", " << probInte << ", " << probNoMo << ", " << probUSen << "\n";
  		myfile.close();
  

/*    		cout << endl << "numDev8:" << nDevicesSF[1] << " numGW:" << nGateways << " simTime:" << simulationTime << " throughput:" << throughput << endl;
  		cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
  		cout << "sent:" << pktSF[1].sent << " reTrans: " << numRTX << " succ:" << pktSF[1].received << " drop:"<< packLoss << " rec:" << pktSF[1].received << " interf:" << pktSF[1].interfered << " noMoreRec:" << pktSF[1].noMoreReceivers << " underSens:" << pktSF[1].underSensitivity << endl;
  		cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
*/

  		myfile.open (fileData, ios::out | ios::app);
  		myfile << "sent: " << pktSF[1].sent << " reTrans: " << numRTX  << " succ: " << pktSF[1].received << " drop: "<< packLoss << " rec: " << pktSF[1].received << " interf: " << pktSF[1].interfered << " noMoreRec: " << pktSF[1].noMoreReceivers << " underSens: " << pktSF[1].underSensitivity << "\n";
  		myfile << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << "\n";
  		myfile << "numDev: " << nDevices << " numGat: " << nGateways << " simTime: " << simulationTime << " throughput: " << throughput<< "\n";
  		myfile << "#######################################################################" << "\n\n";
  		myfile.close();  
 	
	}
	
	if(nDevicesSF[2]){
		/***************
  		*  Results sf9 *
  		***************/
 		numRTX = sumReTransmission(9);
		NS_LOG_DEBUG("numRTX-SF9: " << numRTX);
		
		throughput = 0;
		packLoss = (pktSF[2].sent-numRTX) - pktSF[2].received;
		//throughput = pktSF[2].received * 28 * 8 / ((simulationTime - appStartTime) * 1000.0); // throughput in kilo bits por seconds (kps)
		throughput = pktSF[2].received / (simulationTime - appStartTime); // throughput in packets por seconds


		probSucc_p = double(pktSF[2].received)/(pktSF[2].sent-numRTX);
		probSucc_t = double(pktSF[2].received)/pktSF[2].sent;
		probLoss = (double(packLoss)/(pktSF[2].sent-numRTX))*100;
		probInte = (double(pktSF[2].interfered)/(pktSF[2].sent-numRTX))*100;
		probNoMo = (double(pktSF[2].noMoreReceivers)/(pktSF[2].sent-numRTX))*100;
		probUSen = (double(pktSF[2].underSensitivity)/(pktSF[2].sent-numRTX))*100;

		probSucc_p = probSucc_p * 100;
		probSucc_t = probSucc_t * 100;
	
		/*	myfile.open (fileDelay, ios::out | ios::app);
		myfile << "\n\n";
		myfile.close();
		*/	

  		//cout << endl << "nDevices9" << ", " << "throughput" << ", " << "probSucc_p" << ", " << "probSucc_t" << ", " << "probSucc_t" << ", " << "probLoss" << ", " << "probInte" << ", " << "probNoRec" << ", " << "probUSen" << endl; 
   		//cout << "  " << nDevicesSF[2] << ",     " << throughput << ",     " << probSucc_p << ",     " << probSucc_t << ",     " << probSucc_t << ",     " << probLoss << ",    " << probInte << ", " << probNoMo << ", " << probUSen << endl;


  		myfile.open (fileMetric9, ios::out | ios::app);
  		myfile << nDevices << ", " << throughput << ", " << probSucc_p << ", " << probSucc_t << ", " <<  probLoss << ", " << probInte << ", " << probNoMo << ", " << probUSen << "\n";
  		myfile.close();  
  

/*    		cout << endl << "numDev9:" << nDevicesSF[2] << " numGW:" << nGateways << " simTime:" << simulationTime << " throughput:" << throughput << endl;
  		cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
  		cout << "sent:" << pktSF[2].sent << " trans: " << numRTX << " succ:" << pktSF[2].received << " drop:"<< packLoss << " rec:" << pktSF[2].received << " interf:" << pktSF[2].interfered << " noMoreRec:" << pktSF[2].noMoreReceivers << " underSens:" << pktSF[2].underSensitivity << endl;
  		cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
*/

  		myfile.open (fileData, ios::out | ios::app);
  		myfile << "sent: " << pktSF[2].sent << " succ: " << pktSF[2].received << " drop: "<< packLoss << " rec: " << pktSF[2].received << " interf: " << pktSF[2].interfered << " noMoreRec: " << pktSF[2].noMoreReceivers << " underSens: " << pktSF[2].underSensitivity << "\n";
  		myfile << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << "\n";
  		myfile << "numDev: " << nDevices << " numGat: " << nGateways << " simTime: " << simulationTime << " throughput: " << throughput<< "\n";
  		myfile << "#######################################################################" << "\n\n";
  		myfile.close();  
 	
	}

 
  	return(0);
}/* ----------  end of function main  ---------- */

