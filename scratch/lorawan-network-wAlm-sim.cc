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
#include "ns3/random-sender-helper.h"
#include "ns3/command-line.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("LoRaWanNetworkSimulator");

#define MAXRTX	6

// Network settings
int nDevices = 2;
int nRegulars =1;
int nAlarms = 1;
int gatewayRings = 1;
int nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
double radius = 800;
double gatewayRadius = 7500/((gatewayRings-1)*2+1);
double simulationTime = 3600;
int appPeriodSeconds = 60;
vector<uint8_t> totalTxAmounts (MAXRTX, 0);
vector<Time> sndTimeDelay;

typedef struct _Statistics{
	int noMoreReceivers = 0;
	int interfered = 0;
	int underSensitivity = 0;
	int received = 0;
	int sent = 0;
//	int packSucc = 0;
}Statistics;

enum Scenaries{
	MODE0,
	MODE1,
	MODE2,
	MODE3,
	MODE4,
	MODE5,
	MODE6,
	MODE7,
	MODE8,
	MODE9,
};				/* ----------  end of enum _StrategieSF  ---------- */

Statistics pktRegulars;
Statistics pktAlarms;

// sum Time on Air for regular events
//Time sumRegToA=Seconds(0);
//Time sumRegDelay=Seconds(0);

// sum Time on Air for alarm events
//Time sumAlmToA=Seconds(0);
Time sumAlmDelay=Seconds(0);

// Channel model
bool shadowingEnabled = false;
bool buildingsEnabled = false;

// Output control
bool printEDs = true;
bool printBuildings = false;
//bool printDelay = true;
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
	bool rtxFlag;
//	bool packFlag;
	bool almFlag;
	Time sndTime;
	Time rcvTime;
	Time duration; 
	int outcomeNumber;
	vector<enum PacketOutcome> outcomes;
};

std::map<Ptr<Packet const>, PacketStatus> packetTracker;

