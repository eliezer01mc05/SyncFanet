/**
@mainpage IPv4Configurator Tutorial for the INET Framework

This tutorial will show how to use the <tt>IPv4NetworkConfigurator</tt> module to configure IP addresses and routing tables in wired and wireless IPv4 networks in the INET framework.
The tutorial is organized into multiple steps, each corresponding to a simulation model. The steps demonstrate how to accomplish certain
tasks with the configurator.

This is an advanced tutorial, and assumes that the reader is familiar with creating and running simulations in @opp and INET. If that wasn't the case,
the <a href="https://omnetpp.org/doc/omnetpp/tictoc-tutorial/"
target="_blank">TicToc Tutorial</a> is a good starting point to get familiar with @opp. The <a
href="../../../doc/walkthrough/tutorial.html" target="_blank">INET Walkthrough</a> is an introduction to INET and how to work with protocols.
The <a href="https://omnetpp.org/doc/inet/api-current/tutorials/wireless/" target="_blank">Wireless Tutorial</a> is another advanced tutorial, and deals with wireless features of the INET framework. There is a comprehensive description of the configurator's features in the <a href="https://omnetpp.org/doc/inet/api-current/neddoc/index.html?p=inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator.html" target="_blank"><tt>IPv4NetworkConfigurator</tt> NED documentation</a>
in the INET reference.

For additional information, the following documentation should be useful:

- <a href="https://omnetpp.org/doc/omnetpp/manual/usman.html" target="_blank">@opp User Manual</a>
- <a href="https://omnetpp.org/doc/omnetpp/api/index.html" target="_blank">@opp API Reference</a>
- <a href="https://omnetpp.org/doc/inet/api-current/inet-manual-draft.pdf" target="_blank">INET Manual draft</a>
- <a href="https://omnetpp.org/doc/inet/api-current/neddoc/index.html" target="_blank">INET Reference</a>

All simulation models are defined in the omnetpp.ini file as separate configurations. The configurations use a number of different networks,
defined in separate .NED files.

@section contents Contents

 - @ref step1
 - @ref step2
 - @ref step3
 - @ref step4
 - @ref step5
 - @ref step6
 - @ref step7
 - @ref step8
 - @ref step9
 - @ref step10
 - @ref step11
 - @ref step12

@nav{index,step1}

<!-------------------------------------------------------------------------------------------------------->

@page step1 Step 1 - Fully automatic IP address assignment

@section s1goals Goals

The goal of this step is to demonstrate that in many scenarios, the configurator can properly configure IP addresses in a network with its default settings, without
any user input. This is useful when it is irrelevant what the nodes' actual IP addresses are in a simulation
(this step deals with IP addresses only, routing will be discussed in later steps).

@section s1model The model

@subsection s1about  About the configurator

In INET simulations, configurator modules are responsible for assigning IP addresses to network nodes, and for setting up their
routing tables. Essentially, the configurator modules simulate a real life network administrator. There are various configurator
models in INET (<tt>IPv4NetworkConfigurator</tt>, <tt>FlatNetworkConfigurator</tt>, etc.), but this tutorial is about the features of the <tt>IPv4NetworkConfigurator</tt>, 
which we will refer to as <strong>configurator</strong>. The following is a broad review of the configurator's features and operation,
these and others will be discussed in detail in the following steps.

The configurator assigns IP addresses to interfaces, and sets up static routing in IPv4 networks.
It doesn't configure IP addresses and routes directly, but stores the configuration in its internal data structures.
Network nodes contain an instance of <tt>IPv4NodeConfigurator</tt>, which configures the corresponding node's interface table and routing table
based on information contained in the global <tt>IPv4NetworkConfigurator</tt> module.

The configurator supports automatic and manual network configuration, and their combination. By default,
the configuration is fully automatic, but the user can specify parts (or all) of the configuration manually, and the rest
will be configured automatically by the configurator. The configurator's various features can be turned on and off with NED parameters. The details of the configuration, such as IP addresses and routes, can be specified in an XML configuration file.

@subsection s1configuration The configuration

The configuration for this step uses the <i>ConfiguratorA</i> network, defined in <i>ConfiguratorA.ned</i>.
The network looks like this:

<img src="step1network.png">

The network contains 3 routers, each connected to the other two. There are 3 subnetworks with <tt>StandardHosts</tt>, connected to the routers by ethernet switches.
It also contains an instance of <tt>IPv4NetworkConfigurator</tt>.

The configuration for this step in omnetpp.ini is the following: 

@dontinclude omnetpp.uncommented.ini
@skip Step1
@until ####

The configuration for Step 1 is basically empty. The configurator configures addresses according to its default parameters, and using the default XML configuration.

The default parameters pertaining to IP address assignment are the following:

<pre>
assignAddresses = default(true);
assignDisjunctSubnetAddresses = default(true);
</pre>

- <strong>assignAddresses = true</strong> tells the configurator to assign IP addresses to interfaces. It assigns addresses based on the supplied XML configuration,
or the default XML configuration if none is specified. Since no XML configuration is specified in this step, it uses the default configuration.

- <strong>assignDisjunctSubnetAddresses = true</strong> sets that the configurator should assign different address prefixes and netmasks
to nodes on different links (nodes are considered to be on the same link if they can reach each other directly, or through L2 devices only).

Additionally, the <strong>dumpAddresses</strong> parameter sets whether the configurator prints assigned IP addresses to the module output.
This is false by default, but it's set to true in the <i>General</i> configuration at the begining of omnetpp.ini (along with other settings, which
will be discussed later).

@dontinclude omnetpp.uncommented.ini
@skipline General
@until ####

An XML configuration can be supplied with the <i>config</i> parameter. When the user doesn't specify an XML configuration,
the configurator will use the following default configuration:

<div class="fragment">
<pre class="monospace">
config = default(xml("<config><interface hosts='**' address='10.x.x.x' netmask='255.x.x.x'/></config>"))
</pre>
</div>

The default XML configuration tells the configurator to assign IP addresses to all interfaces of all hosts, 
from the IP range 10.0.0.0 - 10.255.255.255 and netmask range 255.0.0.0 - 255.255.255.255.

@section s1results Results

The IP addresses assigned to interfaces by the configurator are shown on the image below.
The switches and hosts connected to the individual routers are considered to be on the same link.
Note that the configurator assigned addresses sequentially starting from 10.0.0.1, while making sure that different subnets got different address prefixes and netmasks,
as instructed by the <strong>assignDisjunctSubnetAddresses</strong> parameter.

<img src="step1addresses.png" width=850px>

Note that the configurator assigned a 29 bit netmask to the hosts and the router interfaces connecting to the switches, and a 30 bit netmask
to the other router interfaces. 3 hosts and the router's interface towards a switch as group has 4 interfaces, thus a 30 bit netmask/2 bit host identifier
would have sufficed. However, the configurator doesn't assign addresses where part of the address not covered by the subnet mask - the host identifier -
is all-zeros or all-ones (indicating subnet zero and the subnet broadcast address).

@nav{index,step2}
@fixupini

<!-------------------------------------------------------------------------------------------------------->

@page step2 Step 2 - Manually overriding individual IP addresses

@nav{step1,step3}

@section s2goals Goals

Sometimes it might be useful to specify the IP addresses of some nodes with special purpose in the network and leave the rest for the automatic configuration. This helps remembering IP addresses of said nodes. This step demonstrates manually specifying individual IP addresses.

@section s2model The model

This step uses the <i>ConfiguratorA</i> network from the previous step. We will assign the 10.0.0.50 address to <i>host1</i>
and 10.0.0.100 to <i>host3</i>. The configurator will automatically assign addresses to the rest of the nodes.

@subsection s2config Configuration

The configuration in omnetpp.ini for this step is the following:

@dontinclude omnetpp.uncommented.ini
@skip Step2
@until ####

The XML configuration can be supplied to the <i>config</i> parameter in one of two ways:

- Inline XML using the <i>xml()</i> function. The argument of the function is the XML code.
- External XML file using the <i>XMLdoc()</i> function. The argument of the function is the name of the XML configuration file.

In this step, the XML configuration is supplied to the configurator as inline XML. XML configurations contain one <i><config></i> element. Under this root element there can be
multiple configuration elements, such as the <i><interface></i> elements here.
The <interface> element (and other elements) can contain selector attributes, which limit the scope of what interfaces are affected by the <interface> element.
Multiple interfaces can be selected with one <interface> element using the * wildcard.
They can also contain parameter attributes, which deal with what parameters those selected interfaces will have, like IP addresses and
netmasks. Address templates can be specified with one or more 'x' in the address. The 'x' in the IP address and netmask signify that the value is not fixed, but the configurator should choose it automatically.
With these address templates it is possible to leave everything to the configurator or specify everything, and anything in between.
- The <strong>hosts</strong> selector attribute selects hosts. The selection pattern can be full path (e.g. "*.host0") or a module name anywhere in the hierarchy (e.g. "host0"). Only interfaces in the selected host will be affected by the <interface> element.
- The <strong>names</strong> selector attribute selects interfaces. Only the interfaces that match the specified names will be selected (e.g. "eth0").
- The <strong>address</strong> parameter attribute specifies the addresses to be assigned. Address templates can be used, where an 'x' in place of an octet means that the value
should be selected by the configurator automatically. The value "" means that the no address will be assigned. Unconfigured interfaces will still have
allocated addresses in their subnets, so they can be easily configured later dynamically.
- The <strong>netmask</strong> parameter attribute specifies the netmasks to be assigned. Address templetes can be used here as well.

All attributes are optional. Attributes not specified are left for the automatic configuration. There are many other attributes available. For the complete list of attributes of the <interface> element
(or any other element), please refer to the <a href="https://omnetpp.org/doc/inet/api-current/neddoc/index.html?p=inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator.html" target="_blank"><tt>IPv4NetworkConfigurator</tt></a> NED documentation.

In the XML configuration for this step, the first two rules state that <i>host3's</i> (hosts="*.host3") interface named 'eth0' (names="eth0") should get the IP address 10.0.0.100 (address="10.0.0.100"), and <i>host1's</i> interface 'eth0' should get 10.0.0.50.
The third rule is the exact copy of the default configuration, which tells the configurator to assign the rest of the addresses automatically.
Note that this is the default rule in two contexts. It is the default rule that the configurator uses when no XML config is specified. Also it is
the last and least specific among the address assignment rules here, thus it takes effect for interfaces that don't match the previous rules.

Note that the order of configuration elements is important, but the configurator doesn't assign addresses in the order of XML interface elements. It iterates
interfaces in the network, and for each interface the first matching rule in the XML configuration will take effect. Thus, the statements that are positioned earlier in the configuration take precedence over those that come later.

When an XML configuration is supplied, it must contain interface elements in order to assign addresses at all. To make sure the configurator automatically assigns addresses to all interfaces, a rule similar to the one in the default configuration has to be included. Unless the intention is to leave some interfaces unassigned. The default rule should be the <strong>last</strong> one among the interface rules (so the more specific ones override it).

@section s2results Results

The assigned addresses are shown in the following image.

<img src="step2address.png" width=850px>

As in the previous step, the configurator assigned disjunct subnet addresses. Note that the configurator still assigned addresses sequentially,
that is after setting the 10.0.0.100 address to <i>host3</i>, it didn't go back to the beginning of the address pool when assigning the
remaining addresses.

@nav{step1,step3}
@fixupini

<!-------------------------------------------------------------------------------------------------------->

@page step3 Step 3 - Automatically assigning IP addresses to a subnet from a given range

@nav{step2,step4}

@section s3goals Goals

Complex networks often contain several subnetworks, and the user may want to assign specific IP address ranges for them.
This can make it easier to tell them apart when looking at the IP addresses.
This step demonstrates how to assign a range of IP addresses to subnets.

@section s3model The model

This step uses the <i>ConfiguratorA</i> network, as in the previous two steps.
One switch and the connected hosts as a group will be one the same subnet, and there are 3 such groups in the network.


The configuration is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step3
@until ####

This time the XML configuration is supplied in an external file (step3.xml), using the xmldoc function:

@include step3.xml

- The first 3 lines assign IP addresses with different network prefixes to hosts in the 3 different subnets.

- The <i>towards</i> selector can be used to easily select the interfaces that are connected towards a certain host (or set of hosts using wildcards).
The next 3 entries specify that each router's interface that connects to the subnet should belong in that subnet.

- The last entry sets the network prefix of interfaces of all routers to be 10.1.x.x. 
The routers' interfaces facing the subnets were assigned addresses by the previous rules, so this rule only effects the interfaces facing the other
routers. These 7 rules assign addresses to all interfaces in the network, thus a default rule is not required.

The same effect can be achieved in more than one way. Here is an alternative XML configuration (step3alt1.xml) that results in the same address assignment:

@include step3alt1.xml

The <i>among</i> selector selects the interfaces of the specified hosts towards the specified hosts (the statement <i>among="X Y Z"</i> is the same as
<i>hosts="X Y Z" towards="X Y Z"</i>).

Another alternative XML configuration is the following:

@include step3alt2.xml

This assigns an address to one host in each of the 3 subnets. It assigns addresses to the interfaces of the routers facing the other routers, and includes a copy of the default
configuration. Because <i>assignDisjunctSubnetAddresses=true</i>, the configurator puts the unspecified hosts, and the subnet facing
router interfaces into the same subnet as the specified host.

@section s3results Results

The assigned addresses are shown on the following image.

<img src="step3address.png" width=850px>

The addresses are assigned as intended.
This is useful because it is easy to recognize which group a node belongs to just by looking at its address (e.g. in the logs).

@nav{step2,step4}
@fixupini

<!-------------------------------------------------------------------------------------------------------->

@page step4 Step 4 - Fully automatic static routing table configuration

@nav{step3,step5}

@section s4goals Goals

Just as with IP addresses, in many cases the configurator sets up routes in a network properly without any user input.
This step demonstrates the default configuration for routing.

@section s4model The model

This step uses the <i>ConfiguratorB</i> network, defined in ConfiguratorB.ned. This extends the previous network, <i>ConfiguratorA</i>, with
the addition of a <tt>RoutingTableCanvasVisualizer</tt> module.

@dontinclude ConfiguratorB.ned
@skip ConfiguratorB
@until ####

<img src="step4network.png">

@subsection s4config Configuration

The configuration for this step in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skip Step4
@until ####

A ping app in <i>host1</i> is configured to send ping packets to <i>host7</i>, which is on the other side of the network.
This application is suitable for visualizing routing tables.

The <i>RoutingTableCanvasVisualizer</i> module can be used to visualize routes in the network.
Routes are visualized with arrows. In general, an arrow indicates an entry in the source host's routing table. It points
to the host that is the next hop or gateway for that routing table entry.
Routes to be visualized are selected with the visualizer's <i>destinationFilter</i> parameter.
All routes leading towards that destination are indicated by arrows.
The default setting is <tt>""</tt>, which means no routes are visualized. The <tt>"**"</tt> setting visualizes all routes
going from every node to every other node, which can make the screen cluttered.
In this step the <i>destinationFilter</i> is set to visualize all routes heading towards <i>host7</i>.

The IP address assignment is fully automatic, and the resulting addresses should be the same as in Step 1.

@subsection s4defaults Configurator routing parameters

The configurator's default parameters concerning static routing are the following:

<pre>
addStaticRoutes = default(true)
addDefaultRoutes = default(true)
addSubnetRoutes = default(true)
optimizeRoutes = default(true)
</pre>

The configuration for this step didn't set any of these parameters, thus the default values will take effect.

- <i>addStaticRoutes</i>: Add static routes to the routing table of all nodes in the network, 
with routes leading to all destination interfaces.
- <i>addDefaultRoutes</i>: Add a default route if all routes from a node go through the same gateway.
This is often the case with hosts, which usually connect to a network via a single interface. This parameter
is not used if <i>addStaticRoutes = false</i>.
- <i>addSubnetRoutes</i>: Optimize routing tables by adding routes towards subnets instead of individial interfaces. 
This is only used where applicable, and not used if <i>addStaticRoutes = false</i>.
- <i>optimizeRoutes</i>: Optimize routing tables by merging entries where possible. Not used if <i>addStaticRoutes = false</i>.

Additionally, the <i>dumpTopology</i>, <i>dumpLinks</i> and <i>dumpRoutes</i> parameters are set to true in the <i>General</i> configuration.
These instruct the configurator to print to the module output the topology of the network, the recognized network links, and the routing tables of all nodes, respectively. Topology describes which nodes are connected to which nodes. Hosts that can directly reach each other (i.e. the next hop is the destination), 
are considered to be on the same link.

@dontinclude omnetpp.uncommented.ini
@skip General
@until ####

The <i>General</i> configuration also sets GlobalARP to keep the packet exchanges simpler. GlobalARP fills the ARP tables of all nodes in advance,
so when the simulation begins no ARP exchanges are necessary. The <i>**.routingTable.netmaskRoutes = ""</i> keeps the routing table modules from
adding netmask routes to the routing tables. Netmask routes mean that nodes with the same netmask but different IP should reach each other directly.
These routes are also added by the configurator, so netmaskRoutes is turned off to avoid duplicate routes.

@section s4result Results

The visualized routes are displayed on the following image:

<img src="step4routes.png" width=850px>

Routes from all nodes to <i>host7</i> are visualized. Note that arrows don't go through switches,
because they are not the next hop. When routes are concerned, they are transparent L2 devices.

The routing tables are the following (routes visualized on the image above are highlighted):

@htmlonly
<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorB.host0 (hosts 1-2 similar)
-- Routing table --
Destination      Netmask          Gateway          Iface           Metric
10.0.0.0         255.255.255.248  *                eth0 (10.0.0.1) 0
<span class="marker">*                *                10.0.0.4         eth0 (10.0.0.1) 0</span>

Node ConfiguratorB.host3 (hosts 4-5 similar)
-- Routing table --
Destination      Netmask          Gateway          Iface           Metric
10.0.0.8         255.255.255.248  *                eth0 (10.0.0.9) 0
<span class="marker">*                *                10.0.0.10        eth0 (10.0.0.9) 0</span>

Node ConfiguratorB.host6 (hosts 7-8 similar)
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
<span class="marker">10.0.0.32        255.255.255.248  *                eth0 (10.0.0.34) 0</span>
<i></i>*                *                10.0.0.33        eth0 (10.0.0.34) 0

Node ConfiguratorB.router0
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.18        255.255.255.255  *                eth1 (10.0.0.17) 0
10.0.0.22        255.255.255.255  *                eth2 (10.0.0.21) 0
10.0.0.25        255.255.255.255  10.0.0.22        eth2 (10.0.0.21) 0
10.0.0.0         255.255.255.248  *                eth0 (10.0.0.4)  0
<span class="marker">10.0.0.32        255.255.255.248  10.0.0.22        eth2 (10.0.0.21) 0</span>
10.0.0.0         255.255.255.224  10.0.0.18        eth1 (10.0.0.17) 0

Node ConfiguratorB.router1
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.17        255.255.255.255  *                eth0 (10.0.0.18) 0
10.0.0.22        255.255.255.255  10.0.0.25        eth2 (10.0.0.26) 0
10.0.0.25        255.255.255.255  *                eth2 (10.0.0.26) 0
10.0.0.8         255.255.255.248  *                eth1 (10.0.0.10) 0
<span class="marker">10.0.0.32        255.255.255.248  10.0.0.25        eth2 (10.0.0.26) 0</span>
10.0.0.0         255.255.255.224  10.0.0.17        eth0 (10.0.0.18) 0

Node ConfiguratorB.router2
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.18        255.255.255.255  10.0.0.26        eth0 (10.0.0.25) 0
10.0.0.21        255.255.255.255  *                eth2 (10.0.0.22) 0
10.0.0.26        255.255.255.255  *                eth0 (10.0.0.25) 0
10.0.0.8         255.255.255.248  10.0.0.26        eth0 (10.0.0.25) 0
<span class="marker">10.0.0.32        255.255.255.248  *                eth1 (10.0.0.33) 0</span>
10.0.0.0         255.255.255.224  10.0.0.21        eth2 (10.0.0.22) 0
</pre>
</div>
@endhtmlonly

The * for the gateway means that the gateway is the same as the destination. Hosts have a routing table entry to reach other nodes on the same subnet directly. They also have a default route with the router as
the gateway for packets sent to outside-of-subnet addresses. Routers have 3 rules in their routing tables for reaching the other routers,
specifically, those interfaces of the other routers that are not facing the hosts.

Below is an animation of <i>host1</i> pinging <i>host7</i>.

<img src="step4_new_4.gif" width="850px">

@nav{step3,step5}
@fixupini

<!-------------------------------------------------------------------------------------------------------->

@page step5 Step 5 - Manually overriding individual routes

@nav{step4,step6}

@section s5goals Goals

The automatic configuration can configure routes properly, but sometimes the user might want to manually override some of the routes.
This step has two parts:

- In the <strong>A</strong> part we will override the routes to just one specific host
- In the <strong>B</strong> part we will override routes to a set of hosts

@section s5a Part A - Overriding routes to a specific host

In this part we will override the routes going from the subnet of <i>router0</i> to <i>host7</i>. With the automatic configuration, packets
from <i>router0's</i> subnet would go through <i>router2</i> to reach <i>host7</i> (as in the previous step). We want them to go through <i>router1</i> instead.

<img src="step4network.png">

@subsection s5aconfig Configuration

This configuration uses the same network as the previous step, ConfiguratorB. The configuration in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step5
@until ####

A ping application is added to <i>host0</i>, in addition to the one in <i>host1</i> added in Step 4.
The new app in <i>host0</i> pings <i>host6</i>, this can demonstrate that only packets sent to <i>host7</i>
are affected by the route override.

For the routes to go through <i>router1</i>, the routing table of <i>router0</i> has to be altered.
The new rules should dictate that packets with the destination of <i>host7</i> (10.0.0.35) should be routed
towards <i>router2</i>. The XML configuration in step5a.xml is the following:

@dontinclude step5a.xml
@skipline <config>
@until </config>

The <route> element describes a routing table entry for one or more nodes in the network.
The <i>hosts</i> optional selector attribute specifies which hosts' routing tables should contain the entry.
There are 5 additional optional parameter attributes. These are the same as in real life routing tables:
address, netmask, gateway, interface, metric.

The <route> element in this XML configuration adds the following rule to <i>router0's</i> routing table:
Packets with the destination of 10.0.0.35/32 should use the interface 'eth1' and the gateway 10.0.0.18 (router2).

@subsection s5aresults Results

The routing table of <i>router0</i> (manually added route highlighted):

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorB.router0
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.18        255.255.255.255  *                eth1 (10.0.0.17) 0
10.0.0.22        255.255.255.255  *                eth2 (10.0.0.21) 0
10.0.0.25        255.255.255.255  10.0.0.22        eth2 (10.0.0.21) 0
<span class="marker">10.0.0.35        255.255.255.255  10.0.0.18        eth1 (10.0.0.17) 0</span>
10.0.0.0         255.255.255.248  *                eth0 (10.0.0.4) 	0
10.0.0.32        255.255.255.248  10.0.0.22        eth2 (10.0.0.21) 0
10.0.0.0         255.255.255.224  10.0.0.18        eth1 (10.0.0.17) 0
</pre>
</div>

The routing table of <i>router0</i> in the last step had 6 entries. Now it has 7,
as the rule specified in the XML configuration has been added (highlighted).
This and the last rule both match packets to <i>host7</i> but the manually added route takes effect
because it comes earlier.

The following animation depicts <i>host1</i> pinging <i>host7</i>, and <i>host0</i> pinging <i>host6</i>. Routes to <i>host7</i> are visualized.

<img src="step5a.gif" width="850px">

Note that only routes towards <i>host7</i> are diverted at <i>router0</i>. The ping reply packet uses the original route between <i>router0</i> and <i>router2</i>.
Ping packets to <i>host6</i> (and back) also use the original route.

@section s5b Part B - Overriding routes to a set of hosts

In this part, we will override routes going from the subnet of <i>hosts 0-2</i> to the subnet of <i>hosts 6-8</i>.
These routes will go through <i>router1</i>, just as in Part A.

@subsection s5bconfig Configuration

The configuration in omnetpp.ini:

@dontinclude omnetpp.uncommented.ini
@skipline Step5B
@until ####

As in Part A, the routing table of <i>router0</i> has to be altered, so that packets to <i>hosts 6-8</i> go towards <i>router1</i>. 
The XML configuration in step5b.xml:

@dontinclude step5b.xml
@skipline config
@until config

The <route> element specifies a routing table entry for <i>router0</i>. The destination is 10.0.0.32 with netmask 255.255.255.248,
which designates the addresses of hosts 6-8. The gateway is <i>router1's</i> address, the interface is the one connected towards
<i>router1</i> (eth1). This rule is added to <i>router0's</i> routing table <strong>in addition</strong>
to the rule added automatically by the configurator. They match the same packets, but the parameters are different (see at the result section
below). The manually added routes come before the automatic ones in routing tables, thus the manual ones take precedence.

@subsection s5bresults Results

The routing table of <i>router0</i> (the manually added route highlighted):

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorB.router0
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.10        255.255.255.255  10.0.0.18        eth1 (10.0.0.17) 0
10.0.0.18        255.255.255.255  *                eth1 (10.0.0.17) 0
10.0.0.22        255.255.255.255  *                eth2 (10.0.0.21) 0
10.0.0.25        255.255.255.255  10.0.0.22        eth2 (10.0.0.21) 0
10.0.0.26        255.255.255.255  10.0.0.18        eth1 (10.0.0.17) 0
10.0.0.33        255.255.255.255  10.0.0.22        eth2 (10.0.0.21) 0
10.0.0.0         255.255.255.248  *                eth0 (10.0.0.4)  0
10.0.0.8         255.255.255.248  10.0.0.18        eth1 (10.0.0.17) 0
<span class="marker">10.0.0.32        255.255.255.248  10.0.0.18        eth1 (10.0.0.17) 0</span>
10.0.0.32        255.255.255.248  10.0.0.22        eth2 (10.0.0.21) 0
</pre>
</div>

The following is the animation of <i>host1</i> pinging <i>host7</i> and <i>host0</i> pinging <i>host6</i>, similarly
to Part A. Routes to <i>host7</i> are visualized.

<img src="step5b.gif" width="850px">

This time both packets outbound to <i>hosts 6 and 7</i> take the diverted route, the replies come back on the original route.

@nav{step4,step6}
@fixupini
@keyword


<!-------------------------------------------------------------------------------------------------------->

@page step6 Step 6 - Setting different metric for automatic routing table configuration

@nav{step5,step7}

@section s6goals Goals

When setting up routes, the configurator uses the shortest path algorithm. By default, paths are optimized for hop count.
However, there are other cost functions available, like data rate, error rate, etc. This step has two parts:
- <strong>Part A</strong> demonstrates using the data rate metric for automatically setting up routes.
- <strong>Part B</strong> demonstrates instructing the configurator not to use a link when setting up routes by manually specifying link cost.

@section s6a Part A: Using the data rate metric

When setting up routes, the configurator first builds a graph representing the network topology. 
A vertex in the graph represents a network node along with all of its interfaces. An edge represents a wired or wireless connection
between two network interfaces. When building the network topology, wireless nodes are considered to be connected to all other wireless
nodes in the same wireless network.

After the graph is built, the configurator assigns weights to vertices and edges according to the configured metric.
Vertices that represent network nodes with IP forwarding turned off have infinite weight, all others have 0.
At last the shortest path algorithm is used to determine the routes based on the assigned weights.

The available metrics are the following:
- <strong>hopCount</strong>: routes are optimized for hop count. All edges have a cost of 1. This is the default metric.
- <strong>dataRate</strong>: routes prefer connections with higher bandwidth. Edge costs are inversely proportional to the data rate of the connection.
- <strong>delay</strong>: routes are optimized for lower delay. Edge costs are proportional to the delay of the connection.
- <strong>errorRate</strong>: routes are optimized for smaller error rate. Edge costs are proportional to the error rate of the connection. This is mostly useful
for wireless networks because the error rate of wired connections is usually negligible.

@subsection s6aconfig Configuration

The configuration for this step extends Step 4, thus it uses the ConfiguratorB network. The configuration in omnetpp.ini is the following:
<!TODO: network image?>

@dontinclude omnetpp.uncommented.ini
@skipline Step6A
@until ####

Below is the XML configuration in step6a.xml:

@dontinclude step6a.xml
@skipline config
@until config

The XML configuration contains the default rule for IP address assignment, and an <autoroute> element that configures the metric to be used.
The <autoroute> element specifies parameters for automatic static routing table configuration. If no <autoroute> element is specified, the configurator
assumes a default that affects all routing tables in the network, and computes shortest paths to all interfaces according to the hop count metric.
The <autoroute> element can contain the following attributes:
- <strong>sourceHosts</strong>: Selector attribute that selects which hosts' routing tables should be modified. The default value is <tt>"**"</tt>.
- <strong>destinationInterfaces</strong>: Parameter attribute that selects destination interfaces for which the shortest paths will be calculated.
The default value is <tt>"**"</tt>.
- <strong>metric</strong>: Parameter attribute that sets the metric to be used when calculating shortest paths. The default value is <tt>"hopCount"</tt>.

There are subelements available in <autoroute>, which are discussed in Part B.

Here the <autoroute> element specifies that routes should be added to the routing table of each host and the metric should be <i>dataRate</i>. The configurator assigns weights to the graph's edges that are inversely proportional to the data rate of the network links.
This way route generation will favor routes with higher data rates.

Note that <i>router0</i> and <i>router2</i> are connected with a 10 Mbit/s ethernet cable, while <i>router1</i> connects to the other routers with
100 Mbit/s ethernet cables. Since routes are optimized for data rate, packets from <i>router0</i> to <i>router2</i> will go via <i>router1</i> because this path has higher bandwidth.

<img src="step4routes_3.png">

@subsection s6results Results

The following image shows the backward routes towards <i>host1</i>.
The resulting routes are similar to the ones in Step 5B. The difference is that routes going backward, from <i>hosts 6-8</i> to <i>hosts 0-2</i>, go through <i>router1</i>. No traffic is routed between <i>router0</i> and <i>router2</i> at all (as opposed to Step 4 and 5).

<img src="step6aroutes.png" width="850px">

The routing table of <i>router0</i> is as follows:

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorB.router0
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.18        255.255.255.255  *                eth1 (10.0.0.17) 0
10.0.0.0         255.255.255.248  *                eth0 (10.0.0.4) 	0
10.0.0.0         255.255.255.192  10.0.0.18        eth1 (10.0.0.17) 0
</pre>
</div>

The first 2 rules describe reaching <i>router1</i> and <i>hosts 0-2</i> directly. The last rule specifies that traffic to any other destination
should be routed towards <i>router1</i>.

The routing table of <i>router2</i> is similar:

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorB.router2
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.26        255.255.255.255  *                eth0 (10.0.0.25) 0
10.0.0.32        255.255.255.248  *                eth1 (10.0.0.33) 0
10.0.0.0         255.255.255.224  10.0.0.26        eth0 (10.0.0.25) 0
</pre>
</div>

The following animation shows <i>host1</i> pinging <i>host7</i> and <i>host0</i> pinging <i>host6</i>. Routes towards <i>host1</i> are visualized.
The packets don't use the link between <i>router0</i> and <i>router2</i>.
<img src="step6a.gif" width="850px">

One can easily check that no routes go through the link between <i>router0</i> and <i>router2</i> by setting the destination filter to "*.*" in the visualizer.
This indicates all routes in the network:

<img src="step6allroutes.png" width=850px>

@endhtmlonly

@section s6b Part B - Manually specifying link cost

This part configures the same routes as Part A, where routes between <i>router0</i> and <i>router2</i> lead through <i>router1</i>.

The configurator is instructed not to use the link between <i>router0</i> and <i>router2</i> when setting up routes, by specifying the cost of the link to
be infinite.

@subsection s6bconfig Configuration

The configuration for this step in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step6B
@until ####

The XML configuration in step6b.xml is as follows:

@dontinclude step6b.xml
@skipline config
@until config

The <autoroute> elements can also contain the following optional subelements, which can be used to specify costs in the graph:
- <strong><node></strong>: Specifies cost parameters to network nodes. The <strong>hosts</strong> selector
attribute selects which hosts are affected, and the <strong>cost</strong> parameter sets a number for their costs. Both attributes are mandatory.
- <strong><link></strong>: Specifies cost parameters to network links. The <strong>interfaces</strong> selector
attribute selects which links are affected, by specifying an interface they belong to. The <strong>cost</strong> parameter
sets the cost. Both attributes are mandatory.

This XML configuration specifies the metric to be hop count, and sets the cost of <i>router0's</i> eth2 interface to infinite.
This affects the link between <i>router0</i> and <i>router2</i>, no routes should go through it.

@subsection s6bresults Results

The routes towards <i>host1</i> are visualized in the following image:

<img src="step6broutes.png" width="850px">

The routes are the same as in Part A. In that part the data rate metric was used, routes didn't use the 10Mbps link between <i>router0</i> and <i>router2</i>.
In this part, that link is "turned off" by specifying an infinite cost for it.

<img src="step6a.gif" width="850px">

@nav{step5,step7}
@fixupini
@keyword

<!-------------------------------------------------------------------------------------------------------->

@page step7 Step 7 - Configuring a hierarchical network

@nav{step6,step8}

@section s7goals Goals

In complex hierarchical networks, routing tables can grow very big. This step demonstrates ways the configurator can reduce the size of
routing tables by optimization and the use of hierarchically assigned addresses. The step contains 3 parts:
- <strong>Part A</strong>: Automatically assigned addresses, no optimization
- <strong>Part B</strong>: Automatically assigned addresses, using optimization
- <strong>Part C</strong>: Hierarchically assigned addresses, using optimization

@section s7a Part A - Automatically assigned addresses

Assigning addresses hierarchially in a network with hierarchical topology can reduce the size of routing tables. However, the configurator's automatic address assignment with its default settings doesn't assign addresses hierarchically. This part uses automatic address assignment,
and the configurator's routing table optimization features are turned off. The size of routing tables in this part can serve as a baseline to compare with.

@subsection s7aconfig Configuration

All 3 parts in this step use the ConfiguratorC network defined in ConfiguratorC.ned. The network looks like this:

<img src="step7network.png" width="850px">

The network is comprised of 3 areas, each containing 2 local area networks (LANs). Each LAN contains 3 <tt>standardHosts</tt>. The hosts in the LAN connect to an area router
through switches. The 3 area routers connect to a central backbone router. The network contains 3 hierarchical levels, which correspond to the hosts in the LANs, the area
routers, and the backbone router.

The configuration for this part in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step7A
@until ####

The configuration turns off every kind of optimization relating to address assignment and route generation.
This means that nodes will have an individual routing table entry to every destination interface.

@subsection s7aresults Results

The assigned addresses are shown on the image below:
@htmlonly
<center><a href="step7a.png" data-lightbox="step7a"><img src="step7a.png" width="850px"></a></center>
@endhtmlonly

The size of some of the routing tables are the following:

<img src="step7a_rt.png">

The routing tables of a host (<i>area1lan2host2</i>) and a router (<i>area1router</i>) are shown below.
The <i>backbonerouter's</i> routing table is similar to <i>area1router's</i>.

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorC.area1lan2host2
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.1         255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.2         255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.3         255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.4         255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.9         255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.10        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.11        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.12        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.17        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.18        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.19        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.20        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.25        255.255.255.255  *                eth0 (10.0.0.26) 0
10.0.0.27        255.255.255.255  *                eth0 (10.0.0.26) 0
10.0.0.28        255.255.255.255  *                eth0 (10.0.0.26) 0
10.0.0.33        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.34        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.35        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.36        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.41        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.42        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.43        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.44        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.49        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.50        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.53        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.54        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.57        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0
10.0.0.58        255.255.255.255  10.0.0.28        eth0 (10.0.0.26) 0

Node ConfiguratorC.area1router
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.1         255.255.255.255  *                eth0 (10.0.0.4)  0
10.0.0.2         255.255.255.255  *                eth0 (10.0.0.4)  0
10.0.0.3         255.255.255.255  *                eth0 (10.0.0.4)  0
10.0.0.9         255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.10        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.11        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.12        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.17        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.18        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.19        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.20        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.25        255.255.255.255  *                eth1 (10.0.0.28) 0
10.0.0.26        255.255.255.255  *                eth1 (10.0.0.28) 0
10.0.0.27        255.255.255.255  *                eth1 (10.0.0.28) 0
10.0.0.33        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.34        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.35        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.36        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.41        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.42        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.43        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.44        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.50        255.255.255.255  *                eth2 (10.0.0.49) 0
10.0.0.53        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.54        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.57        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
10.0.0.58        255.255.255.255  10.0.0.50        eth2 (10.0.0.49) 0
</pre>
</div>

There are 30 interfaces in the network (18 hosts * 1 interface + 4 routers * 3 interfaces).
All routing table entries have 255.255.255.255 netmasks, i.e. separate routes to all destination interfaces.
Thus hosts have 29 entries in their routing tables, for the 29 other interfaces. Similarly, routes have 27 entries.

@section s7b Part B - Automatically assigned addresses, using optimization

In this part, we turn on the optimization features of the configurator that were turned off in Part A.
This should optimize routing tables and decrease table size.

@subsection s7bconfig Configuration

The configuration for this part in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step7B
@until ####

The configuration is empty, the default ned parameter values take effect. That means that the following optimization features are turned on:
assignDisjunctSubnetAddresses, addDefaultRoutes, addSubnetRoutes, optimizeRoutes.

@subsection s7bresults Results

The addresses are the same, but the routing table sizes have gone down:

<img src="step7b_rt.png">

The routing tables of a host, an area router and the backbone router are the following:

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorC.area1lan1host0
-- Routing table --
Destination      Netmask          Gateway          Iface           Metric
10.0.0.0         255.255.255.248  *                eth0 (10.0.0.1) 0
<i></i>*                *                10.0.0.4         eth0 (10.0.0.1) 0

Node ConfiguratorC.area1router
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.50        255.255.255.255  *                eth2 (10.0.0.49) 0
10.0.0.0         255.255.255.248  *                eth0 (10.0.0.4) 	0
10.0.0.24        255.255.255.248  *                eth1 (10.0.0.28) 0
10.0.0.0         255.255.255.192  10.0.0.50        eth2 (10.0.0.49) 0

Node ConfiguratorC.backbonerouter
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.0.0.49        255.255.255.255  *                eth0 (10.0.0.50) 0
10.0.0.53        255.255.255.255  *                eth2 (10.0.0.54) 0
10.0.0.57        255.255.255.255  *                eth1 (10.0.0.58) 0
10.0.0.8         255.255.255.248  10.0.0.53        eth2 (10.0.0.54) 0
10.0.0.16        255.255.255.248  10.0.0.57        eth1 (10.0.0.58) 0
10.0.0.32        255.255.255.248  10.0.0.53        eth2 (10.0.0.54) 0
10.0.0.40        255.255.255.248  10.0.0.57        eth1 (10.0.0.58) 0
10.0.0.0         255.255.255.224  10.0.0.49        eth0 (10.0.0.50) 0
10.0.0.0         255.255.255.192  10.0.0.53        eth2 (10.0.0.54) 0
10.0.0.0         255.255.255.192  10.0.0.57        eth1 (10.0.0.58) 0
</pre>
</div>

- Hosts have just 2 routing table entries. One for reaching other hosts in their LANs, and a default route. 
- The area routers have a rule
for reaching the backbone router, 2 rules for reaching the 2 LANs it's connected to, and a default rule for reaching the rest of the network
through the backbone router. 
- Similarly, the backbone router has 3 rules for reaching the 3 area routers, and 6 rules for reaching the 6 LANs
in the network.
- The backbone router has separate rules for the 2 LANs connected to an area router, because the addresses are not contiguously assigned to the 2 LANs
(e.g. <i>area2lan1</i> has address 10.0.0.8/29, <i>area2lan2</i> has 10.0.0.32/29. But <i>area3lan1</i> has 10.0.0.16/29, which is between the two former address ranges). Thus area2 cannot be covered by a single rule.

@section Part C - Hierarchically assigned addresses, optimized routing tables

Having hierarchically assigned addresses in a network results in smaller routing table sizes,
because a large distant network can be covered with just one rule in a core router's routing table.

@subsection s7cconfig Configuration

The configuration for this part in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step7C
@until ####

As in the previous part, all of the configurator's routing table optimization features are enabled.
The XML configuration for this part in step7c.xml is the following:

@include step7c.xml

This XML configuration assigns addresses hierarchically in the following way, when looking down the hierarchy from the backbone router towards the hosts:
- The first octet of the address for all nodes is 10, i.e. 10.x.x.x
- The second octet denotes the area, e.g. 10.2.x.x corresponds to <i>area2</i>
- The third octet denotes the LAN within the area, e.g. 10.2.1.x corresponds to 
<i>lan1</i> in <i>area2</i>
- The forth octet is the host identifier within a LAN, e.g. 10.2.1.4 corresponds to 
<i>host4</i> in <i>lan1</i> in <i>area2</i>

With this setup, it's possible to cover an area with just one rule in the routing table
of the backbone router. Similarly, the area routers need 2 rules for each LAN that they
are connected to.

@subsection s7cresults Results

The image below shows the assigned addresses.

@htmlonly
<center><a href="step7caddresses.png" data-lightbox="step7caddresses"><img src="step7caddresses.png" width="850px"></a></center>
@endhtmlonly

The sizes of some of the routing tables are displayed in the following image.

<img src="step7c_rt.png">

The routing tables are the following:

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorC.area1lan1host0
-- Routing table --
Destination      Netmask          Gateway          Iface           Metric
10.1.1.0         255.255.255.248  *                eth0 (10.1.1.1) 0
<i></i>*                *                10.1.1.4         eth0 (10.1.1.1) 0

Node ConfiguratorC.area1router
-- Routing table --
Destination      Netmask          Gateway          Iface           Metric
10.1.3.1         255.255.255.255  *                eth2 (10.1.3.2) 0
10.1.1.0         255.255.255.248  *                eth0 (10.1.1.4) 0
10.1.2.0         255.255.255.248  *                eth1 (10.1.2.4) 0
10.2.0.0         255.254.0.0      10.1.3.1         eth2 (10.1.3.2) 0

Node ConfiguratorC.backbonerouter
-- Routing table --
Destination      Netmask          Gateway          Iface           Metric
10.1.3.2         255.255.255.255  *                eth0 (10.1.3.1) 0
10.2.3.2         255.255.255.255  *                eth2 (10.2.3.1) 0
10.3.3.2         255.255.255.255  *                eth1 (10.3.3.1) 0
10.1.0.0         255.255.252.0    10.1.3.2         eth0 (10.1.3.1) 0
10.2.0.0         255.255.252.0    10.2.3.2         eth2 (10.2.3.1) 0
10.3.0.0         255.255.252.0    10.3.3.2         eth1 (10.3.3.1) 0
</pre>
</div>

- Hosts' routing tables contain just 2 rules, as in the previous part. One is for reaching the other members of the host's LAN, and a default rule for reaching everything
else through the area's router.

- The area routers' routing tables contain a specific rule for reaching the backbone router, 2 rules for reaching the 2 LANs that belong to the router's area,
and a default rule for reaching everything else throught the backbone router.

- The backbone router's routing table contains 3 specific rules for reaching the 3 area routers, and 3 rules to reach the 3 areas.

The difference between the configuration for this part and the previous one is that addresses are assigned hierechically in this part. The routing
table of the backbone router contains 6 entries instead of 10 in the previous part. The other nodes' routing tables remained the same. The difference is not drastic because the network is small. However, using hierarchical address assignment in a larger network would make a significant
difference in routing table size.

@lightbox
@fixupini

@nav{step6,step8}

<!-------------------------------------------------------------------------------------------------------->

@page step8 Step 8 - Configuring a mixed wired/wireless network

@nav{step7,step9}

@section s8goals Goals

This step demonstrates routing table configuration in a mixed wired/wireless network. This step consists of 2 parts:
- Part A: Determining members of wireless networks with <wireless> element
- Part B: Determining members of wireless networks by SSID

@section s8a Part A: Using the <wireless> element

The configurator assumes that interfaces of wireless nodes in the same wireless network can reach each other directly.
It can determine which nodes belong to a wireless network in 2 ways:
- It looks at the <i>default_ssid</i> parameter in nodes' agent submodule. Nodes with the same SSID are assumed to be in the same wireless network.
- Members of wireless networks can be specified by the <wireless> element in the XML configuration. In the <wireless> element, the <strong>hosts</strong> and <strong>interfaces</strong> selector attributes can be used to specify members.

Note that nodes need to use the same radio medium module to be in the same wireless network.

@subsection s8aconfig Configuration

This step uses the ConfiguratorD network defined in ConfiguratorD.ned. The network is displayed on the following image.

<img src="step8network.png" width="850px">

The ConfiguratorD network extends ConfiguratorC by adding 2 wireless LANs, <i>area1lan3</i> and <i>area3lan3</i>. The additional LANs consist of an <tt>AccessPoint</tt>
and 3 <tt>WirelessHosts</tt>. The visualizer module is <tt>IntegratedCanvasVisualizer</tt>, which contains multiple visualizer submodules.

The default SSID settings of all wireless nodes is "SSID". By default,
the configurator would put them in the same wireless network, and assume that they can all reach each other directly. This would be reflected
in the routes, hosts in <i>area1lan3</i> would reach hosts in <i>area3lan3</i> directly. This is obviously wrong, because they are out of
range from each other. The 2 LANs need to be in
2 different wireless networks.

Here is the configuration for this step in omnetpp.ini:

@dontinclude omnetpp.uncommented.ini
@skipline Step8
@until ####

A wireless host in <i>area1</i> is configured to ping a wireless host in <i>area3</i>, this helps visualize routes.

There are some settings in the General configuration pertaining to wireless networks:
- The control bit rate of all wireless nodes is set to 54 Mbps for faster ACK transmissions.
- <tt>InterfaceTableVisualizer</tt> is configured to indicate the IP addresses of wireless hosts.

@dontinclude omnetpp.uncommented.ini
@skipline General
@until ####

The XML configuration in step8a.xml is the following:

@dontinclude step8a.xml
@skipline config
@until config

The XML configuration uses the same hierarchical addressing scheme as in Step 7. The 2 wireless LANs are specified to be in separate wireless networks. Note that the wireless interfaces of the access points also belong to the wireless networks.


@subsection s8aresults Results

The addresses and routes are indicated on the following image. Routes leading towards hosts <i>area3lan3</i> are visualized.

@htmlonly
<center><a href="step8a.png" data-lightbox="step8a"><img src="step8a.png" width="850px"></a></center>
@endhtmlonly

<!TODO: outdated screenshot>

Wireless hosts connect to the router through the access points. The access points are L2 devices, similar to switches, so they are transparent for
the routing table visualizer arrows. Wireless hosts get associated with their corresponding access points before they can communicate with the rest of the network.

On the following animation, <i>area1lan3host2</i> pings <i>area3lan3host2</i>. Transmissions are sent to all wireless nodes, but only those in communication range
can receive them correctly (i.e. nodes in the same LAN).

@htmlonly
<center><a href="step8_2.gif" data-lightbox="step8_2"><img src="step8_2.gif" width="850px"></a></center>
@endhtmlonly

<!TODO: outdated gif>

This is how the routes would look like if the XML configuration didnt contain the <wireless> elements:

@htmlonly
<center><a href="step8fullmesh.png" data-lightbox="step8fullmesh"><img src="step8fullmesh.png" width="850px"></a></center>
@endhtmlonly

There are no routes visualized between the <i>backbonerouter</i> and <i>area3router</i>, because routes towards <i>area3lan3</i> go via <i>area1router</i>.
<i>Area3lan3</i> can be reached from the <i>backbonerouter</i> in 2 ways:
- <i>area1router->area1AP->area3lan3</i> (<i>area1AP</i> reaches <i>area3lan3</i> directly since they are assumed to be in the same wireless network)
- <i>area3router->area3AP->area3lan3</i>

In both ways, <i>area3lan3</i> is 2 hops away from the <i>backbonerouter</i>. Routes are configured according to hop count, and the configurator chose the first way.

@section s8b Part B - Using SSID

In this part, the SSID is used to put the 2 wireless LANs in 2 different wireless networks.

@subsection s8bconfig Configuration

The configuration for this part extends Part A. The configuration in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step8B
@until ####

The XML configuration in step8b.xml (displayed below) is the same as the XML configuration for Part A, except there are no <wireless> elements that defined wireless networks.
They are not needed because different SSIDs are configured for the members of the 2 wireless LANs.

@dontinclude step8b.xml
@skipline config
@until config

@subsection s8bresults Results

The results are the same as in the previous part. The 2 wireless LANs are considered to be different wireless networks.

@nav{step7,step9}
@fixupini
@lightbox
@keyword

<!-------------------------------------------------------------------------------------------------------->

@page step9 Step 9 - Leaving some part of the network unconfigured

@nav{step8,step10}

@section s9goals Goals

Configuring the whole network is not always desirable, because some part of the network should rather be configured dynamically.
In this step, some wired and wireless LANs' addresses are unspecified by the configurator, and they get addresses with DHCP.

@section s9model The model

This step uses the ConfiguratorD network, defined in ConfiguratorD.ned.

<img src="step8network.png" width="850px">

The configuration for this step in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step9
@until ####

- Similarly to Step8B, members of the 2 wireless LANs are specified by SSID. 
- <i>Area1lan3host2</i> is configured to ping <i>area3lan3host3</i>.
The pingApp is delayed, so it starts sending pings after the hosts associated with the access points and got their addresses from the dhcp servers.
- <i>DHCPServer</i> submodules are added to the area routers. The DHCP server is configured to listen on the interface connecting to the unspecified LAN.
The interface's netmask is the DHCP server's address range. 
- DHCPClient submodules are added to the LANs which are unspecified by the configurator. There is one such LAN in each area, they are
<i>area1lan3, area2lan1 and area3lan3</i>. These get
the addresses from the DHCP server in the corresponding area router.

- Routes to <i>area3lan3host3</i> are visualized.

The XML configuration in step9.xml is the following:

@dontinclude step9.xml
@skipline config
@until config

Addresses are assigned hierarchically. 5 LANs in the network have addresses assigned by the configurator. 3 LANs get their
addresses from DHCP servers, their interfaces are left unspecified by the configurator. This is accomplished by the lack of address assignment rules for
these hosts in the XML configuration. The area routers' interfaces connecting to the latter LANs need to be specified in order to have correct routes to these
LANs. Additionally, the addresses for these interfaces need to be assigned specifically, and they have to fall in the configured dhcp server address ranges.


@section s9results Results

The addresses and routes are visualized below. This is the state of the network at the start of the simulation.

@htmlonly
<center><a href="step9routes.png" data-lightbox="step9routes"><img src="step9routes.png" width="850px"></a></center>
@endhtmlonly

The hosts of <i>area1lan3, area2lan1 and area3lan3</i> have unspecified addresses. The routing tables of all hosts contain subnet routes
to these 3 LANs. Since these hosts don't have addresses at the start of the simulation, not all routes leading to <i>area3lan3host2</i> can be
visualized. The only routes indicated are the default routes of hosts with static address assignment.

Though the hosts in the 3 LANs have unspecified addresses, subnet routes leading to these LANs are added to the routing tables
of all hosts. The addresses for the interfaces connecting to these LANs have a netmask assigned so there are addresses allocated
for the unspecified hosts. For example, <i>area1router's eth3</i> interface has the address 10.1.4.1/29, and has 4 addresses allocated (10.1.4.2-5).

The routing tables of <i>area1lan3host2, area1router and backbonerouter</i> are the following (routes for reaching the unspecified hosts are highlighted):

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorD.area1lan3host2
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
<span class="marker">10.1.3.0         255.255.255.248  *                wlan0 (<unspec>) 0</span>
<span class="marker"><i></i>*                *                10.1.3.1         wlan0 (<unspec>) 0</span>

Node ConfiguratorD.area1router
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.1.0.1         255.255.255.255  *                eth2 (10.1.0.2)  0
10.1.1.0         255.255.255.248  *                eth0 (10.1.1.4)  0
10.1.2.0         255.255.255.248  *                eth1 (10.1.2.4)  0
<span class="marker">10.1.3.0         255.255.255.248  *                eth3 (10.1.3.1)  0</span>
<span class="marker">10.2.0.0         255.254.0.0      10.1.0.1         eth2 (10.1.0.2)  0</span>

Node ConfiguratorD.backbonerouter
-- Routing table --
Destination      Netmask          Gateway          Iface            Metric
10.1.0.2         255.255.255.255  *                eth1 (10.1.0.1)  0
10.2.0.2         255.255.255.255  *                eth0 (10.2.0.1)  0
10.3.0.2         255.255.255.255  *                eth2 (10.3.0.1)  0
<span class="marker">10.1.0.0         255.255.252.0    10.1.0.2         eth1 (10.1.0.1)  0</span>
<span class="marker">10.2.0.0         255.255.252.0    10.2.0.2         eth0 (10.2.0.1)  0</span>
10.3.0.0         255.255.252.0    10.3.0.2         eth2 (10.3.0.1)  0
</pre>
</div>

- <i>Area1lan3host2</i> has a default route for reaching the other hosts in the LAN. 
- <i>Area1Router</i> has a route for reaching hosts in <i>area1lan3</i>, and a default route for reaching <i>area2 and area3</i>.
- The <i>backbonerouter</i> has subnet routes to each area.

@htmlonly
<center><a href="step9_6.gif" data-lightbox="step9_6"><img src="step9_6.gif" width="850px"></a></center>
@endhtmlonly

@lightbox
@fixupini

@keyword

@nav{step8,step10}

<!-------------------------------------------------------------------------------------------------------->

@page step10 Step 10 - Configuring a completely wireless network

@nav{step9,step11}

@section s10goals Goals

This step demonstrates using the error rate metric for configuring static routes. It also demonstrates leaving the routing tables unconfigured, so a dynamic routing protocol can configure them. The step has 3 parts:

- Part A: Static routing based on error rate metric
- Part B: Unconfigured routing tables, prepared for MANET routing
- Part C: Unconfigured routing tables, configured using AODV protocol

@section s10a Part A: Static routing based on error rate metric

The topology of completely wireless networks is unclear in a static analysis. By default, the configurator assumes all nodes can directly talk to each other.
When they can't, the error rate metric can be used for automatic route configuration instead of the default hop count.

@subsection s10aconfig Configuration

This step uses the ConfiguratorE network, defined in ConfiguratorE.ned. The network looks like this:

<img src="step10network.png" width="850px">

It contains 8 <tt>AODVRouters</tt> laid out in a chain.

The configuration for this part in omnetpp.ini is the folowing:

@dontinclude omnetpp.uncommented.ini
@skipline Step10A
@until ####

The transmitter power of radios determine their communication range. The range is set up so hosts are only in range of the adjacent hosts in the chain.
The <tt>routingTableCanvasVisualizer</tt> is set to visualize routes to all destinations. Communication ranges of all hosts are displayed.

The transmission power outside the communication range is below the sensitivity of the receiving node, thus the
error rate is infinite. However, the fact that the receiving host is within the communication range circle doesn't
mean that it can receive the transmission correctly.

The XML configuration in step10a.xml is as follows:

@dontinclude step10a.xml
@skipline config
@until config

It contains a copy of the default address configurations, and an autoroute element using the error rate metric.
The configurator calculates the packet error rate for an Maximum Transfer Unit (MTU) sized packet. Edge costs in the connectivity
graph are assigned according to this. 


@subsection s10aresults Results

Configured routes and communication ranges are displayed on the following image. Error rate outside the communication range is infinite,
thus all arrows are within the circles. Routes lead through adjacent hosts in the chain. In each
segment of the path, correct reception is possible.

<img src="step10aroutes.png" width="850px">

@section s10b Part B: Unconfigured routing tables, prepared for MANET routing

Static routing is often not adequate in wireless networks, as the nodes might move and the topology can change.
Dynamic routing protocols can react to these changes. When using dynamic protocols, the configurator is only used
to configure the addresses. It leaves the routing table configuration to the dynamic protocol.

The configuration for this part in omnetpp.ini extends the one for Part A. It is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step10B
@until ####

The configurator is instructed to leave the routing tables empty, by setting <tt>addStaticRoutes</tt> to false. The configurator just assigns
the addresses according to the default XML configuration.

@subsection s10bresults Results

<img src="step10a.png" width="850px">

As instructed, the configurator didn't add any routes, as indicated by the lack of arrows. The routing tables are empty.

<img src="step10b_rt.png">

@section s10c Part C: Routing tables configured using AODV protocol

In this part, routing tables are set up by the Ad-hoc On-demand Distance Vector (AODV) dynamic routing protocol. The configuration for this part extends Part B. The configuration in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step10C
@until ####

As specified in the previous part, the configurator is still instructed not to add any routes. Additionally, <i>host1</i> is set to ping
<i>host2</i>. Since AODV is a reactive routing protocol, the ping is required to trigger the AODV protocol to set up routes.

@subsection s10cresults Results

The routing tables are initially empty. The first ping packet triggers AODV's route discovery process, which eventually configures the routes.
AODV is a reactive protocol, unused routes expire after a while. This happens to the routes to <i>host2</i>, as it's not in the path between
<i>host1</i> and <i>host7</i>. This is diplayed in the following animation.

<img src="step10_6.gif">

To record the animation, the simulation was run in fast mode, thus routes appear instantly. It takes a few seconds simulation time for the unused
routes to expire.

@nav{step9,step11}
@fixupini
@keyword

<!-------------------------------------------------------------------------------------------------------->

@page step11 Step 11 - Manually modifying an automatically created configuration

@nav{step10,step12}

@section s11goals Goals

Sometimes the configuration is just almost right. In such a case it's possible to dump the configuration into a file, edit it and use the file in place of the original configuration. This step has 2 parts:

- Part A: dumping the full configuration
- Part B: using the modified configuration

@section s11a Part A - dumping the full configuration

The configurator can be instructed to dump its configuration in a config file, in the XML configuration format. This contains
all the assigned addresses, routing table entries and members of wireless links, so they can be easily modified. The modified
config file can be used as the XML configuration for subsequent simulation runs.

@subsection s11aconfig Configuration

This step uses the ConfiguratorB network defined in ConfiguratorB.ned.

<img src="step4network.png">

The configuration for this part in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step11A
@until ####

The configurator's <tt>dumpConfig</tt> parameter can be used to dump the configuration into a file.

@subsection s11aresults Results

Routes to all nodes are visualized on the following image.

<img src="step11a.png" width="850px">

The configuration is dumped into step11a_dump.xml.

<!TODO: modify addresses?>

@section s11b Part B - Using the modified configuration

In this part, we edit the config file, and use it as the XML configuration. The goal is that packets should travel counter-clockwise in the triangle of the 3 routers. That is, every router should forward packets in the triangle through its interface on the right.

@subsection s11bconfig Configuration

The configuration for this part in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step11B
@until ####

The modified config is used as the XML configuration. Since the configuration specifies all routes, <tt>addStaticRoutes</tt> needs to be set to
<strong>false</strong>, so the configurator doesn't add automatic static routes. A host in each LAN pings another host in the adjacent LAN in the counter-clockwise direction.

To this end, the routes in the router are modified. Routes that would send packets the wrong way (i.e. not counter-clockwise in the triangle) are
redirected to the other interface. In essence, all routers send out packets through their interface to the right (expect for packets destined to
the connecting LAN).

The modified XML configuration is in step11b.xml (see step11dump.xml for the original). The differences between the original and the modified config files area displayed below (the original is shown in red).

@htmlonly
<a href="step11diff.png" data-lightbox="step11diff"><img src="step11diff.png" width="900px"></a>
@endhtmlonly

@subsection s11bresults Results

Routes to all nodes are visualized on the following image. Note that arrows point only counter-clockwise in the triangle.

@htmlonly
<center><a href="step11b.png" data-lightbox="step11b"><img src="step11b.png" width="850px"></a></center>
@endhtmlonly

The ping exchanges are displayed in the animation below.

<img src="step11_3.gif" width="850px">

The packets travel only counter-clockwise in the triangle of the 3 routers.


@lightbox
@fixupini
@nav{step10,step12}

<!-------------------------------------------------------------------------------------------------------->

@page step12 Step 12 - Mixing different kinds of autorouting

@nav{step11,index}

@section s12goals Goals

Sometimes it is best to configure different parts of a network according to different metrics. This step demonstrates
using the hop count and error rate metrics in a mixed wired/wireless network.

@section s12model The model

The step uses the ConfiguratorE network, defined in ConfiguratorF.ned. The network looks like this:

<img src="step12network.png">

The core of the network is composed of 3 routers connected to each other, each belonging to an area. There are 3 areas, each contains a number of hosts,
connected to the area router. 
- Area1 is composed of 3 <tt>WirelessHosts</tt>, one of them
is connected to the router with a wired connection. 
- Area2 has an <tt>AccessPoint</tt>, and 3 <tt>WirelessHosts</tt>. 
- Area3 has 3 <tt>StandardHosts</tt> connected to the
router through a switch.

Since there is no access point in <i>area1</i>, the hosts create an ad-hoc wireless network. They connect to the rest of the network through <i>area1host3</i>,
which has a wired connection to the router.
However, <i>area1host3</i> is not in the communication range of <i>area1host1</i> (illustrated on the image below). Thus <i>area1host2</i> is configured to forward
<i>area1host1's</i> packets to <i>area1host3</i>. The error rate metric is better suited to configure routes in this LAN. Routes in the rest of the network
can be configured properly based on the hop count metric.

<img src="step12ranges.png">

The configuration for this step in omnetpp.ini is the following:

@dontinclude omnetpp.uncommented.ini
@skipline Step12
@until ####

- For hosts in area1 to operate in ad-hoc mode, IP forwarding is turned on, and their management modules are set to ad-hoc management.
- <i>area1host1</i> is configured to ping <i>area2host1</i>, which is on the other side of the network
- Routes to all hosts and communication ranges are visualized.

The XML configuration in step12.xml is the following:

@dontinclude step12.xml
@skipline config
@until config

To have routes from every node to every other node, all nodes must be covered by an autoroute element.
The XML configuration contains 2 autoroute elements. Routing tables of hosts in <i>area1</i> are configured according to the error rate metric,
while all others according to hop count.

The global <i>addStaticRoutes, addDefaultRoutes and addSubnetRoutes</i> parameters can be specified per interface, with the <interface> element.
These can be set with the <strong>add-static-route</strong>, <strong>add-default-route</strong> and <strong>add-subnet-route</strong> bool parameters.
They are true by default. The global and per-interface settings are in a logical AND relationship, thus both have to be true to take effect.

The default route assumes there is one gateway,
and all nodes on the link can reach it directly. This is not the case for <i>area1</i>, because <i>area1host1</i> is out of range of the gateway host. 
The <i>add-default-route</i> parameter is set to false for the <i>area1</i> hosts.

@section s12results Results

The routes are visualized on the following image.

<img src="step12routes.png">

As intended, <i>area1host1</i> connects to the network via <i>area1host2</i>.

The routing of <i>area1host1</i> is as follows:

<div class="fragment fit">
<pre class="monospace">
Node ConfiguratorF.area1host1
-- Routing table --
Destination      Netmask          Gateway          Iface             Metric
10.0.0.1         255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.2         255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.5         255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.6         255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.9         255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.10        255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.18        255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.28        255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.33        255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.34        255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.41        255.255.255.255  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.16        255.255.255.248  *                wlan0 (10.0.0.17) 0
10.0.0.24        255.255.255.248  10.0.0.19        wlan0 (10.0.0.17) 0
10.0.0.40        255.255.255.248  10.0.0.19        wlan0 (10.0.0.17) 0
</pre>
</div>

The gateway is 10.0.0.19 (<i>area1host2</i>) in all rules, expect the one where it is *. That rule is for reaching
the other hosts in the LAN directly. This doesn't seem to be according to the error rate metric, but this rule
matches destinations 10.0.0.18 and 10.0.0.19 only. Since 10.0.0.18 is covered by a previous rule, this one
is actually for reaching 10.0.0.19 directly.

<TODO: !gif>

@fixupini
@keyword



*/