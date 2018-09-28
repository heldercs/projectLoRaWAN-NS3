/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 University of Washington
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
 */
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/command-line.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/double.h"
#include "ns3/gnuplot.h"
#include <iostream>
#include <fstream>

/**
 * \file
 * \ingroup core-examples
 * \ingroup randomvariable
 * Example program illustrating use of ns3::RandomVariableStream
 *
 * This program can be run from waf such as
 * `./waf --run sample-random-variable-stream`
 * 
 * This program is about as simple as possible to display the use of ns-3 
 * to generate random numbers.  By default, the uniform random variate that
 * will be printed is 0.816532.  Because ns-3 uses a fixed seed for the
 * pseudo-random number generator, this program should always output the
 * same number.  Likewise, ns-3 simulations using random variables will
 * behave deterministically unless the user changes the Run number or the
 * Seed.
 * 
 * There are three primary mechanisms to change the seed or run numbers
 * from their default integer value of 1:
 * 
 * 1. Through explicit call of SeedManager::SetSeed () and 
 *    SeedManager::SetRun () (commented out below) 
 * 2. Through the passing of command line arguments such as:
 *    `./waf --command-template="%s --RngRun=<value>" --run program-name`
 * 3. Through the use of the NS_GLOBAL_VALUE environment variable, such as:
 *    `$ NS_GLOBAL_VALUE="RngRun=<value>" ./waf --run program-name`
 * 
 * For instance, setting the run number to 3 will change the program output to
 * 0.775417
 * 
 * Consult the ns-3 manual for more information about the use of the 
 * random number generator
 */

using namespace ns3;

int main (int argc, char *argv[]){
	CommandLine cmd;
	int nSeed=3, nRun=6, num=0;
	double x, y, min=1.0, max=60.0;
 
  	std::string fileNameWithNoExtension = "plot-2d";
  	std::string graphicsFileName        = fileNameWithNoExtension + ".png";
  	std::string plotFileName            = fileNameWithNoExtension + ".plt";
  	std::string plotTitle               = "2-D Plot";
  	std::string dataTitle1               = "2-D Data1";
 	std::string dataTitle2               = "2-D Data2";


	// Instantiate the plot and set its title.
  	Gnuplot plot (graphicsFileName);
  	plot.SetTitle (plotTitle);

  	// Make the graphics file, which the plot file will create when it
  	// is used with Gnuplot, be a PNG file.
  	plot.SetTerminal ("png");

  	// Set the labels for each axis.
  	plot.SetLegend ("X Values", "Y Quadratic Values");

  	// Set the range for the x axis.
  	plot.AppendExtra ("set xrange [-6:+6]");

  	// Instantiate the dataset, set its title, and make the points be
  	// plotted along with connecting lines.
  	Gnuplot2dDataset dataset1;
  	dataset1.SetTitle (dataTitle1);
  	dataset1.SetStyle (Gnuplot2dDataset::LINES_POINTS);

	// create the dataset
	for(x=-5.0;x<=5.0; x++){
			//function
			y=x*x;
			dataset1.Add(x,y);
	}
	
  	// Add the dataset to the plot.
  	plot.AddDataset (dataset1);

	Gnuplot2dDataset dataset2;
	dataset2.SetTitle(dataTitle2);
 	dataset2.SetStyle (Gnuplot2dDataset::LINES_POINTS);

	// create the dataset
	for(x=-5.0;x<=5.0; x++){
			//function
			y=x*x + 2.0*x;
			dataset2.Add(x,y);
	}
	
  	// Add the dataset to the plot.
  	plot.AddDataset (dataset2);

  	// Open the plot file.
  	std::ofstream plotFile (plotFileName.c_str());

  	// Write the plot file.
  	plot.GenerateOutput (plotFile);

  	// Close the plot file.
  	plotFile.close ();

  	cmd.AddValue ("nSeed", "Number of Seed for random variables", nSeed);
	cmd.AddValue ("nRun", "Number of Run for random variables", nRun);
	cmd.Parse (argc, argv);

   	std::cout << "num seed:" << nSeed << std::endl;
	std::cout << "num run:"  << nRun << std::endl;
	
  	RngSeedManager::SetSeed (nSeed);
	RngSeedManager::SetRun (nRun);
  
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
	
	uv->SetAttribute("Min", DoubleValue(min));	
	uv->SetAttribute("Max", DoubleValue(max));	


	for(int i=0; i<20; i++){
		num = uv->GetInteger();
  		std::cout << num << std::endl;
	}	

	return(0);
}
