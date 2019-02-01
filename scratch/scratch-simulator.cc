/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/node-container.h"

using namespace ns3;
using namespace std;


NS_LOG_COMPONENT_DEFINE ("ScratchSimulator");

int  main (int argc, char *argv[]){
  	NS_LOG_UNCOND ("Scratch Simulator");

	double scale = 10;
	double shape = 2.5;
	
  	NodeContainer endDevices;
  	endDevices.Create (3);

	for (NodeContainer::Iterator j = endDevices.Begin (); j < endDevices.End(); ++j){
		cout<< endDevices.GetN() << endl;
	}

	Ptr<ParetoRandomVariable> x = CreateObject<ParetoRandomVariable> ();
	x->SetAttribute ("Scale", DoubleValue (scale));
	x->SetAttribute ("Shape", DoubleValue (shape));
	// The expected value for the mean of the values returned by a
	// // Pareto distributed random variable is
	// //
	// //                   shape * scale
	// //     E[value]  =  ---------------  ,
	// //                     shape - 1
	double value = x->GetValue ();
	cout << "value=" << value << endl;
	
	Simulator::Run ();
  	Simulator::Destroy ();
  
	return(0);
}
