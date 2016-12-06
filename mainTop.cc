#include "ns3/core-module.h"
#include "ns3/network-module.h" 
#include "ns3/internet-module.h" 
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/animation-interface.h"

using namespace ns3;

double cwnd = 0, ssthresh = 0;
Ptr<OutputStreamWrapper> cwndFile;
Ptr<OutputStreamWrapper> ssthreshFile;
Ptr<OutputStreamWrapper> seqFile;

// Modified to only write "newCwnd" values
static void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
	*stream->GetStream()  <<  Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;
	*ssthreshFile->GetStream() << Simulator::Now().GetSeconds() << "\t" << ssthresh << std::endl;
	cwnd = newCwnd;
}

	
static void ssthreshTrace (Ptr<OutputStreamWrapper> stream, uint32_t oldthresh, uint32_t newthresh)
{
	*cwndFile->GetStream()  <<  Simulator::Now ().GetSeconds () << "\t" << cwnd << std::endl;
	*stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newthresh << std::endl;
	ssthresh = newthresh;
}

/*static void seqTrace (Ptr<OutputStreamWrapper> stream, uint32_t oldseq, uint32_t newseq)
{
	*stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << newseq << std::endl;
}*/

static void TraceCwnd ()
{
  // Trace changes to the congestion window
    AsciiTraceHelper ascii;
    cwndFile = ascii.CreateFileStream ("Cwnd.data");
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback (&CwndChange,cwndFile));
}

static void TraceSSthresh()
{
	//trace the ssthresh values
	AsciiTraceHelper ascii;
    ssthreshFile = ascii.CreateFileStream ("Thresh.data");
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold", MakeBoundCallback (&ssthreshTrace,ssthreshFile));
}

/*static void TraceSequence ()
{
  // Trace changes to the congestion window
    AsciiTraceHelper ascii;
    seqFile = ascii.CreateFileStream ("Sequence.data");
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpSocketBase/NextTXSequence", MakeBoundCallback (&seqTrace,seqFile));
}*/


int main(int argc, char *argv[])
{
  	// Ensure Proper Argument Usage
	if (argc != 1) {
		printf("%d ERROR: Incorrect number of arguements.", argc);
		printf(" Please run using the following format...\n");
		printf("tcpchain\n\n");
		exit(1);
    }
    
    // Assign Arguements

	NS_LOG_COMPONENT_DEFINE ("MainTop");



	std::cout << "REAL STUFFS" << std::endl;
	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

	// Create nodes A,B,C,D
  	NodeContainer nodes, nodes2;
  	nodes.Create (7);

	// Create pointtopoint links with assigned values
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  	// Set Devices
	NetDeviceContainer devAB, devBC, devCD, devAE, devEF, devFG,devGD;
	devAB = pointToPoint.Install (nodes.Get(0), nodes.Get(1));
	devBC = pointToPoint.Install (nodes.Get(1), nodes.Get(2));
	devCD = pointToPoint.Install (nodes.Get(2), nodes.Get(3));
	devAE = pointToPoint.Install (nodes.Get(0), nodes.Get(4));
	devEF = pointToPoint.Install (nodes.Get(4), nodes.Get(5));
	devFG = pointToPoint.Install (nodes.Get(5), nodes.Get(6));
	devGD = pointToPoint.Install (nodes.Get(6), nodes.Get(3));


	// Initialize Stack
	InternetStackHelper stack;
	stack.InstallAll();

	// Initialize and assign Address
	NS_LOG_INFO ("CREATE: IP Address");
	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceAB = address.Assign(devAB);
	address.SetBase ("11.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceBC = address.Assign(devBC);
	address.SetBase ("12.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceCD = address.Assign(devCD);
	address.SetBase ("13.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceAE = address.Assign(devAE);
	address.SetBase ("14.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceEF = address.Assign(devEF);
	address.SetBase ("15.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceFG = address.Assign(devFG);
	address.SetBase ("16.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceGD = address.Assign(devGD);

	// Set Bit Error Rate for A-B, C-D
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.000001));
    devAB.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devAB.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devCD.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devCD.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
	devAE.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devAE.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
	devEF.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devEF.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
	devFG.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devFG.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
	devGD.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devGD.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
    // Set Bit Error Rate for B-C
    em->SetAttribute ("ErrorRate", DoubleValue(0.00005));
    devBC.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devBC.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    // Create and Assign TCP Sink to node D
    uint16_t sinkPort = 8080;
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (3));
    sinkApps.Start (Seconds (1.0));
    sinkApps.Stop (Seconds (30.0));


    
    BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (interfaceCD.GetAddress (1), sinkPort));
    // Set the amount of data to send in bytes.  Zero is unlimited.
	source.SetAttribute("MaxBytes",UintegerValue(1000000));
    ApplicationContainer sourceApps = source.Install (nodes.Get (0));
    sourceApps.Start (Seconds (1.0));
    sourceApps.Stop (Seconds (30.0));

    


	Ptr<Node> b = nodes.Get(1);
	Ptr<Ipv4> ipv4b = b->GetObject<Ipv4>(); 
	Ptr<Node> c = nodes.Get(2);
	Ptr<Ipv4> ipv4c = c->GetObject<Ipv4>(); 
  
	// Create Congestion Window Stream
	Simulator::Schedule(Seconds(0.00001),&TraceCwnd);
	Simulator::Schedule(Seconds(0.00001),&TraceSSthresh);
	Simulator::Schedule (Seconds (2),&Ipv4::SetDown,ipv4b, 2);
	Simulator::Schedule (Seconds (2),&Ipv4::SetDown,ipv4c, 1);
	Simulator::Schedule (Seconds (2),&Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
	//Simulator::Schedule(Seconds(0.00001),&TraceSequence);
	
	
	// Set Routing Tables and Pcap File
	pointToPoint.EnablePcapAll("pcap");

	AnimationInterface anim("simple.tr"); 
	AnimationInterface::SetConstantPosition (nodes.Get(0),  0, 0);
	AnimationInterface::SetConstantPosition (nodes.Get(1), 10, 0);
	AnimationInterface::SetConstantPosition (nodes.Get(2), 20, 0);
	AnimationInterface::SetConstantPosition (nodes.Get(3), 30, 0);
	AnimationInterface::SetConstantPosition (nodes.Get(4),  5, 10);
	AnimationInterface::SetConstantPosition (nodes.Get(5), 15, 10);
	AnimationInterface::SetConstantPosition (nodes.Get(6), 25, 10);

  	
	// Run actual Program
    Simulator::Stop (Seconds (31));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
