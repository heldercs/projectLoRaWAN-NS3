/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
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

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include <math.h>

using namespace ns3;
using namespace std;

#define PI 3.14159265

NS_LOG_COMPONENT_DEFINE ("RandExample");

void 
CourseChange (std::string context, Ptr<const MobilityModel> position)
{
  Vector pos = position->GetPosition ();
  std::cout << Simulator::Now () << ", pos=" << position << ", x=" << pos.x << ", y=" << pos.y
            << ", z=" << pos.z << std::endl;
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
	
  //SeedManager::SetSeed (1);  // Changes seed from default of 1 to 3
  //SeedManager::SetRun (2);  // Changes run number from default of 1 to 7
  RngSeedManager::SetSeed(1);
  RngSeedManager::SetRun(10);
  //double angle = 0;
  uint32_t num = 10;
  //double sAngle = (2*M_PI)/num;
  NodeContainer c;
  c.Create (num);

 
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "X", DoubleValue (0.0),
                                 "Y", DoubleValue (0.0),
                                 "rho", DoubleValue (200));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);
/*   
    mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "MinX", DoubleValue (-5.0),
                                 "MinY", DoubleValue (-5.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (5.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install(c);
*/
  // Make it so that nodes are at a certain height > 0

  //Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  //allocator->Add (Vector (0.0, 0.0, 0.0));
  //mobility.SetPositionAllocator (allocator);
  mobility.Install (c);
  
  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = c.Begin ();
       j != c.End (); ++j){
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      mobility->SetPosition (position);
  }
  Simulator::Stop (Seconds (100.0));
  uint32_t i =0;
 
  while (i < num){
	//cout << "angle: " << angle << " sin: " << sin(angle) << endl;
    Ptr<MobilityModel> mobi = c.Get (i)->GetObject<MobilityModel>();
    Vector position = mobi->GetPosition();
    //position.x = 10 * cos(angle);
    //position.y = 10 * sin(angle);
    cout << "x:" << position.x << " y:" << position.y << endl;
 		i++;
    //angle+=sAngle;
	}

  //Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback (&CourseChange));
  
  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
