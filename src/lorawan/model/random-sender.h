/*
 * =====================================================================================
 *
 *       Filename:  random-sender.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  28/01/2019 15:24:12
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Francisco Helder Candido (FHC), helderhdw@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef RANDOM_SENDER_H
#define RANDOM_SENDER_H

#include "ns3/application.h"
#include "ns3/nstime.h"
#include "ns3/lora-mac.h"
#include "ns3/attribute.h"

namespace ns3 {

class RandomSender : public Application {
public:

	RandomSender ();
  	~RandomSender ();

  	static TypeId GetTypeId (void);

   	/**
   	* Set the initial delay of this application
   	*/
  	void SetInitialDelay (Time delay);

  	/**
   	* Send a packet using the LoraNetDevice's Send method
   	*/
  	void SendPacket (void);

  	/**
   	* Set the sending mean
   	* \param mean the mean between two packet sendings
   	*/
  	void SetMean (Time mean);
  	
	/**
   	* Get the sending inteval
   	* \returns the interval between two packet sends
   	*/
  	Time GetMean (void) const;

  	/**
   	* Start the application by scheduling the first SendPacket event
   	*/
  	void StartApplication (void);

  	/**
   	* Stop the application
   	*/
  	void StopApplication (void);

private:
  	/**
   	* The interval between to consecutive send events
   	*/
  	Time m_mInterval;
	
	/**
	* The next time with which the alarm will be set to send messages
	*/
  	Ptr<ExponentialRandomVariable> m_nextDelay;

	/**
   	* The initial delay of this application
   	*/
  	Time m_initialDelay;

  	/**
   	* The sending event scheduled as next
   	*/
  	EventId m_sendEvent;

  	/**
   	* The MAC layer of this node
   	*/
  	Ptr<LoraMac> m_mac;

  	/**
   	* The size of the packets this application sends
   	*/
  	Ptr<RandomVariableStream> m_pktSize;

	/**
	* Whether or not this application uses a random packet size.
   	*/
  	bool m_randomPktSize;
};

} //namespace ns3

#endif /* RANDOM_APPLICATION */