void CheckReceptionByAllGWsComplete (std::map<Ptr<Packet const>, PacketStatus>::iterator it){
	if(!(*it).second.almFlag){
		// Check whether this packet is received by all gateways	
		if ((*it).second.outcomeNumber == nGateways){
			NS_LOG_DEBUG("Regular receved all gw");
      		// Update the statistics
      		PacketStatus status = (*it).second;
      		for (int j = 0; j < nGateways; j++){
          		switch (status.outcomes.at (j)){
						case RECEIVED:
									pktRegulars.received += 1;
	         			break;
            			case UNDER_SENSITIVITY:
/*  									if(status.rtxFlag && status.rtxNum)
										pktRegulars.sent -= 1;
									else
*/   	          						pktRegulars.underSensitivity += 1;
           				break;
            			case NO_MORE_RECEIVERS:
/*    									if(status.rtxFlag && status.rtxNum)
										pktRegulars.sent -= 1;
									else
*/            	   						pktRegulars.noMoreReceivers += 1;
             			break;
            			case INTERFERED:
/*                       				if(status.rtxFlag && status.rtxNum)
										pktRegulars.sent -= 1;
									else
*/	               						pktRegulars.interfered += 1;
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
	}else{
		// Check whether this packet is received by all gateways		
		if ((*it).second.outcomeNumber == nGateways){
			NS_LOG_DEBUG("alarm receved all gw");
      		// Update the statistics
      		PacketStatus status = (*it).second;
			int id = status.senderId - nRegulars;
      		for (int j = 0; j < nGateways; j++){
          		switch (status.outcomes.at (j)){
						case RECEIVED:
							//cout << "received" << endl;
							pktAlarms.received += 1;
							if (status.rtxNum < (MAXRTX-1)){
								NS_LOG_DEBUG("almDely:" << (status.rcvTime - sndTimeDelay[id]).GetMilliSeconds() << " milliSeconds");
								sumAlmDelay += status.rcvTime - sndTimeDelay[id];
								sndTimeDelay[id] = Seconds(0);
							}else{
								NS_LOG_DEBUG("almDely:" << (status.rcvTime - status.sndTime).GetMilliSeconds() << " milliSeconds");
								sumAlmDelay += status.rcvTime - status.sndTime;
							}
							NS_LOG_DEBUG("sumAlmDely:" << sumAlmDelay.GetMilliSeconds() << " milliSeconds");
            			break;
            			case UNDER_SENSITIVITY:
								if(status.rtxFlag && status.rtxNum)
									pktAlarms.sent -= 1;
								else
              					pktAlarms.underSensitivity += 1;
           				break;
            			case NO_MORE_RECEIVERS:
							  	if(status.rtxFlag && status.rtxNum)
									pktAlarms.sent -= 1;
								else
	               	   				pktAlarms.noMoreReceivers += 1;
             			break;
            			case INTERFERED:
			 	  				if(status.rtxFlag && status.rtxNum)
									pktAlarms.sent -= 1;
								else
	               					pktAlarms.interfered += 1;
           				break;
            			case UNSET:
   	            			//cout << "unset" << endl;
	               		break;
	    				default:
	  	            		//cout << "default" << endl;
	 					break;
            	}
        	}
      		// Remove the packet from the tracker
      		packetTracker.erase (it);
		}
	}
}

void TransmissionRegularCallback (Ptr<Packet> packet, LoraTxParameters txParams, uint32_t systemId){
	NS_LOG_INFO ("Transmitted a regular packet from device " << systemId);
	
  	// Create a packetStatus
	PacketStatus status;
	status.packet = packet;
	status.senderId = systemId;
	status.rtxNum = txParams.retxLeft;
	status.rtxFlag = txParams.retxFlag;
	status.almFlag = 0;
//	status.packFlag = 1;
	if(status.rtxNum >= MAXRTX-1)
		status.sndTime = Simulator::Now();
	status.rcvTime = Time::Max();
	status.outcomeNumber = 0;
	status.outcomes = std::vector<enum PacketOutcome> (nGateways, UNSET);
	status.duration = LoraPhy::GetOnAirTime (packet, txParams);	
	pktRegulars.sent += 1;
	NS_LOG_DEBUG("Regular flag:" << txParams.retxFlag << " num:" << (unsigned)txParams.retxLeft);	
		
/*	if (status.rtxFlag)
		totalTxAmounts.at(4-status.rtxNum-1)++;
 	else
		totalTxAmounts.at(4-status.rtxNum)++;
*/
  	packetTracker.insert (std::pair<Ptr<Packet const>, PacketStatus> (packet, status));
}
   
void TransmissionAlarmCallback (Ptr<Packet> packet, LoraTxParameters txParams, uint32_t systemId){
	NS_LOG_INFO ("Transmitted a alarm packet from device " << systemId);
 
	// Create a packetStatus
	PacketStatus status;
	int id;

	status.packet = packet;
	status.senderId = systemId;
	status.rtxNum = txParams.retxLeft;
	status.rtxFlag = txParams.retxFlag;
//	status.packFlag = 1;
	status.almFlag = 1;
	if(status.rtxNum >= MAXRTX-1)	
		status.sndTime = Simulator::Now();
	status.rcvTime = Time::Max();
	status.outcomeNumber = 0;
	status.outcomes = std::vector<enum PacketOutcome> (nGateways, UNSET);
	status.duration = LoraPhy::GetOnAirTime (packet, txParams);	
	pktAlarms.sent += 1;
	id = status.senderId - nRegulars;

	NS_LOG_DEBUG("sf: " << (unsigned)txParams.sf << " sndT:" << status.sndTime.GetMilliSeconds() << " num:" << (unsigned)txParams.retxLeft);	
	
	if (status.rtxFlag && status.sndTime != Seconds(0)){
		NS_LOG_DEBUG("insert value in " << id << " sndT: " << status.sndTime.GetMilliSeconds());
		sndTimeDelay[id] = status.sndTime;
	}

	if (status.rtxFlag)
		totalTxAmounts.at(MAXRTX-status.rtxNum-1)++;
 	else
		totalTxAmounts.at(MAXRTX-status.rtxNum)++;

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

void PrintEndDevices (NodeContainer endDevices, NodeContainer gateways, std::string filename1, std::string filename2, std::string filename3){
  	const char * c = filename1.c_str ();
  	std::ofstream spreadingFactorFile;
  	spreadingFactorFile.open (c);
	// print for regular event
  	for (int j = 0; j < nRegulars; ++j){
    	Ptr<Node> object = endDevices.Get(j);
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
	spreadingFactorFile.close ();
  	
  	c = filename2.c_str ();
  	spreadingFactorFile.open (c);
	// print for alarm event
  	for (int j = nRegulars; j < nDevices; ++j){
    	Ptr<Node> object = endDevices.Get(j);
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
  	spreadingFactorFile.close ();
  	
	c = filename3.c_str ();
  	spreadingFactorFile.open (c);	
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
 *         Name:  setStrategiesAllocationSF
 *  Description:  
 * =====================================================================================
 */
void setStrategiesAllocationSF (NodeContainer endDevices, uint8_t mode){
		uint8_t countAlm = 1;
		uint8_t countReg = 1;
		
		switch (mode) {
				case MODE2:
				case MODE5: 
				case MODE8:
					{
						//cout << "mode: " << (unsigned)mode << endl;	
						for (int j = nRegulars; j < nDevices; ++j){
							Ptr<Node> object = endDevices.Get(j);
    						Ptr<NetDevice> netDevice = object->GetDevice (0);
      						Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      						NS_ASSERT (loraNetDevice != 0);
      						Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      						NS_ASSERT (mac != 0);
    						countAlm = mac->GetDataRate();
							if (countAlm)
								countAlm -=1;
							mac->SetDataRate(countAlm);
						}
					}			
					break;
				case MODE3:
				case MODE6:
				case MODE9:
					{
						//cout << "mode: " << (unsigned)mode << endl;
						uint8_t sfAlm =5;
						for (int j = nRegulars; j < nDevices; ++j){
							Ptr<Node> object = endDevices.Get(j);
    						Ptr<NetDevice> netDevice = object->GetDevice (0);
      						Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      						NS_ASSERT (loraNetDevice != 0);
      						Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      						NS_ASSERT (mac != 0);
    						countAlm = mac->GetDataRate();
							if (countAlm < sfAlm)
								sfAlm = countAlm;
						}
						for (int j = nRegulars; j < nDevices; ++j){
							Ptr<Node> object = endDevices.Get(j);
    						Ptr<NetDevice> netDevice = object->GetDevice (0);
      						Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      						NS_ASSERT (loraNetDevice != 0);
      						Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      						NS_ASSERT (mac != 0);
							mac->SetDataRate(sfAlm);
						}

						for (int j = 0; j < nRegulars; ++j){
							Ptr<Node> object = endDevices.Get(j);
    						Ptr<NetDevice> netDevice = object->GetDevice (0);
      						Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      						NS_ASSERT (loraNetDevice != 0);
      						Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      						NS_ASSERT (mac != 0);
    						countReg = mac->GetDataRate();
							if (countReg == sfAlm){
								countReg -=1;
								mac->SetDataRate(countReg);
							}
						}
					}
					break;
				default:
					//cout << "sf default"  << endl;
					break;
		}				/* -----  end switch  ----- */
}		/* -----  end of function setStrategiesAllocationSF  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  buildingHandler
 *  Description:  
 * =====================================================================================
 */
void buildingHandler ( NodeContainer endDevices, NodeContainer gateways ){
	double xLength = 100;
  	double deltaX = 50;
  	double yLength = 60;
  	double deltaY = 70;
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
  	gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (1));
  	gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (1));
  	gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (1));
  	gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (-gridWidth*(xLength+deltaX)/2+deltaX/2));
  	gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (-gridHeight*(yLength+deltaY)/2+deltaY/2));
  	BuildingContainer bContainer = gridBuildingAllocator->Create (gridWidth * gridHeight);

  	BuildingsHelper::Install (endDevices);
  	BuildingsHelper::Install (gateways);
  	//BuildingsHelper::MakeMobilityModelConsistent ();
	
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
 *         Name:  findSfMin
 *  Description: find SF smallest frequent   
 * =====================================================================================
 */
int32_t findSfMin ( int *vec ){
	int32_t num = distance(vec, min_element(vec,vec+6));
	
	if (vec[1] == vec[0])
		num = 1;
	else if (vec[2] == vec[1])
		num = 2; 
	else if (vec[3] == vec[2])
		num = 3; 
	else if (vec[4] == vec[3])
		num = 4; 
	else if (vec[5] == vec[4])
		num = 5; 
	
	//cout << "The index of smallest element is "  << num << endl;
	
	return(num);
}		/* -----  end of function findSfMin  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  starEdge
 *  Description: Alarms topology in start 
 * =====================================================================================
 */
void starEdge ( NodeContainer endDevices ){	
    double angleAlm=0, sAngleAlm=M_PI/2;
    int radiusAlm;
	if (nAlarms < 8)
		radiusAlm=4000;   // openfield 3000
    else if (nAlarms < 10)
		radiusAlm=3000;    // indoor 350; openFild 
	else if(nAlarms < 12)
        radiusAlm=5500; // openField 5500; bigPlant 1400
	else if(nAlarms < 14)
        radiusAlm=5000; // openField 5500; bigPlant 1400
   	else if (nAlarms < 20)
		radiusAlm=5500;
    else
        radiusAlm=1000;
    uint8_t op=nAlarms%4, count=4, count2=2;
    //double angleAlm=0, sAngleAlm=3*M_PI/4;    
    //int radiusAlm=200;
    //iterate our nodes and print their position.
    for (int j = nRegulars; j < nDevices; ++j){
        Ptr<Node> object = endDevices.Get(j);
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        NS_ASSERT (position != 0);
        Vector pos = position->GetPosition ();
        switch (op) {
                case 0:
                        pos.x = radiusAlm * cos(angleAlm);
                        pos.y = radiusAlm * sin(angleAlm);
                        if(count==1){
							if (nAlarms < 8)
								radiusAlm += 0;
							else if (nAlarms < 9)
								radiusAlm = 5500;
							else if (nAlarms < 10)
								radiusAlm += 3500;    // indoor 120; openfield 3500
                            else if (nAlarms < 12)
                                radiusAlm -= 1000; // openField 1000; bigPlant 500
                            else if (nAlarms < 14)
                                radiusAlm -= 1500; // openField 1000; bigPlant 500
                            else if (nAlarms < 16)
								radiusAlm -= 800;
                            else if (nAlarms < 18)
								radiusAlm -= 1000;
                            else if (nAlarms < 20)
								radiusAlm -= 800;
							else
                                radiusAlm += 1200;
                            angleAlm += M_PI/4;
                            count=5;
                        }
                        count--;
                        break;
                case 1:
                        pos.x = 0;
                        pos.y = 0;
                        op = 0;
                        break;
                case 2:
						if (nAlarms < 7){
					       	pos.x = 1000 * cos(angleAlm); // openfiled
                        	pos.y = 1000 * sin(angleAlm);
						}else if (nAlarms < 10){
				       		pos.x = 150 * cos(angleAlm); // smallPlant 150
                        	pos.y = 150 * sin(angleAlm);
						}else if (nAlarms < 12){
			       			pos.x = 2000 * cos(angleAlm); // opendFiled 2000; bigPlant 250
                        	pos.y = 2000 * sin(angleAlm);
						}else if (nAlarms < 15){
			       			pos.x = 1200 * cos(angleAlm); // opendFiled 2000; bigPlant 250
                        	pos.y = 1200 * sin(angleAlm);
						}else if (nAlarms < 20){
			       			pos.x = 1300 * cos(angleAlm); // opendFiled 2000; bigPlant 250
                        	pos.y = 1300 * sin(angleAlm);
						}else{
                        	pos.x = 800 * cos(angleAlm);
                        	pos.y = 800 * sin(angleAlm);
						}
                        
						if(count2==2){
                            sAngleAlm = M_PI;
                        }else{
                            //angleAlm = M_PI/4;
                            sAngleAlm = M_PI/2;
                            op = 0;
                        }
                        count2--;
                        break;
                case 3:
    					if (nAlarms < 12){
			       			pos.x = 2000 * cos(angleAlm); //openField 2000; bigPlant 500
                        	pos.y = 2000 * sin(angleAlm);
						}else{
                        	pos.x = 1500 * cos(angleAlm);
                        	pos.y = 1500 * sin(angleAlm);
						}
                        
						if(count2==2){
                            sAngleAlm = M_PI;
                        }else{
                            angleAlm = 0;
                            sAngleAlm = M_PI/2;
                            op = 1;
                        }
                        count2--;
                        break;
                default:
                        break;
        }// -----  end switch  ----- 
        position->SetPosition (pos);
        angleAlm += sAngleAlm;
        //radiusAlm -= 1000;
    }	
}		/* -----  end of function startEdge  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  orbitEdge
 *  Description:  Alarms topology in orbit
 * =====================================================================================
 */
void orbitEdge ( NodeContainer endDevices ){
    double angleAlm=0, sAngleAlm=3*M_PI/4;
    int radiusAlm=1000; // openField 800; bigPlant 200
    
    // iterate our nodes and print their position.
    for (int j = nRegulars; j < nDevices; ++j){
        Ptr<Node> object = endDevices.Get(j);
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        NS_ASSERT (position != 0);
        Vector pos = position->GetPosition ();
        pos.x = radiusAlm * cos(angleAlm);
        pos.y = radiusAlm * sin(angleAlm);
        position->SetPosition (pos);
        angleAlm += sAngleAlm;
		if (nAlarms >= 20)
			radiusAlm += 200;
		else if (nAlarms >= 18)
			radiusAlm += 250;
        else if (nAlarms >= 15)
            radiusAlm += 300;
        else if (nAlarms >= 13)
            radiusAlm += 400;
		else if (nAlarms >= 11)
			radiusAlm += 500; 
        else if (nAlarms >= 9)  // openField 600; bigPlant 150
            radiusAlm += 600;
        else if (nAlarms >= 7)
            radiusAlm += 750;
        else
            radiusAlm += 1000; 
    }	
}		/* -----  end of function orbitEdge  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  circleEdge
 *  Description: Aalrm topology in circle 
 * =====================================================================================
 */
void circleEdge ( NodeContainer endDevices ){
	double angleAlm=0, sAngleAlm=2*M_PI/nAlarms;	
    //double angleAlm=0, sAngleAlm=3*M_PI/4;	
  	int radiusAlm=300;
  	
	// iterate our nodes and print their position.
  	for (int j = nRegulars; j < nDevices; ++j){
    	Ptr<Node> object = endDevices.Get(j);
     	Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
     	NS_ASSERT (position != 0);
  		Vector pos = position->GetPosition ();
		pos.x = radiusAlm * cos(angleAlm);
 		pos.y = radiusAlm * sin(angleAlm);
	   	position->SetPosition (pos);
		angleAlm += sAngleAlm;
		//radiusAlm += 300; 
  	}
}/* -----  end of function circleEdge  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  printSumTransmission
 *  Description:  
 * =====================================================================================
 */
uint8_t printSumTransmission ( ){
	uint8_t total = 0;
	cout << "vectRtx: ";
	for (int i = 0; i < int(totalTxAmounts.size ()); i++){
        cout << (unsigned)totalTxAmounts[i] << " ";
        total += totalTxAmounts[i];
    }
	cout << endl;
	return(total);
}		/* -----  end of function printSumTransmission  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main (int argc, char *argv[]){

 	string fileRegularMetric="./TestResult/mac-sta-inf.txt";
  	string fileRegularData="./TestResult/mac-sta-dat.dat";
  	string fileAlarmMetric="./TestResult/mac-sta-inf.txt";
  	string fileAlarmData="./TestResult/mac-sta-dat.dat";
  	string fileRtx="./TestResult/alarmsRtx.dat";
 	string pcapfile="./TestResult/mac-s1g-slots";
	string endDevRegFile="./TestResult/test";
	string endDevAlmFile="./TestResult/test";
	string gwFile="./TestResult/test";
	int trial=1, packLoss=0;
  	uint32_t nSeed=1;
	double angle=0, sAngle=0, avgAlmDelay = 0;
	//float G=0, S=0;
	ofstream myfile;

  	CommandLine cmd;
   	cmd.AddValue ("nSeed", "Number of seed to position", nSeed);
  	cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  	cmd.AddValue ("gatewayRings", "Number of gateway rings to include", gatewayRings);
  	cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  	cmd.AddValue ("gatewayRadius", "The distance between two gateways", gatewayRadius);
  	cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  	cmd.AddValue ("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds);
  	cmd.AddValue ("printEDs", "Whether or not to print a file containing the ED's positions", printEDs);
  	cmd.AddValue ("file1", "files containing result regular information", fileRegularData);
  	cmd.AddValue ("file2", "files containing result regular data", fileRegularMetric);
   	cmd.AddValue ("file3", "files containing result alarm information", fileAlarmData);
  	cmd.AddValue ("file4", "files containing result alarm data", fileAlarmMetric);
  	cmd.AddValue ("file5", "files containing result delay", fileRtx);
  	cmd.AddValue ("trial", "set trial parameters", trial);
  	cmd.Parse (argc, argv);

	//nAlarms = 10;
	//nRegulars = nDevices - nAlarms;
	nRegulars = nDevices/(1.01); 
	nAlarms = nDevices - nRegulars;
	NS_LOG_DEBUG("number regular event: " << nRegulars << "number alarm event: " << nAlarms );


	endDevRegFile += to_string(trial) + "/endDevicesReg" + to_string(nRegulars) + ".dat";
	endDevAlmFile += to_string(trial) + "/endDevicesAlm" + to_string(nAlarms) + ".dat";
	gwFile += to_string(trial) + "/GWs" + to_string(gatewayRings) + ".dat";
  	
  	
	// Set up logging
  	//LogComponentEnable ("LoRaWanNetworkSimulator", LOG_LEVEL_ALL);
  	//LogComponentEnable ("NetworkServer", LOG_LEVEL_ALL);
	//LogComponentEnable ("NetworkController", LOG_LEVEL_ALL);
  	//LogComponentEnable ("NetworkControllerComponent", LOG_LEVEL_ALL);
 	//LogComponentEnable ("NetworkScheduler", LOG_LEVEL_ALL);
 	//LogComponentEnable ("NetworkStatus", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
  	//LogComponentEnable ("CorrelatedShadowingPropagationLossModel", LOG_LEVEL_INFO);
  	//LogComponentEnable ("BuildingPenetrationLoss", LOG_LEVEL_INFO);
  	//LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
  	//LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  	//LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LoraMac", LOG_LEVEL_ALL);
  	//LogComponentEnable ("EndDeviceLoraMac", LOG_LEVEL_ALL);
  	//LogComponentEnable ("GatewayLoraMac", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
	//LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
  	//LogComponentEnable ("Forwarder", LOG_LEVEL_ALL);
 	//LogComponentEnable ("DeviceStatus", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LoraMacHelper", LOG_LEVEL_ALL);
  	//LogComponentEnable ("PeriodicSenderHelper", LOG_LEVEL_ALL);
  	//LogComponentEnable ("PeriodicSender", LOG_LEVEL_ALL);
   	//LogComponentEnable ("RandomSenderHelper", LOG_LEVEL_ALL);
  	//LogComponentEnable ("RandomSender", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LoraMacHeader", LOG_LEVEL_ALL);
  	//LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);


  	/***********
  	*  Setup  *
  	***********/
  	RngSeedManager::SetSeed(1);
  	RngSeedManager::SetRun(nSeed);
  	
	// Compute the number of gateways
  	//nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
  	nGateways = gatewayRings;
  	sAngle = M_PI; //(2*M_PI)/(nGateways);

 
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

  	// Create a set regular of nodes
  	NodeContainer endDevices;
  	endDevices.Create (nDevices);

  	// Assign a mobility model to each node
  	mobility.Install (endDevices);
	/*  for (int j = 0; j < nRegulars; ++j){
			Ptr<Node> node = endDevices.Get(j);
			mobility.Install(node);
	}
  	
	for (int j = nRegulars;	j < nDevices; ++j){
    	Ptr<Node> node = endDevices.Get(j);
		mobilityAlm.Install(node);
	}*/

	
  	// Create a LoraDeviceAddressGenerator
  	uint8_t nwkId = 54;
  	uint32_t nwkAddr = 1864;
  	Ptr<LoraDeviceAddressGenerator> addrGen = CreateObject<LoraDeviceAddressGenerator> (nwkId,nwkAddr);

  	// Make it so that nodes are at a certain height > 0
	for (NodeContainer::Iterator j = endDevices.Begin (); 
		j != endDevices.End (); ++j){
    	Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      	Vector position = mobility->GetPosition ();
		//cout << "x: " << position.x << " y: " << position.y << endl;
		//position.x = 26.6043; 
		//position.y = -421.608;
      	position.z = 1.2;
      	mobility->SetPosition (position);
	}
  	
	/* circle edges */
	//circleEdge(endDevices);
  	
	/**************************************
  	*  Set up topologies for the alarms   *
  	***************************************/
 	switch (trial) {
			case MODE4:
			case MODE5:
			case MODE6:	
				// start edges 
				starEdge(endDevices);
				break;
			case MODE7:
			case MODE8:
			case MODE9:	
				// orbits edges	
				orbitEdge(endDevices);		
				break;
			default:	
				break;
	}				// -----  end switch  ----- 

  	// Create the LoraNetDevices of the regular end devices
  	phyHelper.SetDeviceType (LoraPhyHelper::ED);
  	macHelper.SetDeviceType (LoraMacHelper::ED);
  	macHelper.SetAddressGenerator (addrGen);
  	helper.Install (phyHelper, macHelper, endDevices);

  	// Now end devices and alarms are connected to the channel

  	// Connect trace sources for Regulars
  	for (int j = 0; j < nRegulars; ++j){
			Ptr<Node> node = endDevices.Get(j);
			Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
			Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
      		phy->TraceConnectWithoutContext ("StartSending",
        	                               MakeCallback (&TransmissionRegularCallback));
			//Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac>();
			//mac->SetMType (LoraMacHeader::CONFIRMED_DATA_UP);
	}
  	// Connect trace sources for Alarms
  	for (int j = nRegulars; j < nDevices; ++j){
			//cout << (j-nAlarms) << " ";   
			Ptr<Node> node = endDevices.Get(j);
			Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
			Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
      		phy->TraceConnectWithoutContext ("StartSending",
        	                               MakeCallback (&TransmissionAlarmCallback));
			Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac>();
			mac->SetMType (LoraMacHeader::CONFIRMED_DATA_UP);
			mac->SetMaxNumberOfTransmissions (MAXRTX);
			// initializer sumRtxDelay 
			sndTimeDelay.push_back(Seconds(0));	
	}
	//cout << "size: " << sndTimeDelay.size() << endl;

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
	// set spreading factor for regular event
  	macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
	
 	/********************************************************
  	*  Set up strategies for end device's spreading factor  *
  	*********************************************************/
	setStrategiesAllocationSF (endDevices, trial);

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

  	/****************************************************************
  	*  Install regulars and alarms applications on the end devices  *
  	****************************************************************/
   	Time appStopTime = Seconds(simulationTime);
 
  	PeriodicSenderHelper appRegularHelper = PeriodicSenderHelper ();
  	appRegularHelper.SetPeriod (Seconds (appPeriodSeconds));
  	ApplicationContainer appRegContainer = appRegularHelper.Install (endDevices.Get(0));
	for(int j = 1; j < nRegulars; j++)
		appRegContainer.Add(appRegularHelper.Install(endDevices.Get(j)));	

	uint32_t appStartTime = Simulator::Now().GetSeconds ();
 	NS_LOG_DEBUG("sTime:" << appStartTime << "  pTime:" << appStopTime.GetSeconds());
  	appRegContainer.Start (Seconds(appStartTime));
  	appRegContainer.Stop (appStopTime);
  
   	RandomSenderHelper appAlarmHelper = RandomSenderHelper ();
	ApplicationContainer appAlmContainer = appAlarmHelper.Install (endDevices.Get(nRegulars));
	for(int j = (nRegulars + 1); j < nDevices; j++)
		appAlmContainer.Add(appAlarmHelper.Install(endDevices.Get(j)));	

	NS_LOG_DEBUG("startTime:" << appStartTime << "  stopTime:" << appStopTime.GetSeconds());
  	appAlmContainer.Start (Seconds(appStartTime));
  	appAlmContainer.Stop (appStopTime);

 	/**********************
   	* Print output files *
   	*********************/
    if (printEDs){
    	PrintEndDevices (endDevices, gateways, endDevRegFile, endDevAlmFile, gwFile);
		//PrintSimulationTime ( );
 	}

  	/****************
  	*  Simulation  *
  	****************/

  	Simulator::Stop (appStopTime + Hours(1));

  	//PrintSimulationTime ();
    //oldtime = time (0) ;
  	Simulator::Run ();
    //cout << "Real time: " << std::time (0) - oldtime << " seconds" << endl;
  	Simulator::Destroy ();

  	/*****************************************
  	*  Statistics Results for regular event  *
  	*****************************************/
	NS_LOG_DEBUG("\n\n");
  	NS_LOG_DEBUG("/*****************************************");
  	NS_LOG_DEBUG("*  Statistics Results for regular event  *");
  	NS_LOG_DEBUG("*****************************************/");

  	double throughput = 0;
  	packLoss = pktRegulars.sent - pktRegulars.received;
	throughput = pktRegulars.received * 28 * 8 / ((simulationTime - appStartTime) * 1000.0);

	//normalized offered traffic	
	//G =  (double)sumRegToA.GetSeconds()/appPeriodSeconds;

  	double probSucc = (double(pktRegulars.received)/pktRegulars.sent);
  	double probLoss = (double(packLoss)/pktRegulars.sent)*100;
	double probInte = (double(pktRegulars.interfered)/pktRegulars.sent)*100;
	double probNoMo = (double(pktRegulars.noMoreReceivers)/pktRegulars.sent)*100;
	double probUSen = (double(pktRegulars.underSensitivity)/pktRegulars.sent)*100;

	//normalized throughput
  	//S = G*probSucc;  
	
	//NS_LOG_DEBUG("pSucc: " << probSucc << " G: " << G << " S: " << S);

	//avgDelay = (sumDelay/pktRegulars.received).GetMilliSeconds();
  	//NS_LOG_DEBUG("avgRegDelay: " << avgRegDelay << " milliSeconds");	
 
 	probSucc = probSucc * 100;
    

/*  	cout << endl << "nRegulars" << ", " << "throughput" << ", " << "probSucc" << ", " << "probLoss" << ", " << "probInte" << ", " << "probNoRec" << ", " << "probUSen" << endl; 
   	cout << "  "  << nRegulars << ",     " << throughput << ",     " << probSucc << ",     " << probLoss << ",    " << probInte << ", " << probNoMo << ", " << probUSen << endl;
*/
  	myfile.open (fileRegularMetric, ios::out | ios::app);
  	myfile << nDevices << ", " << throughput << ", " << probSucc << ", " <<  probLoss << ", " << probInte << ", " << probNoMo << ", " << probUSen << "\n";
  	myfile.close();  
  
 
/*  	cout << endl << "numDev:" << nRegulars << " numGW:" << nGateways << " simTime:" << simulationTime << " throughput:" << throughput << endl;
  	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
  	cout << "sent:" << pktRegulars.sent << " succ:" << pktRegulars.received << " drop:"<< packLoss << " rec:" << pktRegulars.received << " interf:" << pktRegulars.interfered << " noMoreRec:" << pktRegulars.noMoreReceivers << " underSens:" << pktRegulars.underSensitivity << endl;
  	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
*/

  	myfile.open (fileRegularData, ios::out | ios::app);
  	myfile << "sent: " << pktRegulars.sent << " succ: " << pktRegulars.received << " drop: "<< packLoss << " rec: " << pktRegulars.received << " interf: " << pktRegulars.interfered << " noMoreRec: " << pktRegulars.noMoreReceivers << " underSens: " << pktRegulars.underSensitivity << "\n";
  	myfile << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << "\n";
  	myfile << "numDev: " << nRegulars << " numGat: " << nGateways << " simTime: " << simulationTime << " throughput: " << throughput<< "\n";
  	myfile << "#######################################################################" << "\n\n";
  	myfile.close();  
 
  	/***************************************
  	*  Statistics Results for alarm event  *
  	***************************************/
	NS_LOG_DEBUG("\n\n");
   	NS_LOG_DEBUG("/*****************************************");
  	NS_LOG_DEBUG("*  Statistics Alarms for alarm event *");
  	NS_LOG_DEBUG("*****************************************/");

 	throughput = 0;
  	packLoss = pktAlarms.sent - pktAlarms.received;
	throughput = pktAlarms.received * 14 * 8 / ((simulationTime - appStartTime) * 1000.0);
	 
	//normalized offered traffic	
	//G =  (double)sumAlmToA.GetSeconds()/appPeriodSeconds;

  	probSucc = (double(pktAlarms.received)/pktAlarms.sent);
  	probLoss = (double(packLoss)/pktAlarms.sent)*100;
	probInte = (double(pktAlarms.interfered)/pktAlarms.sent)*100;
	probNoMo = (double(pktAlarms.noMoreReceivers)/pktAlarms.sent)*100;
	probUSen = (double(pktAlarms.underSensitivity)/pktAlarms.sent)*100;

	//normalized throughput
  	//S = G*probSucc;  
	
	//NS_LOG_DEBUG("pSucc: " << probSucc << " G: " << G << " S: " << S);

	NS_LOG_DEBUG((unsigned)printSumTransmission());

	myfile.open (fileRtx, ios::out | ios::app);
	myfile << nDevices << ": ";
 	for (const auto &e : totalTxAmounts) myfile << (unsigned)e << " ";
	myfile << "\n" ;
	myfile.close();

	if (pktAlarms.received)
		avgAlmDelay = (sumAlmDelay/pktAlarms.received).GetMilliSeconds();
  	NS_LOG_DEBUG("avgAlmDelay: " << avgAlmDelay << " milliSeconds");	
 


 	probSucc = probSucc * 100;
  
  cout << endl << "nAlarms" << ", " << "throughput" << ", " << "probSucc" << ", " << "probLoss" << ", " << "probInte" << ", " << "probNoRec" << ", " << "probUSen" << ", " << "avgAlmDelay" << endl; 
   	cout << "  "  << nAlarms << ",     " << throughput << ",     " << probSucc << ",     " << probLoss << ",    " << probInte << ",    " << probNoMo << ",     " << probUSen  << ",  " << avgAlmDelay << endl;

 	myfile.open (fileAlarmMetric, ios::out | ios::app);
  	myfile << nDevices << ", " << throughput << ", " << probSucc << ", " <<  probLoss << ", " << probInte << ", " << probNoMo << ", " << probUSen << ", " << avgAlmDelay << "\n";
  	myfile.close();  
 
  
/*   	cout << endl << "numDev:" << nAlarms << " numGW:" << nGateways << " simTime:" << simulationTime << " avgAlmDelay(mSec):" << avgAlmDelay << " throughput:" << throughput << endl;
  	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
  	cout << "sent:" << pktAlarms.sent << " succ:" << pktAlarms.received << " drop:"<< packLoss << " rec:" << pktAlarms.received << " interf:" << pktAlarms.interfered << " noMoreRec:" << pktAlarms.noMoreReceivers << " underSens:" << pktAlarms.underSensitivity << endl;
  	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
*/

  	myfile.open (fileAlarmData, ios::out | ios::app);
  	myfile << "sent: " << pktAlarms.sent << " succ: " << pktAlarms.received << " drop: "<< packLoss << " rec: " << pktAlarms.received << " interf: " << pktAlarms.interfered << " noMoreRec: " << pktAlarms.noMoreReceivers << " underSens: " << pktAlarms.underSensitivity << "\n";
  	myfile << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << "\n";
  	myfile << "numDev: " << nAlarms << " numGat: " << nGateways << " simTime: " << simulationTime << " throughput: " << throughput<< "\n";
  	myfile << "#######################################################################" << "\n\n";
  	myfile.close();  

  	return(0);
}/* ----------  end of function main  ---------- */

