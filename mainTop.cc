#include "ns3/core-module.h"
#include "ns3/network-module.h" 
#include "ns3/internet-module.h" 
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

// Copied from sixth.cc 
class MyApp : public Application
{
public:
  	MyApp (); virtual ~MyApp ();

  	/**
   	 * Register this type.
  	 * \return The TypeId.
   	 */
  	static TypeId GetTypeId (void);
  	void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  	virtual void StartApplication (void);
  	virtual void StopApplication (void);

  	void ScheduleTx (void);
  	void SendPacket (void);

  	Ptr<Socket>     m_socket;
  	Address         m_peer;
  	uint32_t        m_packetSize;
  	uint32_t        m_nPackets;
 	 DataRate        m_dataRate;
  	EventId         m_sendEvent;
  	bool            m_running;
  	uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  	m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  	return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  	m_socket = socket;
  	m_peer = address;
 	m_packetSize = packetSize;
	m_nPackets = nPackets;
  	m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  	m_running = true;
  	m_packetsSent = 0;
  	m_socket->Bind ();
  	m_socket->Connect (m_peer);
  	SendPacket ();
}

void
MyApp::StopApplication (void)
{
  	m_running = false;

  	if (m_sendEvent.IsRunning ())
    {
      	Simulator::Cancel (m_sendEvent);
    }

  	if (m_socket)
    {
      	m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  	Ptr<Packet> packet = Create<Packet> (m_packetSize);
  	m_socket->Send (packet);

  	if (++m_packetsSent < m_nPackets)
    {
      	ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  	if (m_running)
    {
		Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
     	 m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

// Modified to only write "newCwnd" values
static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  	NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;
	std::cout << Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;
}

int main (int argc, char *argv[]) {

  	// Ensure Proper Argument Usage
	if (argc != 3) {
		printf("%d ERROR: Incorrect number of arguements.", argc);
		printf(" Please run using the following format...\n");
		printf("tcpchain <pcap-filename> <congestion-output>\n\n");
		exit(1);
    }
    
    // Assign Arguements
	char *pcapName = argv[1];
	char *congest = argv[2];
	
	NS_LOG_COMPONENT_DEFINE ("TCPChain");

	// Create nodes A,B,C,D
  	NodeContainer nodes;
  	nodes.Create (4);

	// Create pointtopoint links with assigned values
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  	// Set Devices
	NetDeviceContainer devAB, devBC, devCD;
	devAB = pointToPoint.Install (nodes.Get(0), nodes.Get(1));
	devBC = pointToPoint.Install (nodes.Get(1), nodes.Get(2));
	devCD = pointToPoint.Install (nodes.Get(2), nodes.Get(3));

	// Initialize Stack
	InternetStackHelper stack;
	stack.Install (nodes);

	// Initialize and assign Address
	NS_LOG_INFO ("CREATE: IP Address");
	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceAB = address.Assign (devAB);
	address.SetBase ("11.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceBC = address.Assign(devBC);
	address.SetBase ("12.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer interfaceCD = address.Assign(devCD);

	// Set Bit Error Rate for A-B, C-D
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.000001));
    devAB.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devAB.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devCD.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devCD.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  
    // Set Bit Error Rate for B-C
    em->SetAttribute ("ErrorRate", DoubleValue(0.00005));
    devBC.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    devBC.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    // Create and Assign TCP Sink to node D
    uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (interfaceCD.GetAddress (1), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (3));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (30.0));

	// Create Socket
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());



    BulkSendHelper source ("ns3::TcpSocketFactory",
    	InetSocketAddress (interfaceCD.GetAddress (1), sinkPort));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    source.SetAttribute ("MaxBytes", UintegerValue (1000000));
    ApplicationContainer sourceApps = source.Install (nodes.Get (0));
    sourceApps.Start (Seconds (0.0));
    sourceApps.Stop (Seconds (30.0));
  
	// Create Congestion Window Stream
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (congest);
    ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));
  
  	// Set Routing Tables and Pcap File
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	pointToPoint.EnablePcapAll(pcapName);

	// Run actual Program
    Simulator::Stop (Seconds (20));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
