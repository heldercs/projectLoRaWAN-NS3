/*
 * =====================================================================================
 *
 *       Filename:  testSimulator.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  23/08/2018 10:12:28
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Francisco Helder Candido (FHC), helderhdw@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "ns3/core-module.h"
#include <stdlib.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("testSimulator");

void handler (int arg0, int arg1){
	std::cout << "handler called with argument arg0=" << arg0 << " and arg1=" << arg1 << " time: " << Simulator::Now().GetSeconds() << std::endl;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] ){


  LogComponentEnable ("testSimulator", LOG_LEVEL_ALL);

  NS_LOG_INFO ("-----------");
  NS_LOG_INFO ("Simulation ");
  NS_LOG_INFO ("-----------");


	Simulator::Schedule(Seconds(10), &handler, 10, 5);

  Simulator::Stop (Hours (1));
	
  Simulator::Run ();
  std::cout << "andTime: " << Simulator::Now().GetHours() << std::endl;	
  Simulator::Destroy ();
	return(EXIT_SUCCESS);
}/* ----------  end of function main  ---------- */
