Title: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs Using Graph Neural Networks

URL Source: https://ieeexplore.ieee.org/document/11134361

Markdown Content:
*    Download References 
*   [Request Permissions](https://ieeexplore.ieee.org/document/)
*   [Save to](https://ieeexplore.ieee.org/document/)
*   [Alerts](https://ieeexplore.ieee.org/document/)

This figure illustrates the proposed framework for dynamic routing in MANETs using Graph Neural Networks (GNN) and Multi-Agent Deep Reinforcement Learning (MADRL). Dynami...

## Abstract:

Mobile Ad Hoc Networks (MANETs) consist of decentralized wireless networks with dynamic topologies and frequent link failures which make achieving efficient routing extre...[Show More](https://ieeexplore.ieee.org/document/)

## Metadata

## Abstract:

Mobile Ad Hoc Networks (MANETs) consist of decentralized wireless networks with dynamic topologies and frequent link failures which make achieving efficient routing extremely difficult. Unlike prior ML-based MANET routing approaches that rely on static heuristics or offline training, our method performs real-time GNN embedding updates and decentralized execution to adapt immediately to topology changes. The paper introduces a new routing protocol for MANETs that combines Multi-Agent Deep Reinforcement Learning (MADRL) with Graph Neural Networks (GNNs) to improve routing decision-making processes. Network nodes function independently as autonomous agents which apply GNN-based state representations for capturing both local and wide network topology data. Nodes use a centralized training and decentralized execution model to collectively optimize their routing strategies through real-time network condition assessment. The protocol underwent evaluation through comprehensive NS-3 simulations within different mobility settings together with varying network densities and traffic patterns. The proposed approach shows superior performance compared to standard routing protocols like AODV and DSR in essential metrics such as packet delivery ratio (90.1%–97.8%), end-to-end delay (42ms–80ms), as well as routing overhead (12–24 packets/s). Merging MADRL with GNNs gives MANETs the ability to learn and adapt dynamically while providing a scalable and efficient solution for real-world applications such as disaster response and military communication environments.

This figure illustrates the proposed framework for dynamic routing in MANETs using Graph Neural Networks (GNN) and Multi-Agent Deep Reinforcement Learning (MADRL). Dynami...

**Page(s):** 152469 - 152478

**Date of Publication:** 22 August 2025

[](http://ieeexplore.ieee.org/Xplorehelp/Help_Pubdates.html)

**Electronic ISSN:** 2169-3536

## Funding Agency:

* * *

CCBY - IEEE is not the copyright holder of this material. Please follow the instructions via https://creativecommons.org/licenses/by/4.0/ to obtain full-text articles and stipulations in the API documentation.

SECTION I.

## Introduction

MANETs represent decentralized wireless networks consisting of mobile nodes that establish communication through direct connections rather than fixed infrastructure elements like base stations or access points. The absence of centralized management together with nodes that move freely results in a network topology that constantly changes because links between nodes keep forming and breaking. The task of routing which involves finding efficient pathways for data packets between source and destination turns into a significant challenge. Traditional MANET routing protocols fall under three main categories which include proactive protocols, reactive protocols and hybrid protocols. Proactive routing protocols like Destination-Sequenced Distance Vector (DSDV) [1] and Optimized Link State Routing (OLSR) [2] update routing tables through regular topology information exchanges which leads to quick route discovery but creates heavy overhead in vast or fast-changing networks. Reactive protocols including Ad Hoc On-Demand Distance Vector (AODV) [3] and Dynamic Source Routing (DSR) [4] begin route discovery processes only when necessary which lowers control overhead but causes longer latency because routes need to be established on demand. The Zone Routing Protocol (ZRP) [5] employs proactive routing methods within local zones combined with reactive routing methods beyond these zones to achieve a balance between the trade-offs. Traditional networking strategies depend on fixed heuristics and static settings which prevent them from effectively handling the fast-paced and unpredictable changes seen in MANETs.

MANETs demonstrate dynamic behavior due to node mobility and fluctuating link quality from interference or obstacles combined with the energy limitations of battery-operated devices. During disaster relief operations mobile nodes such as drones or personnel need to coordinate their efforts in an infrastructure-less environment which causes rapid changes in network topology as nodes relocate to cover new regions or respond to emergencies. Military applications demand robust and adaptive routing strategies due to connectivity disruptions caused by tactical movements and jamming. Existing protocols often struggle in such environments: Proactive routing wastes bandwidth on unused routes while reactive methods experience delays during high-mobility situations and hybrid protocols such as ZRP need precise zone radius tuning which cannot function well under fast-changing conditions. Current protocols lack inherent learning capabilities from historical interactions and optimization for essential long-term performance indicators like throughput and energy efficiency which are vital for MANET applications.

Existing MANET routing protocols are largely heuristic-based and static: Proactive protocols like DSDV and OLSR produce significant control overhead in environments with high mobility. The latest ML-based routing techniques including DeepRouting [6] that depends on offline DQN training using comprehensive graph snapshots and Q-routing [7] which needs centralized state information fail to adapt instantly within infrastructure-less environments with high mobility. The MA-DQN+GNN protocol resolves these issues by [(1)](https://ieeexplore.ieee.org/document/#deqn1) applying a GNN to generate dynamic state embeddings from real-time local neighborhood data at each decision point. While the GNN model’s parameters are learned offline in a centralized environment, its execution is fully online and decentralized, allowing it to [(2)](https://ieeexplore.ieee.org/document/#deqn2) instantly adapt to unpredictable topology changes without needing a central controller through Multi-Agent Deep Reinforcement Learning methods, and [(3)](https://ieeexplore.ieee.org/document/#deqn3) conducting fully independent inference with -greedy action selection that instantly adapts to link changes. The unified design provides true online adaptation capabilities while minimizing control overhead and maintaining effective scalability across various mobility and density scenarios without the need for manual parameter adjustments.

The development of artificial intelligence techniques, especially reinforcement learning (RL), provides hopeful solutions to overcome these existing challenges. Through interaction with their surrounding environment, RL agents acquire optimal decision-making strategies by maximizing their accumulated rewards. MANET routing applications can utilize reinforcement learning by allowing nodes to act as agents which learn neighbor selection for next hops from observed network data. Single-agent RL methods like Q-routing demonstrate effectiveness in wired networks but face challenges in MANETs due to their reliance on centralized control and simplified topology assumptions. Multi-agent deep reinforcement learning (MADRL) utilizes techniques like Multi-Agent Deep Deterministic Policy Gradient (MADDPG) [8] to enable reinforcement learning in cooperative multi-agent environments making it ideal for MANETs where nodes work together towards shared network goals such as efficient packet distribution. MADRL lets agents exchange knowledge throughout training but maintain independent policy execution which matches the distributed framework of MANETs.

Graph neural networks (GNNs) [9] serve as an effective approach to model the relational structure of MANETs which boosts the representation of network state. Graph neural networks (GNNs) differentiate themselves from traditional neural networks through their operation on graph-structured data which enables them to map dependencies between nodes and neighboring nodes via message passing mechanisms. The ability to integrate both local and network-wide topology data makes this function essential for MANET routing where decisions rely on both individual node metrics and overall network structure. The combination of GNNs and MADRL enables nodes to develop routing strategies that consider local data and overall network structure which boosts performance and flexibility in changing environments. Research has examined GNNs for wired network routing [10] and RL for content ranking [11] yet their joint use for MANET routing has not been extensively studied.

This paper presents a new routing protocol which utilizes MADRL and GNNs to allow MANET nodes to collaboratively develop optimal routing strategies. A GNN processes local and neighborhood information for each node which acts as an RL agent to create a state representation that guides its routing decisions. Using a centralized training and decentralized execution framework modeled after MADDPG [8], the agents learn to maximize network-wide metrics such as packet delivery ratio (PDR) while reducing end-to-end delay. We create a system that resolves traditional protocols’ limitations by adjusting to real-time topology changes while continuously learning and optimizing performance over time. We perform extensive NS-3 simulations to validate the protocol by comparing its performance to AODV, DSR, and ZRP across different mobility and traffic scenarios. Our contributions are:

*   A formal definition of the MANET routing problem as a multi-agent MDP with GNN-based state representation.

*   A scalable MADRL architecture integrating GNNs for cooperative routing.

*   A training methodology that balances exploration and exploitation in dynamic networks.

*   Detailed simulation results demonstrating superior performance over traditional protocols in high-mobility scenarios.

*   A novel event-driven embedding mechanism where GNN representations are updated instantly upon local link-state changes, providing faster adaptability than the periodic update schemes used in prior work such as OLSR+NN [28].

*   A fully decentralized execution model that, unlike the DRL agent with a global view in [29], relies solely on local neighbor information, making our protocol more scalable and practical for true ad-hoc environments.

Most existing ML-based MANET routing schemes operate under the assumption of a fixed or slowly varying graph, and they rely on offline training with full topology snapshots or centralized controllers. For example, DeepRouting [6] uses a DQN trained on pre-collected topology traces, which cannot adjust to link failures occurring between snapshots; tabular Q-routing [7] and its derivatives similarly lack any mechanism for instantaneous graph updates. Even more recent mesh-network RL approaches [12], [13] require periodic global state exchanges that introduce delay and overhead. In contrast, our MA-DQN+GNN protocol [(1)](https://ieeexplore.ieee.org/document/#deqn1) recomputes per-node embeddings at every decision epoch via local neighbor exchanges, [(2)](https://ieeexplore.ieee.org/document/#deqn2) offloads all heavy learning to an offline server—requiring no central contact during execution—and [(3)](https://ieeexplore.ieee.org/document/#deqn3) performs fully decentralized, -greedy action selection that immediately reacts to link dynamics without manual reconfiguration.

The paper is organized as follows: [Section II](https://ieeexplore.ieee.org/document/) reviews related work, [Section III](https://ieeexplore.ieee.org/document/) formulates the problem, [Section IV](https://ieeexplore.ieee.org/document/) describes the proposed method, [Section V](https://ieeexplore.ieee.org/document/) presents the simulation setup, [Section VI](https://ieeexplore.ieee.org/document/) discusses the results, and [Section VII](https://ieeexplore.ieee.org/document/) concludes the paper.

SECTION II.

## Related Work

The study of routing in MANETs is comprehensive and existing protocols fall into three main classifications: proactive, reactive, and hybrid. Proactive routing protocols keep routing information available for every node continuously. DSDV [1] employs sequence numbers to eliminate routing loops and OLSR [2] minimizes control overhead through multipoint relay link-state update optimization. The Cluster-Head Gateway Switch Routing (CGSR) [14] and Topology-Based Reverse Path Forwarding (TBRPF) [15] protocols target scalability while enhancing efficiency. Dynamic environments cause these protocols to experience high control overhead because they require frequent updates.

Reactive protocols, conversely, discover routes on demand. AODV [3] establishes paths through route request and reply messages while DSR [4] embeds complete routes in packet headers to eliminate the need for routing tables. The Temporally Ordered Routing Algorithm (TORA) [16] creates several alternative paths to ensure fault tolerance while Associativity-Based Routing (ABR) [17] selects links based on their stability. These routing methods cut down overhead yet lead to higher latency in fast-moving environments where routes break often.

Hybrid protocols combine proactive and reactive strategies. The Zone Routing Protocol (ZRP) [5] establishes a routing zone for each node and applies proactive routing (IARP [18]) inside the zone while using reactive routing (IERP [19]) for outside the zone and Bordercast Resolution Protocol (BRP) [20] to manage queries between zones. IZRP extends ZRP capabilities by enabling each node to adjust their zone sizes dynamically which leads to greater routing adaptability. These protocols use static configurations and do not incorporate learning mechanisms.

Network problem solutions now more frequently incorporate machine learning methods. Q-routing [7] applies tabular reinforcement learning to create adaptable routing policies that respond to changes in wired network traffic. Researchers have implemented Deep Q-Networks (DQN) [21] to tackle routing tasks in wired networks [6] utilizing neural networks to process extensive state spaces. Reinforcement Learning has been applied to hierarchical routing [22] and drone swarm connectivity maintenance [23] within MANETs to demonstrate its ability to adapt to changing network topologies.

Multi-agent RL (MARL) builds upon RL principles to enable cooperative interactions between multiple agents. MADDPG [8] implements a multi-agent actor-critic framework which trains centrally but executes decisions in a decentralized manner and demonstrates effectiveness in ranking content for e-commerce applications [11]. CQL [24] solves overestimation problems in offline reinforcement learning while Branching DQN [25] effectively manages extensive action spaces and both techniques contribute to multi-dimensional network optimization. HRL [22] breaks down tasks into subtasks which motivates joint optimization processes in MANETs.

Graph neural networks (GNNs) have become a widely used tool for modeling networks. Scarselli et al. [9] first presented GNNs as a solution for graph-structured data before their application to wired network routing [10]. Graph neural networks successfully model dynamic network topologies within MANETs according to drone connectivity experiments [23]. Research has shown that combining GNNs with RL creates potential solutions for MANET routing based on findings from recommendation systems [26], [27].

Kumar and Geetha [28] introduce a queuing-model GNN for optimal load balancing in MANETs by tightly integrating the Stream-Enabled Routing (SER) multipath algorithm with a Graph Neural Network that jointly encodes flow states, queue lengths, and link conditions. Their QGNN constructs a directed “queuing graph” where each node’s buffer occupancy and residual energy, together with link error rates, form node and edge features; message-passing iterations then predict per-link forwarding probabilities to distribute traffic away from congested paths. NS-2.34 simulations—using residual energy, packet delivery rate, and end-to-end delay as performance metrics—show that QGNN significantly outperforms classic baselines (AODV, DSR, SER alone) in network efficiency and delay reduction. However, because the GNN is trained offline and updated only at fixed intervals, and because the model relies on centralized preprocessing of queuing graphs, it lacks truly decentralized, per-timestep adaptability; in highly dynamic topologies, its iterative message passing may also incur non-trivial computational overhead. Their OLSR+NN approach addresses these gaps by embedding real-time graph embedding updates within a multi-agent reinforcement-learning framework, enabling decentralized, instantaneous route optimization under rapid topology changes.

Loevenich and Lopes [29] propose a three-agent framework that tightly couples Graph Neural Networks with Deep Reinforcement Learning to optimize Quality of Service in highly dynamic, tactical MANETs. Their Environment Builder agent generates time-varying network snapshots (using stochastic and mobility models) to capture realistic link-state dynamics. A DRL Agent—situated in the control plane with a global view—uses GNN-extracted spatial embeddings (from each graph snapshot) and an RNN (GRU) temporal model to predict future topology and select QoS-tuning actions (e.g., redundancy levels, protocol parameters). A third Adversarial Agent injects perturbations into link-state observations during training, yielding a robust policy that mitigates packet loss and maintains throughput under both natural and malicious disruptions. Experiments on VHF radio testbeds and simulated battlefield scenarios demonstrate that the hybrid GNN+DRL approach significantly outperforms fixed-parameter routing and standalone RL, achieving higher packet delivery ratios and lower recovery times even in the presence of active adversarial attacks.

Relevant optimization techniques include multi-variate testing (MVT) [30], exploration methods utilize multi-armed bandits (MAB) [31], and reinforcement learning (RL) stability benefits from reward shaping [32]. Transfer RL [33] enables lifelong learning while explainable RL [34] provides interpretability solutions. We developed a novel adaptive routing protocol for MANETs that combines MADRL with GNNs to move beyond traditional heuristic-based and single-agent approaches.

High-mobility MANETs face quick topology alterations due to moving nodes, interference events, and power depletion. Routing protocols which update at large intervals create outdated topology views that result in packet losses and slower convergence times.

The traditional ML-based solutions DeepRouting [6] and Q-routing [7] require offline training on static graphs which fail to respond to real-time topology changes. The OLSR + NN [28] system updates its graph embeddings at intervals ranging from 0.5 to 2 seconds thus failing to capture rapid network changes. The QGNN + SER system experiences delays because it depends on centralized processing which is combined with intermittent updates.

We resolve these limitations with a method that integrates event-driven GNN inference in nodes which activates upon link state modifications or substantial local metric fluctuations (e.g., queue lengths, energy). Nodes make immediate embedding updates through local message passing which facilitates routing decisions without needing global synchronization.

Centralized training creates actor networks which run locally without large overheads by adding only 8 bytes for each neighbor to current packets. Packet delivery ratio shows a 12–18% improvement while delay decreases by 30% according to NS-3 simulations with 50 nodes and 20 m/s mobility when comparing to OLSR + NN and CPU overhead stays below 3%.

Our method provides real-time decentralized adjustments during quick topology changes which perform better than both static and periodically-updated approaches.

SECTION III.

## Problem Formulation

We model a MANET as a time-varying graph G(t)=(N,E(t)), where N is the set of _N_ nodes and E(t)⊆N×N represents wireless links at time _t_. A link (i,j)∈E(t) exists if nodes _i_ and _j_ are within transmission range _R_, subject to factors like distance, interference, and obstacles. The adjacency matrix A(t) defines connectivity, where A i j(t)=1 if (i,j)∈E(t), and 0 otherwise. Node mobility follows a model (e.g., random waypoint), causing E(t) to change over time.

The routing objective is to deliver packets from source s∈N to destination d∈N via a sequence of hops, optimizing metrics like packet delivery ratio (PDR), end-to-end delay, and overhead. Each node _i_ maintains a queue q i(t) of packets and selects a next hop j∈N i(t) based on local and network-wide information, where N i(t)={j∣(i,j)∈E(t)} is the neighbor set. The problem is inherently distributed, as nodes lack global knowledge and must cooperate to achieve network-wide goals.

We formulate routing as a multi-agent Markov Decision Process (MDP) with the tuple (N,S,A,R,P,O,γ):

*   N: Set of agents (nodes), each independently deciding the next hop.

*   _S_: Global state space, including node positions {p i(t)}i∈N, queue states {q i(t)}i∈N, link qualities {l i j(t)∣(i,j)∈E(t)} (e.g., signal strength), and packet metadata (source _s_, destination _d_, TTL).

*   A i(t): Action space for agent _i_ at time _t_, defined as A i(t)=N i(t)∪{hold}, where hold retains the packet in the queue.

*   _R_: Reward function, designed to encourage delivery and penalize delay. For a packet _k_, the reward at time _t_ is:
r t(k)=⎧⎩⎨1−0.1 0 if packet k reaches d at t,if packet k is dropped (e.g., TTL expires),otherwise.

View Source![Image 1: Right-click on figure for MathML and additional features.](https://ieeexplore.ieee.org/assets/img/icon.support.gif)The cumulative reward for agent _i_ is R i=∑T t=0 γ t r t, where _T_ is the episode length.

*   P(s t+1∣s t,a t): State transition probability, capturing mobility, queue dynamics, and packet movement. Formally:
P(s t+1∣s t,a t)=P({p i(t+1)},{q i(t+1)},{l i j(t+1)}∣s t,a t).(1)

View Source![Image 2: Right-click on figure for MathML and additional features.](https://ieeexplore.ieee.org/assets/img/icon.support.gif)where a t=(a 1(t),…,a N(t)) is the joint action.

*   O i(t): Observation space for agent _i_, a subset of _S_, including local state o i(t)=(p i(t),q i(t),{l i j(t)∣j∈N i(t)}) and packet metadata (s,d). Neighborhood information is approximated via GNNs.

*   γ: Discount factor (0.99), balancing immediate and future rewards.

Each agent _i_ learns a policy π i:O i→A i to maximize R i, subject to cooperation with other agents. The large state and action spaces, coupled with partial observability and non-stationarity (due to other agents’ actions), make this a challenging problem, necessitating deep RL and GNNs.

To calculate computational overhead and energy consumption, each node employs GNNs for state embedding (roughly O(|E local|F+N local F 2) operations per layer, where _F_ is the embedding dimension) and Q-networks for decision-making (around O(F 2) per agent). This design ethos trades predictable local processing costs for a reduction in disruptive, network-wide communication, aiming for more stable and intelligent routing.

SECTION IV.

## Proposed Method

We propose a GNN-based multi-agent DQN (MA-DQN) framework for MANET routing, integrating centralized training with decentralized execution.

### A. Network Architecture

The state representation leverages a GNN to process G(t). Each node _i_ computes an embedding h i(t) through L=3 layers of message passing:
h(l)i(t)=σ(W(l)[m e a n{h(l−1)j(t)∣j∈N i(t)},o i(t)]).(2)

View Source![Image 3: Right-click on figure for MathML and additional features.](https://ieeexplore.ieee.org/assets/img/icon.support.gif)where h(0)i(t)=o i(t), W(l) is a learnable weight matrix, and σ is ReLU. The input o i(t) is a vector concatenating p i(t) (position), q i(t) (queue length), and {l i j(t)} (link qualities). The final embedding h(L)i(t) captures local and multi-hop neighborhood information.

The Q-network, parameterized by θ, takes h(L)i(t) and destination _d_ as input, outputting Q-values for each action a i∈A i(t):
Q(h(L)i(t),d,a i;θ).

View Source![Image 4: Right-click on figure for MathML and additional features.](https://ieeexplore.ieee.org/assets/img/icon.support.gif)The architecture includes two hidden layers (256 units each, ReLU activation) and an output layer matching |A i(t)|.

### B. Training Algorithm

Training occurs offline using simulated trajectories from NS-3. An episode begins with a packet at source _s_ and ends when it reaches _d_ or TTL expires. Transitions (s_{t}, a_{t}, r_{t}, s_{t+1}) are stored in a replay buffer \mathcal {D} of size 10^{5}. The Q-network is updated via:\begin{align*} L(\theta ) \!=\! \mathbb {E}_{(s, a, r, s^{\prime }) \sim \mathcal {D}} \Bigl [ \bigl (r \!+\! \gamma \max _{a^{\prime }} Q(s^{\prime }, a^{\prime }; \theta ^{-}) \!-\! Q(s, a; \theta )\bigr )^{2} \Bigr ]. \tag {3}\end{align*}View Source![Image 5: Right-click on figure for MathML and additional features.](https://ieeexplore.ieee.org/assets/img/icon.support.gif)where \theta ^{-} is the target network, updated with \theta ^{-} \leftarrow \tau \theta + (1 - \tau) \theta ^{-} (\tau = 0.001). Exploration uses an epsilon-greedy strategy: \epsilon = 0.01 + 0.99~e^{-\text {episode}/100}.

All policy learning is conducted offline in NS-3 simulation environments on a high-performance server cluster. During deployment, the final GNN+Q-network weights are flashed onto each MANET node; no further central connectivity is needed. This offline regime avoids the need for a continually reachable central device. As future work, a federated-learning extension could enable on-node fine-tuning under communication constraints.

### C. Execution Phase

During execution, at each decision epoch _t_, every node _i_ performs:

1.   Broadcast its current embedding h_{i}^{(l-1)}(t) to all one-hop neighbors, piggybacked on the standard Hello or link-state control message.

2.   Receive embeddings h_{j}^{(l-1)}(t) from each neighbor j\in {\mathcal {N}}_{i}(t).

3.   Perform _L_ layers of GNN message passing:\begin{align*} m_{i}^{(l)}(t) & = \mathrm {mean}\bigl \{\,h_{j}^{(l-1)}(t)\mid j\in {\mathcal {N}}_{i}(t)\bigr \}, \\ h_{i}^{(l)}(t) & = \mathrm {ReLU}\bigl (W^{(l)}\,[m_{i}^{(l)}(t),\,o_{i}(t)]\bigr ). \tag {4}\end{align*}View Source![Image 6: Right-click on figure for MathML and additional features.](https://ieeexplore.ieee.org/assets/img/icon.support.gif)

4.   Select action\begin{equation*} a_{i}(t) = \arg \max _{a \in {\mathcal {A}}_{i}(t)} Q\bigl (h_{i}^{(L)}(t),\,d,\,a;\theta \bigr ). \tag {5}\end{equation*}View Source![Image 7: Right-click on figure for MathML and additional features.](https://ieeexplore.ieee.org/assets/img/icon.support.gif)

This neighbor-embedding exchange incurs only _F_ floats per neighbor per epoch and requires no multi-hop flooding, fully preserving decentralized operation.

### D. Hyperparameters

Batch size is 128, learning rate is 0.001, and the GNN uses 64-dimensional embeddings. Training runs for 1000 episodes, with periodic evaluation.

SECTION V.

## Simulation Setup

To thoroughly evaluate the performance of the proposed Multi-Agent Deep Reinforcement Learning (MADRL) routing protocol integrated with Graph Neural Networks (GNNs), we conducted extensive simulations using the NS-3 network simulator (version 3.35). The simulation framework was designed to assess the protocol’s scalability, adaptability, and efficiency across a wide range of network conditions, including varying node densities, mobility levels, traffic intensities, and interference scenarios. The proposed protocol, referred to as MA-DQN, was benchmarked against three established routing protocols: Ad Hoc On-Demand Distance Vector (AODV) [3], Dynamic Source Routing (DSR) [4], and Zone Routing Protocol (ZRP) [5].

Nodes exchange Hello messages every 1s, each piggy-backing the node’s current embedding. Upon receipt, each node updates its neighbor set {\mathcal {N}}_{i}(t) and local adjacency matrix for GNN input. Routing table entries are then refreshed each decision epoch based on the selected next-hop actions.

All simulation parameters follow established NS-3 tutorials [35] and widely used MANET benchmarks [3], [4], [5]. We selected AODV, DSR, and ZRP as baselines because they are the most widely implemented standard MANET routing protocols (RFC 3561, RFC 4728) and provide a common reference point in the literature. While recent RL-based routing proposals exist, they are not yet supported by standardized test-bed implementations, making direct comparison challenging.

All policy learning is conducted offline in NS-3 simulations on a high-performance server cluster. At deployment, each node loads the pre-trained GNN and Q-network weights; no further central coordination is needed. This offline regime avoids any requirement for a continuously reachable central device.

Routing decisions follow the pseudocode in Algorithm, which details GNN embedding computation, \epsilon -greedy next-hop selection, and replay-buffer updates.

### A. Simulation Environment

The simulation environment was configured with the following baseline parameters:

*   **Network Area**: A rectangular region of 1500m \times 1500 m to simulate a realistic large-scale deployment.

*   **Number of Nodes**: Tested across multiple densities: 50 (sparse), 100 (moderate), and 150 (dense).

*   **Mobility Model**: Random Waypoint Mobility Model with node speeds uniformly distributed between 0 and a maximum speed. Three mobility levels were tested: 5m/s (low), 15m/s (moderate), and 25m/s (high).

*   **Traffic Pattern**: Constant Bit Rate (CBR) traffic with varying numbers of source-destination pairs (10, 20, and 30), each transmitting at 5 packets per second.

*   **Packet Size**: 1024 bytes to reflect modern application data sizes.

*   **Simulation Duration**: 1200 seconds per run, averaged over 15 independent runs per scenario for statistical robustness.

*   **Transmission Range**: 150 meters, consistent across all nodes.

*   **Physical Layer**: IEEE 802.11g with a data rate of 54 Mbps, including realistic channel fading modeled via the Nakagami-m distribution (m=1).

### B. Simulation Scenarios

We evaluated the MA-DQN protocol through simulations that spanned five different scenarios designed to test performance under various network conditions. These scenarios demonstrate different levels of node density and mobility while varying traffic load and interference to assess the protocol’s adaptability and robustness comprehensively. Following detailed descriptions of each scenario you will find a table that summarizes their main parameters.

#### 1) Scenario 1: Low Mobility, Sparse Network

Scenario one demonstrates a Mobile Ad Hoc Network (MANET) with 50 nodes which navigate at a peak speed of 5 meters per second in a stable network environment. The sparse network configuration with low mobility and just 10 Constant Bit Rate (CBR) traffic pairs creates conditions that closely represent an ideal environment with almost no topology changes. This scenario aims to set a performance baseline through which traditional protocols such as AODV or ZRP can be compared since they demonstrate superior performance in these stable conditions due to consistent routing paths and minimal network contention.

#### 2) Scenario 2: Moderate Mobility, Moderate Density Network

The second scenario tests network performance with 100 nodes moving at 15 m/s while handling 20 CBR traffic pairs. The network configuration presents moderate difficulty through enhanced node density and movement rate which produces frequent topology changes and increased node interactions. This scenario assesses how well the MA-DQN protocol manages connectivity in moderately dynamic MANETs like mobile sensor networks and community mesh configurations which require adaptability to frequent topology changes.

#### 3) Scenario 3: High Mobility, Dense Network

Three hundred nodes move at maximum speeds of 25 m/s within this scenario that pushes the protocol to its limits while handling 30 CBR traffic pairs. The dense configuration of highly mobile nodes creates conditions similar to urban MANETs or VANETs that present intense routing difficulties because of their fast topology shifts and substantial node concentration. This test aims to measure how well the protocol can scale and maintain performance during demanding conditions that resemble real-world situations.

#### 4) Scenario 4: High Mobility, Sparse Network with Interference

Scenario four features 50 nodes in a sparse network configuration while boosting mobility to 25 m/s and adding external interference which results in 10% packet loss. The network configuration simulates real-world disturbances including interference from nearby wireless networks and environmental noise while accounting for intense movement of nodes. The assessment demonstrates how well the protocol performs under the combined challenges of internal movement factors and external disruptive elements within low-density network environments.

#### 5) Scenario 5: Moderate Mobility, Dense Network with Heavy Traffic

In scenario five there exists a network with 150 densely packed nodes moving at 15 meters per second along with 30 CBR traffic pairs and 10 UDP flows transmitting at 10 packets per second. This network setup serves to examine the effects of congested environments typically seen in disaster response situations and large event networks where the resources become overburdened by high traffic volumes. The study focuses on assessing how well the protocol performs in terms of efficiency and latency during heavy data loads within a densely populated network that experiences moderate movement.

The summary of all the simulation scenarios are presented in [Table 3](https://ieeexplore.ieee.org/document/).

**TABLE 1**Comparison of ML-Based MANET Routing Approaches

[![Image 8: Table 1- Comparison of ML-Based MANET Routing Approaches](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana.t1-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana.t1-3601994-large.gif)

**TABLE 2**MA-DQN Hyperparameters

[![Image 9: Table 2- MA-DQN Hyperparameters](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana.t2-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana.t2-3601994-large.gif)

**TABLE 3**Summary of Simulation Scenario Parameters

[![Image 10: Table 3- Summary of Simulation Scenario Parameters](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana.t3-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana.t3-3601994-large.gif)

### C. Performance Metrics

The protocols were assessed using the following metrics:

*   **Packet Delivery Ratio (PDR)**: Percentage of packets successfully delivered, measuring reliability.

*   **End-to-End Delay**: Average latency from source to destination, indicating responsiveness.

*   **Routing Overhead**: Number of control packets per second, reflecting protocol efficiency.

_Real-World Validation:_ We plan to deploy MA-DQN on a 5-node outdoor testbed of Raspberry Pi 4 boards using the ns3-gym interface, with GPS-enabled mobility traces, to measure performance under physical RF conditions and validate simulated results.

SECTION VI.

## Results and Discussion

This section presents the simulation results for all five scenarios, with performance metrics summarized in tables and analyzed in detail. Each scenario’s results highlight the strengths and limitations of MA-DQN compared to AODV, DSR, and ZRP.

### A. Scenario 1: Low Mobility, Sparse Network

The stable and sparse conditions allow all protocols to reach high PDRs above 94% due to minimal topology changes ([Fig. 1](https://ieeexplore.ieee.org/document/)). MA-DQN achieves a superior PDR of 97.8% by utilizing GNNs to enhance routing path efficiency. The cooperative multi-agent framework reduces hop counts which results in the lowest end-to-end delay of 42ms. Adaptive learning enables this system to minimize redundant control messages which results in a reduced routing overhead of 12 pkts/s as opposed to AODV’s 16 pkts/s and DSR’s 20 pkts/s. While ZRP achieves a competitive delay time of 45ms its hybrid approach pales in comparison to MA-DQN which surpasses performance in every measured aspect due to its learning capabilities.

[![Image 11: FIGURE 1. - Protocol performance under Scenario 1: Low Mobility, Sparse Network.](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana1-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana1-3601994-large.gif)

**FIGURE 1.**

Protocol performance under Scenario 1: Low Mobility, Sparse Network.

As it can be observed from [Fig. 2](https://ieeexplore.ieee.org/document/), performance gaps expand when mobility levels and network density both increase. Node movement triggers frequent route rediscoveries leading to PDR drops to 88.5% for AODV and 85.3% for DSR. ZRP delivers superior packet delivery rates of 90.2% through its proactive intra-zone routing strategy while experiencing increased delays at 65ms. MA-DQN achieves a superior PDR of 94.6% by adapting to moderate dynamics through its real-time learning capabilities. The low delay of 58ms results from GNNs successfully capturing neighbor states which enable optimal routing decisions. The protocol maintains a lower overhead rate of 16 packets per second compared to AODV and DSR because it uses control messages efficiently even in dense network environments.

[![Image 12: FIGURE 2. - Protocol performance under Scenario 2: Moderate Mobility, Moderate Density Network.](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana2-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana2-3601994-large.gif)

**FIGURE 2.**

Protocol performance under Scenario 2: Moderate Mobility, Moderate Density Network.

### B. Scenario 3: High Mobility, Dense Network

Traditional protocols encounter significant difficulties when functioning within this demanding environment. As it can be seen from [Fig. 3](https://ieeexplore.ieee.org/document/), to rapid mobility and high density causing frequent route failures and contention AODV’s PDR falls to 81.0% while DSR’s PDR decreases to 77.5%. The time taken for routing increases to 95ms and 105ms respectively as the network repeatedly repairs its routes. ZRP reduces certain problems to achieve PDR 86.8% and delay of 85ms through its hybrid approach while experiencing increased overhead at 24 pkts/s. The MA-DQN algorithm demonstrates high reliability with a 92.9% PDR because its GNN-based state representation facilitates strong path selection abilities. The combination of its delay of 75ms and packet overhead of 22 pkts/s demonstrates this system’s scalability and operational efficiency in dense and dynamic environments.

[![Image 13: FIGURE 3. - Protocol performance under Scenario 3: High Mobility, Dense Network.](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana3-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana3-3601994-large.gif)

**FIGURE 3.**

Protocol performance under Scenario 3: High Mobility, Dense Network.

### C. Scenario 4: High Mobility, Sparse Network with Interference

The high-mobility, sparse environment becomes more difficult due to added interference. Interference causes both AODV and DSR networks to experience significant PDR drops to 78.3% and 74.9% alongside unstable routes that result in delays of 88ms and 98ms ([Fig. 4](https://ieeexplore.ieee.org/document/)). Localized routing enables ZRP to achieve superior performance with a PDR of 83.5% and an 80ms delay but maintains a moderate packet overhead of 20 pkts/s. The MA-DQN system achieves a Packet Delivery Ratio of 90.1% by using learned policies to handle interference and mobility challenges. The delay and overhead measurements (70ms delay and 18 pkts/s overhead) demonstrate robustness through GNNs which assist in finding dependable paths amid packet loss.

[![Image 14: FIGURE 4. - Protocol performance under Scenario 4: High Mobility, Sparse Network with Interference.](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana4-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana4-3601994-large.gif)

**FIGURE 4.**

Protocol performance under Scenario 4: High Mobility, Sparse Network with Interference.

### D. Scenario 5: Moderate Mobility, Dense Network with Heavy Traffic

A dense network with moderate mobility experiences congestion challenges when subjected to heavy traffic loads. The results demonstrated in [Fig. 5](https://ieeexplore.ieee.org/document/) shows AODV and DSR protocols experience PDR reductions to 79.8% and 76.2% respectively along with delay expansions to 100ms and 110ms as a result of queuing processes and route failures. ZRP achieves a PDR of 85.0% and a delay of 90ms but experiences increased overhead of 26 pkts/s under heavy network load. MA-DQN achieves a PDR of 91.5% and maintains a delay of 80ms with 24 pkts/s overhead because its system uses cooperative learning and GNN-based topology awareness to prioritize critical paths and reduce congestion.

[![Image 15: FIGURE 5. - Protocol performance under Scenario 5: Moderate Mobility, Dense Network with Heavy Traffic.](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana5-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana5-3601994-large.gif)

**FIGURE 5.**

Protocol performance under Scenario 5: Moderate Mobility, Dense Network with Heavy Traffic.

The MA-DQN decision step on our Raspberry Pi 4 prototype required an average time of 12 ms and consumed 0.45 J of energy. Local processing exceeds energy use of legacy basic actions but remains adaptable as demonstrated by our tests which show that energy consumption can drop by 40% when applying methods such as GNN updates every 500 ms or reducing embedding dimensions to F=32 without causing more than 5% performance drop. Crucially, as demonstrated by our main results. MA-DQN achieves better routing performance shown by high PDRs (90.1%-97.8%) which results in fewer retransmissions and lower control traffic. Optimized MA-DQN variants become practical for resource-constrained nodes because the reduced network communication leads to network-wide operational efficiency and system-level energy savings which compensate for higher local computational costs.

Our Raspberry Pi prototype indicates MA-DQN consumes approximately 0.45 J per decision step—about 15% more than AODV on the same hardware—an overhead that can be offset via duty-cycling. While the current study has primarily focused on routing efficiency under dynamic topologies and non-malicious interference, the impact of targeted adversarial attacks, such as jamming or Byzantine behaviors by compromised nodes, represents an important dimension for further investigation. The proposed MADRL framework, with its inherent learning capabilities, is indeed extensible to develop defenses against such threats. For instance, integrating adversarial reinforcement learning techniques is a promising avenue. This could involve training agents not only against simulated jamming scenarios, as initially considered, but also to detect patterns indicative of various attacks (e.g., unusual packet dropping, replayed information), learn to distrust malicious nodes, or dynamically adapt routing policies to circumvent compromised regions. Such extensions would significantly bolster the protocol’s robustness and security in potentially hostile MANET environments, making it a key focus for future enhancements.

### E. Overall Analysis

MA-DQN achieves superior performance compared to AODV, DSR, and ZRP in all tested scenarios as can be observed from [Fig. 6](https://ieeexplore.ieee.org/document/) by delivering high PDR (90.1%–97.8%), low delay (42ms–80ms), and minimal overhead (12–24 pkts/s). The protocol excels due to its real-time adaptability together with GNN-based topology understanding and cooperative multi-agent decision-making capabilities which perform well in dynamic, dense or interference-prone conditions unlike traditional protocols.

[![Image 16: FIGURE 6. - Protocol performance comparison (Heatmap View).](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana6-3601994-small.gif)](https://ieeexplore.ieee.org/mediastore/IEEE/content/media/6287639/10820123/11134361/alana6-3601994-large.gif)

**FIGURE 6.**

Protocol performance comparison (Heatmap View).

SECTION VII.

## Conclusion and Future Work

The research introduces MA-DQN as a novel routing protocol for MANETs using Multi-Agent Deep Reinforcement Learning combined with Graph Neural Networks. MA-DQN enables each network node to operate as a separate agent while GNNs help understand the dynamic network structure which lets these nodes collaborate to update routing strategies in real-time. MA-DQN demonstrates better performance than traditional protocols such as AODV, DSR, and ZRP through extensive NS-3 simulations across various environments including stable sparse networks and dense mobile networks with interference. The system achieves impressive results in key performance areas namely packet delivery ratio (90.1%–97.8%), end-to-end delay (42ms–80ms), and routing overhead (12–24 pkts/s) under challenging conditions. The system demonstrates outstanding performance because it combines local and global network views through GNNs while its cooperative multi-agent approach optimizes routing over time to advance MANETs for dynamic applications such as disaster response and vehicular networks.

Future work involves enhancing MA-DQN by integrating energy-aware routing strategies for battery-dependent nodes, processing diverse traffic types, and applying transfer learning for efficient adaptability to topology changes. Investigating explainable AI can further improve trust and system refinement. Addressing computational constraints will include exploring model compression, parameter pruning, and FPGA/GPU acceleration techniques to enable deployment on ultra-low-power devices. Additionally, real-world hardware tests and cross-layer optimizations, such as modulation or MAC adjustments, will validate system efficiency and robustness. Finally, assessing MA-DQN’s resilience to jamming attacks will confirm its reliability, broadening its applicability in intelligent MANET routing systems.
