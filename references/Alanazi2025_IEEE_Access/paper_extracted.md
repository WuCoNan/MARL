Received 4 July 2025, accepted 5 August 2025, date of publication 22 August 2025, date of current version 5 September 2025.
Digital Object Identifier 10.1109/ACCESS.2025.3601994




Multi-Agent Deep Reinforcement Learning for
Dynamic Routing in MANETs Using Graph Neural
Networks
FAISAL ALANAZI                 1 AND MAHDI ZAREEI                      2 , (Senior Member, IEEE)
1 Department of Electrical Engineering, College of Engineering, Prince Sattam Bin Abdulaziz University, Al-Kharj 11942, Saudi Arabia
2 School of Engineering and Sciences, Tecnologico de Monterrey, Monterrey 64849, Mexico

Corresponding author: Faisal Alanazi (faisal.alanazi@psau.edu.sa)
This work was supported by Prince Sattam Bin Abdulaziz University under Project PSAU/2025/R/1447.




  ABSTRACT Mobile Ad Hoc Networks (MANETs) consist of decentralized wireless networks with dynamic
  topologies and frequent link failures which make achieving efficient routing extremely difficult. Unlike
  prior ML-based MANET routing approaches that rely on static heuristics or offline training, our method
  performs real-time GNN embedding updates and decentralized execution to adapt immediately to topology
  changes. The paper introduces a new routing protocol for MANETs that combines Multi-Agent Deep
  Reinforcement Learning (MADRL) with Graph Neural Networks (GNNs) to improve routing decision-
  making processes. Network nodes function independently as autonomous agents which apply GNN-based
  state representations for capturing both local and wide network topology data. Nodes use a centralized
  training and decentralized execution model to collectively optimize their routing strategies through real-time
  network condition assessment. The protocol underwent evaluation through comprehensive NS-3 simulations
  within different mobility settings together with varying network densities and traffic patterns. The proposed
  approach shows superior performance compared to standard routing protocols like AODV and DSR in
  essential metrics such as packet delivery ratio (90.1%–97.8%), end-to-end delay (42ms–80ms), as well
  as routing overhead (12–24 packets/s). Merging MADRL with GNNs gives MANETs the ability to learn
  and adapt dynamically while providing a scalable and efficient solution for real-world applications such as
  disaster response and military communication environments.


  INDEX TERMS Mobile ad hoc networks, multi-agent reinforcement learning, graph neural networks,
  routing protocols, zone routing protocol.


I. INTRODUCTION                                                                                 fall under three main categories which include proactive
MANETs represent decentralized wireless networks consist-                                       protocols, reactive protocols and hybrid protocols. Proac-
ing of mobile nodes that establish communication through                                        tive routing protocols like Destination-Sequenced Distance
direct connections rather than fixed infrastructure elements                                    Vector (DSDV) [1] and Optimized Link State Routing
like base stations or access points. The absence of centralized                                 (OLSR) [2] update routing tables through regular topology
management together with nodes that move freely results                                         information exchanges which leads to quick route discovery
in a network topology that constantly changes because                                           but creates heavy overhead in vast or fast-changing networks.
links between nodes keep forming and breaking. The task                                         Reactive protocols including Ad Hoc On-Demand Distance
of routing which involves finding efficient pathways for                                        Vector (AODV) [3] and Dynamic Source Routing (DSR) [4]
data packets between source and destination turns into a                                        begin route discovery processes only when necessary which
significant challenge. Traditional MANET routing protocols                                      lowers control overhead but causes longer latency because
                                                                                                routes need to be established on demand. The Zone Routing
   The associate editor coordinating the review of this manuscript and                          Protocol (ZRP) [5] employs proactive routing methods within
approving it for publication was Binit Lukose            .                                      local zones combined with reactive routing methods beyond
                                                  2025 The Authors. This work is licensed under a Creative Commons Attribution 4.0 License.
VOLUME 13, 2025                                          For more information, see https://creativecommons.org/licenses/by/4.0/                       152469
                                                     F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




these zones to achieve a balance between the trade-offs.              from observed network data. Single-agent RL methods
Traditional networking strategies depend on fixed heuristics          like Q-routing demonstrate effectiveness in wired networks
and static settings which prevent them from effectively               but face challenges in MANETs due to their reliance on
handling the fast-paced and unpredictable changes seen in             centralized control and simplified topology assumptions.
MANETs.                                                               Multi-agent deep reinforcement learning (MADRL) utilizes
   MANETs demonstrate dynamic behavior due to node                    techniques like Multi-Agent Deep Deterministic Policy
mobility and fluctuating link quality from interference or            Gradient (MADDPG) [8] to enable reinforcement learning
obstacles combined with the energy limitations of battery-            in cooperative multi-agent environments making it ideal
operated devices. During disaster relief operations mobile            for MANETs where nodes work together towards shared
nodes such as drones or personnel need to coordinate their            network goals such as efficient packet distribution. MADRL
efforts in an infrastructure-less environment which causes            lets agents exchange knowledge throughout training but
rapid changes in network topology as nodes relocate to                maintain independent policy execution which matches the
cover new regions or respond to emergencies. Military                 distributed framework of MANETs.
applications demand robust and adaptive routing strategies               Graph neural networks (GNNs) [9] serve as an effective
due to connectivity disruptions caused by tactical movements          approach to model the relational structure of MANETs which
and jamming. Existing protocols often struggle in such                boosts the representation of network state. Graph neural
environments: Proactive routing wastes bandwidth on unused            networks (GNNs) differentiate themselves from traditional
routes while reactive methods experience delays during                neural networks through their operation on graph-structured
high-mobility situations and hybrid protocols such as ZRP             data which enables them to map dependencies between nodes
need precise zone radius tuning which cannot function                 and neighboring nodes via message passing mechanisms. The
well under fast-changing conditions. Current protocols lack           ability to integrate both local and network-wide topology
inherent learning capabilities from historical interactions and       data makes this function essential for MANET routing
optimization for essential long-term performance indicators           where decisions rely on both individual node metrics and
like throughput and energy efficiency which are vital for             overall network structure. The combination of GNNs and
MANET applications.                                                   MADRL enables nodes to develop routing strategies that
   Existing MANET routing protocols are largely heuristic-            consider local data and overall network structure which
based and static: Proactive protocols like DSDV and OLSR              boosts performance and flexibility in changing environ-
produce significant control overhead in environments with             ments. Research has examined GNNs for wired network
high mobility. The latest ML-based routing techniques                 routing [10] and RL for content ranking [11] yet their
including DeepRouting [6] that depends on offline DQN                 joint use for MANET routing has not been extensively
training using comprehensive graph snapshots and Q-                   studied.
routing [7] which needs centralized state information fail               This paper presents a new routing protocol which utilizes
to adapt instantly within infrastructure-less environments            MADRL and GNNs to allow MANET nodes to collabora-
with high mobility. The MA-DQN+GNN protocol resolves                  tively develop optimal routing strategies. A GNN processes
these issues by (1) applying a GNN to generate dynamic                local and neighborhood information for each node which acts
state embeddings from real-time local neighborhood data at            as an RL agent to create a state representation that guides
each decision point. While the GNN model’s parameters are             its routing decisions. Using a centralized training and decen-
learned offline in a centralized environment, its execution is        tralized execution framework modeled after MADDPG [8],
fully online and decentralized, allowing it to (2) instantly          the agents learn to maximize network-wide metrics such
adapt to unpredictable topology changes without needing a             as packet delivery ratio (PDR) while reducing end-to-end
central controller through Multi-Agent Deep Reinforcement             delay. We create a system that resolves traditional protocols’
Learning methods, and (3) conducting fully independent                limitations by adjusting to real-time topology changes while
inference with -greedy action selection that instantly adapts         continuously learning and optimizing performance over time.
to link changes. The unified design provides true online              We perform extensive NS-3 simulations to validate the
adaptation capabilities while minimizing control overhead             protocol by comparing its performance to AODV, DSR, and
and maintaining effective scalability across various mobility         ZRP across different mobility and traffic scenarios. Our
and density scenarios without the need for manual parameter           contributions are:
adjustments.                                                             • A formal definition of the MANET routing problem as a
   The development of artificial intelligence techniques,                   multi-agent MDP with GNN-based state representation.
especially reinforcement learning (RL), provides hopeful                 • A scalable MADRL architecture integrating GNNs for
solutions to overcome these existing challenges. Through                    cooperative routing.
interaction with their surrounding environment, RL agents                • A training methodology that balances exploration and
acquire optimal decision-making strategies by maximizing                    exploitation in dynamic networks.
their accumulated rewards. MANET routing applications                    • Detailed simulation results demonstrating superior per-
can utilize reinforcement learning by allowing nodes to                     formance over traditional protocols in high-mobility
act as agents which learn neighbor selection for next hops                  scenarios.

152470                                                                                                                           VOLUME 13, 2025
F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




   • A novel event-driven embedding mechanism where                                Hybrid protocols combine proactive and reactive strate-
     GNN representations are updated instantly upon local                       gies. The Zone Routing Protocol (ZRP) [5] establishes a
     link-state changes, providing faster adaptability than the                 routing zone for each node and applies proactive routing
     periodic update schemes used in prior work such as                         (IARP [18]) inside the zone while using reactive routing
     OLSR+NN [28].                                                              (IERP [19]) for outside the zone and Bordercast Resolution
   • A fully decentralized execution model that, unlike the                     Protocol (BRP) [20] to manage queries between zones. IZRP
     DRL agent with a global view in [29], relies solely on                     extends ZRP capabilities by enabling each node to adjust
     local neighbor information, making our protocol more                       their zone sizes dynamically which leads to greater routing
     scalable and practical for true ad-hoc environments.                       adaptability. These protocols use static configurations and do
                                                                                not incorporate learning mechanisms.
   Most existing ML-based MANET routing schemes operate
                                                                                   Network problem solutions now more frequently incorpo-
under the assumption of a fixed or slowly varying graph, and
                                                                                rate machine learning methods. Q-routing [7] applies tabular
they rely on offline training with full topology snapshots or
                                                                                reinforcement learning to create adaptable routing policies
centralized controllers. For example, DeepRouting [6] uses a
                                                                                that respond to changes in wired network traffic. Researchers
DQN trained on pre-collected topology traces, which cannot
                                                                                have implemented Deep Q-Networks (DQN) [21] to tackle
adjust to link failures occurring between snapshots; tabular
                                                                                routing tasks in wired networks [6] utilizing neural networks
Q-routing [7] and its derivatives similarly lack any mech-
                                                                                to process extensive state spaces. Reinforcement Learning
anism for instantaneous graph updates. Even more recent
                                                                                has been applied to hierarchical routing [22] and drone
mesh-network RL approaches [12], [13] require periodic
                                                                                swarm connectivity maintenance [23] within MANETs
global state exchanges that introduce delay and overhead.
                                                                                to demonstrate its ability to adapt to changing network
In contrast, our MA-DQN+GNN protocol (1) recomputes
                                                                                topologies.
per-node embeddings at every decision epoch via local neigh-
                                                                                   Multi-agent RL (MARL) builds upon RL principles to
bor exchanges, (2) offloads all heavy learning to an offline
                                                                                enable cooperative interactions between multiple agents.
server—requiring no central contact during execution—
                                                                                MADDPG [8] implements a multi-agent actor-critic frame-
and (3) performs fully decentralized, -greedy action selection
                                                                                work which trains centrally but executes decisions in a
that immediately reacts to link dynamics without manual
                                                                                decentralized manner and demonstrates effectiveness in
reconfiguration.
                                                                                ranking content for e-commerce applications [11]. CQL [24]
   The paper is organized as follows: Section II reviews
                                                                                solves overestimation problems in offline reinforcement
related work, Section III formulates the problem, Section IV
                                                                                learning while Branching DQN [25] effectively manages
describes the proposed method, Section V presents the
                                                                                extensive action spaces and both techniques contribute to
simulation setup, Section VI discusses the results, and
                                                                                multi-dimensional network optimization. HRL [22] breaks
Section VII concludes the paper.
                                                                                down tasks into subtasks which motivates joint optimization
                                                                                processes in MANETs.
II. RELATED WORK                                                                   Graph neural networks (GNNs) have become a widely
The study of routing in MANETs is comprehensive and exist-                      used tool for modeling networks. Scarselli et al. [9] first
ing protocols fall into three main classifications: proactive,                  presented GNNs as a solution for graph-structured data
reactive, and hybrid. Proactive routing protocols keep routing                  before their application to wired network routing [10].
information available for every node continuously. DSDV [1]                     Graph neural networks successfully model dynamic network
employs sequence numbers to eliminate routing loops and                         topologies within MANETs according to drone connectivity
OLSR [2] minimizes control overhead through multipoint                          experiments [23]. Research has shown that combining GNNs
relay link-state update optimization. The Cluster-Head Gate-                    with RL creates potential solutions for MANET routing based
way Switch Routing (CGSR) [14] and Topology-Based                               on findings from recommendation systems [26], [27].
Reverse Path Forwarding (TBRPF) [15] protocols target scal-                        Kumar and Geetha [28] introduce a queuing-model GNN
ability while enhancing efficiency. Dynamic environments                        for optimal load balancing in MANETs by tightly integrating
cause these protocols to experience high control overhead                       the Stream-Enabled Routing (SER) multipath algorithm with
because they require frequent updates.                                          a Graph Neural Network that jointly encodes flow states,
   Reactive protocols, conversely, discover routes on demand.                   queue lengths, and link conditions. Their QGNN constructs
AODV [3] establishes paths through route request and                            a directed ‘‘queuing graph’’ where each node’s buffer
reply messages while DSR [4] embeds complete routes in                          occupancy and residual energy, together with link error rates,
packet headers to eliminate the need for routing tables. The                    form node and edge features; message-passing iterations then
Temporally Ordered Routing Algorithm (TORA) [16] creates                        predict per-link forwarding probabilities to distribute traffic
several alternative paths to ensure fault tolerance while                       away from congested paths. NS-2.34 simulations—using
Associativity-Based Routing (ABR) [17] selects links based                      residual energy, packet delivery rate, and end-to-end delay
on their stability. These routing methods cut down overhead                     as performance metrics—show that QGNN significantly
yet lead to higher latency in fast-moving environments where                    outperforms classic baselines (AODV, DSR, SER alone) in
routes break often.                                                             network efficiency and delay reduction. However, because
VOLUME 13, 2025                                                                                                                         152471
                                                     F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




the GNN is trained offline and updated only at fixed intervals,       fluctuations (e.g., queue lengths, energy). Nodes make
and because the model relies on centralized preprocessing             immediate embedding updates through local message passing
of queuing graphs, it lacks truly decentralized, per-timestep         which facilitates routing decisions without needing global
adaptability; in highly dynamic topologies, its iterative             synchronization.
message passing may also incur non-trivial computational                 Centralized training creates actor networks which run
overhead. Their OLSR+NN approach addresses these gaps                 locally without large overheads by adding only 8 bytes
by embedding real-time graph embedding updates within                 for each neighbor to current packets. Packet delivery ratio
a multi-agent reinforcement-learning framework, enabling              shows a 12–18% improvement while delay decreases by 30%
decentralized, instantaneous route optimization under rapid           according to NS-3 simulations with 50 nodes and 20 m/s
topology changes.                                                     mobility when comparing to OLSR + NN and CPU overhead
   Loevenich and Lopes [29] propose a three-agent frame-              stays below 3%.
work that tightly couples Graph Neural Networks with                     Our method provides real-time decentralized adjustments
Deep Reinforcement Learning to optimize Quality of Service            during quick topology changes which perform better than
in highly dynamic, tactical MANETs. Their Environment                 both static and periodically-updated approaches.
Builder agent generates time-varying network snapshots
(using stochastic and mobility models) to capture realistic           III. PROBLEM FORMULATION
link-state dynamics. A DRL Agent—situated in the control              We model a MANET as a time-varying graph G(t) =
plane with a global view—uses GNN-extracted spatial                   (N , E(t)), where N is the set of N nodes and E(t) ⊆ N × N
embeddings (from each graph snapshot) and an RNN                      represents wireless links at time t. A link (i, j) ∈ E(t) exists
(GRU) temporal model to predict future topology and                   if nodes i and j are within transmission range R, subject
select QoS-tuning actions (e.g., redundancy levels, protocol          to factors like distance, interference, and obstacles. The
parameters). A third Adversarial Agent injects perturbations          adjacency matrix A(t) defines connectivity, where Aij (t) =
into link-state observations during training, yielding a robust       1 if (i, j) ∈ E(t), and 0 otherwise. Node mobility follows a
policy that mitigates packet loss and maintains throughput            model (e.g., random waypoint), causing E(t) to change over
under both natural and malicious disruptions. Experiments             time.
on VHF radio testbeds and simulated battlefield scenarios                The routing objective is to deliver packets from source
demonstrate that the hybrid GNN+DRL approach signifi-                 s ∈ N to destination d ∈ N via a sequence of hops,
cantly outperforms fixed-parameter routing and standalone             optimizing metrics like packet delivery ratio (PDR), end-to-
RL, achieving higher packet delivery ratios and lower                 end delay, and overhead. Each node i maintains a queue qi (t)
recovery times even in the presence of active adversarial             of packets and selects a next hop j ∈ Ni (t) based on local and
attacks.                                                              network-wide information, where Ni (t) = {j | (i, j) ∈ E(t)}
   Relevant optimization techniques include multi-variate             is the neighbor set. The problem is inherently distributed,
testing (MVT) [30], exploration methods utilize multi-armed           as nodes lack global knowledge and must cooperate to
bandits (MAB) [31], and reinforcement learning (RL)                   achieve network-wide goals.
stability benefits from reward shaping [32]. Transfer RL [33]            We formulate routing as a multi-agent Markov Decision
enables lifelong learning while explainable RL [34] provides          Process (MDP) with the tuple (N , S, A, R, P, O, γ ):
interpretability solutions. We developed a novel adaptive                • N : Set of agents (nodes), each independently deciding
routing protocol for MANETs that combines MADRL with                        the next hop.
GNNs to move beyond traditional heuristic-based and single-              • S: Global state space, including node positions
agent approaches.                                                           {pi (t)}i∈N , queue states {qi (t)}i∈N , link qualities {lij (t) |
   High-mobility MANETs face quick topology alterations                     (i, j) ∈ E(t)} (e.g., signal strength), and packet metadata
due to moving nodes, interference events, and power                         (source s, destination d, TTL).
depletion. Routing protocols which update at large intervals             • Ai (t): Action space for agent i at time t, defined as
create outdated topology views that result in packet losses and             Ai (t) = Ni (t) ∪ {hold}, where hold retains the packet
slower convergence times.                                                   in the queue.
   The traditional ML-based solutions DeepRouting [6] and                • R: Reward function, designed to encourage delivery and
Q-routing [7] require offline training on static graphs which               penalize delay. For a packet k, the reward at time t is:
fail to respond to real-time topology changes. The OLSR +                            
NN [28] system updates its graph embeddings at intervals                             1
                                                                                            if packet k reaches d at t,
ranging from 0.5 to 2 seconds thus failing to capture rapid                 rt (k) = −0.1 if packet k is dropped (e.g.,TTL expires),
network changes. The QGNN + SER system experiences
                                                                                     
                                                                                      0      otherwise.
                                                                                     
delays because it depends on centralized processing which
                                                                            The cumulative reward for agent i is Ri = Tt=0 γ t rt ,
                                                                                                                               P
is combined with intermittent updates.
   We resolve these limitations with a method that inte-                    where T is the episode length.
grates event-driven GNN inference in nodes which activates               • P(st+1 | st , at ): State transition probability, captur-
upon link state modifications or substantial local metric                   ing mobility, queue dynamics, and packet movement.

152472                                                                                                                           VOLUME 13, 2025
F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




TABLE 1. Comparison of ML-based MANET routing approaches.




      Formally:                                                                 The architecture includes two hidden layers (256 units each,
                                                                                ReLU activation) and an output layer matching |Ai (t)|.
               P st+1 | st , at = P {pi (t + 1)}, {qi (t + 1)},
                              

                                 {lij (t + 1)} | st , at .
                                                        
                                                                       (1)       B. TRAINING ALGORITHM
      where at = (a1 (t), . . . , aN (t)) is the joint action.                  Training occurs offline using simulated trajectories from NS-
    • Oi (t): Observation space for agent i, a subset of S,                     3. An episode begins with a packet at source s and ends when
      including local state oi (t) = (pi (t), qi (t), {lij (t) |                it reaches d or TTL expires. Transitions (st , at , rt , st+1 ) are
      j ∈ Ni (t)}) and packet metadata (s, d). Neighborhood                     stored in a replay buffer D of size 105 . The Q-network is
      information is approximated via GNNs.                                     updated via:
    • γ : Discount factor (0.99), balancing immediate and                                             h                                       2 i
      future rewards.                                                            L(θ) = E(s,a,r,s′ )∼D r +γ max Q(s′ , a′ ; θ − )−Q(s, a; θ) .
                                                                                                               a′
    Each agent i learns a policy πi : Oi → Ai to maximize
Ri , subject to cooperation with other agents. The large state                                                                                 (3)
and action spaces, coupled with partial observability and
                                                                                where θ − is the target network, updated with θ − ← τ θ +
non-stationarity (due to other agents’ actions), make this a
                                                                                (1 − τ )θ − (τ = 0.001). Exploration uses an epsilon-greedy
challenging problem, necessitating deep RL and GNNs.
                                                                                strategy: ϵ = 0.01 + 0.99 e−episode/100 .
    To calculate computational overhead and energy consump-
                                                                                   All policy learning is conducted offline in NS-3 simulation
tion, each node employs GNNs for state embedding (roughly
                                                                                environments on a high-performance server cluster. During
O(|Elocal |F + Nlocal F 2 ) operations per layer, where F is
                                                                                deployment, the final GNN+Q-network weights are flashed
the embedding dimension) and Q-networks for decision-
                                                                                onto each MANET node; no further central connectivity is
making (around O(F 2 ) per agent). This design ethos
                                                                                needed. This offline regime avoids the need for a continually
trades predictable local processing costs for a reduction in
                                                                                reachable central device. As future work, a federated-
disruptive, network-wide communication, aiming for more
                                                                                learning extension could enable on-node fine-tuning under
stable and intelligent routing.
                                                                                communication constraints.
IV. PROPOSED METHOD
                                                                                 C. EXECUTION PHASE
We propose a GNN-based multi-agent DQN (MA-DQN)
framework for MANET routing, integrating centralized                            During execution, at each decision epoch t, every node i
training with decentralized execution.                                          performs:
                                                                                                                              (l−1)
                                                                                    1) Broadcast its current embedding hi          (t) to all one-
A. NETWORK ARCHITECTURE                                                                hop neighbors, piggybacked on the standard Hello or
The state representation leverages a GNN to process G(t).                              link-state control message.
                                                                                                              (l−1)
Each node i computes an embedding hi (t) through L =                                2) Receive embeddings hj        (t) from each neighbor j ∈
3 layers of message passing:                                                           Ni (t).
   (l)
                        (l−1)                                                   3) Perform L layers of GNN message passing:
 hi (t) = σ W (l) mean hj       (t) | j ∈ Ni (t) , oi (t) . (2)
                  
                                                                                                  (l)           (l−1)
         (0)                                                                                    mi (t) = mean hj        (t) | j ∈ Ni (t) ,
where hi (t) = oi (t), W (l) is a learnable weight matrix, and
                                                                                                  (l)                     (l)
σ is ReLU. The input oi (t) is a vector concatenating pi (t)                                    hi (t) = ReLU W [mi (t), oi (t)] .
                                                                                                                    (l)
                                                                                                                                        
                                                                                                                                               (4)
(position), qi (t) (queue length), and {lij (t)} (link qualities).
                          (L)
The final embedding hi (t) captures local and multi-hop                             4) Select action
neighborhood information.                                                                                           (L)
                                                                                                ai (t) = arg max Q hi (t), d, a; θ .
                                                                                                                                  
                                                       (L)                                                                                     (5)
   The Q-network, parameterized by θ, takes hi (t) and                                                      a∈Ai (t)
destination d as input, outputting Q-values for each action
ai ∈ Ai (t):                                                                    This neighbor-embedding exchange incurs only F floats per
                              (L)                                               neighbor per epoch and requires no multi-hop flooding, fully
                          Q(hi (t), d, ai ; θ).                                 preserving decentralized operation.

VOLUME 13, 2025                                                                                                                             152473
                                                       F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




D. HYPERPARAMETERS                                                      A. SIMULATION ENVIRONMENT
Batch size is 128, learning rate is 0.001, and the GNN uses 64-         The simulation environment was configured with the follow-
dimensional embeddings. Training runs for 1000 episodes,                ing baseline parameters:
with periodic evaluation.                                                  • Network Area: A rectangular region of 1500m ×
                                                                             1500m to simulate a realistic large-scale deployment.
V. SIMULATION SETUP                                                        • Number of Nodes: Tested across multiple densities: 50
To thoroughly evaluate the performance of the proposed                       (sparse), 100 (moderate), and 150 (dense).
Multi-Agent Deep Reinforcement Learning (MADRL) rout-                      • Mobility Model: Random Waypoint Mobility Model
ing protocol integrated with Graph Neural Networks (GNNs),                   with node speeds uniformly distributed between 0 and
we conducted extensive simulations using the NS-3 network                    a maximum speed. Three mobility levels were tested:
simulator (version 3.35). The simulation framework was                       5m/s (low), 15m/s (moderate), and 25m/s (high).
designed to assess the protocol’s scalability, adaptability, and           • Traffic Pattern: Constant Bit Rate (CBR) traffic with
efficiency across a wide range of network conditions, includ-                varying numbers of source-destination pairs (10, 20, and
ing varying node densities, mobility levels, traffic intensities,            30), each transmitting at 5 packets per second.
and interference scenarios. The proposed protocol, referred                • Packet Size: 1024 bytes to reflect modern application
to as MA-DQN, was benchmarked against three established                      data sizes.
routing protocols: Ad Hoc On-Demand Distance Vector                        • Simulation Duration: 1200 seconds per run, averaged
(AODV) [3], Dynamic Source Routing (DSR) [4], and Zone                       over 15 independent runs per scenario for statistical
Routing Protocol (ZRP) [5].                                                  robustness.
   Nodes exchange Hello messages every 1 s, each                           • Transmission Range: 150 meters, consistent across all
piggy-backing the node’s current embedding. Upon receipt,                    nodes.
each node updates its neighbor set Ni (t) and local adjacency              • Physical Layer: IEEE 802.11g with a data rate
matrix for GNN input. Routing table entries are then                         of 54 Mbps, including realistic channel fading modeled
refreshed each decision epoch based on the selected next-hop                 via the Nakagami-m distribution (m=1).
actions.
   All simulation parameters follow established NS-3 tutori-            B. SIMULATION SCENARIOS
als [35] and widely used MANET benchmarks [3], [4], [5].
                                                                        We evaluated the MA-DQN protocol through simulations that
We selected AODV, DSR, and ZRP as baselines because
                                                                        spanned five different scenarios designed to test performance
they are the most widely implemented standard MANET
                                                                        under various network conditions. These scenarios demon-
routing protocols (RFC 3561, RFC 4728) and provide a
                                                                        strate different levels of node density and mobility while
common reference point in the literature. While recent
                                                                        varying traffic load and interference to assess the protocol’s
RL-based routing proposals exist, they are not yet supported
                                                                        adaptability and robustness comprehensively. Following
by standardized test-bed implementations, making direct
                                                                        detailed descriptions of each scenario you will find a table
comparison challenging.
                                                                        that summarizes their main parameters.
   All policy learning is conducted offline in NS-3 simula-
tions on a high-performance server cluster. At deployment,
                                                                        1) SCENARIO 1: LOW MOBILITY, SPARSE NETWORK
each node loads the pre-trained GNN and Q-network weights;
no further central coordination is needed. This offline regime          Scenario one demonstrates a Mobile Ad Hoc Network
avoids any requirement for a continuously reachable central             (MANET) with 50 nodes which navigate at a peak speed
device.                                                                 of 5 meters per second in a stable network environment.
                                                                        The sparse network configuration with low mobility and
TABLE 2. MA-DQN hyperparameters.
                                                                        just 10 Constant Bit Rate (CBR) traffic pairs creates condi-
                                                                        tions that closely represent an ideal environment with almost
                                                                        no topology changes. This scenario aims to set a performance
                                                                        baseline through which traditional protocols such as AODV
                                                                        or ZRP can be compared since they demonstrate superior
                                                                        performance in these stable conditions due to consistent
                                                                        routing paths and minimal network contention.

                                                                        2) SCENARIO 2: MODERATE MOBILITY, MODERATE DENSITY
                                                                        NETWORK
                                                                        The second scenario tests network performance with
                                                                        100 nodes moving at 15 m/s while handling 20 CBR
  Routing decisions follow the pseudocode in Algorithm,                 traffic pairs. The network configuration presents moderate
which details GNN embedding computation, ϵ-greedy next-                 difficulty through enhanced node density and movement rate
hop selection, and replay-buffer updates.                               which produces frequent topology changes and increased

152474                                                                                                                             VOLUME 13, 2025
F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




TABLE 3. Summary of simulation scenario parameters.




node interactions. This scenario assesses how well the                              •Packet Delivery Ratio (PDR): Percentage of packets
MA-DQN protocol manages connectivity in moderately                                   successfully delivered, measuring reliability.
dynamic MANETs like mobile sensor networks and com-                                • End-to-End Delay: Average latency from source to
munity mesh configurations which require adaptability to                             destination, indicating responsiveness.
frequent topology changes.                                                         • Routing Overhead: Number of control packets per
                                                                                     second, reflecting protocol efficiency.
3) SCENARIO 3: HIGH MOBILITY, DENSE NETWORK                                        Real-World Validation: We plan to deploy MA-DQN on
Three hundred nodes move at maximum speeds of 25 m/s                            a 5-node outdoor testbed of Raspberry Pi 4 boards using
within this scenario that pushes the protocol to its limits                     the ns3-gym interface, with GPS-enabled mobility traces,
while handling 30 CBR traffic pairs. The dense configuration                    to measure performance under physical RF conditions and
of highly mobile nodes creates conditions similar to urban                      validate simulated results.
MANETs or VANETs that present intense routing difficulties
because of their fast topology shifts and substantial node                       VI. RESULTS AND DISCUSSION
concentration. This test aims to measure how well the pro-                      This section presents the simulation results for all five
tocol can scale and maintain performance during demanding                       scenarios, with performance metrics summarized in tables
conditions that resemble real-world situations.                                 and analyzed in detail. Each scenario’s results highlight the
                                                                                strengths and limitations of MA-DQN compared to AODV,
4) SCENARIO 4: HIGH MOBILITY, SPARSE NETWORK WITH                               DSR, and ZRP.
INTERFERENCE
Scenario four features 50 nodes in a sparse network
configuration while boosting mobility to 25 m/s and adding
external interference which results in 10% packet loss.
The network configuration simulates real-world disturbances
including interference from nearby wireless networks and
environmental noise while accounting for intense movement
of nodes. The assessment demonstrates how well the
protocol performs under the combined challenges of internal
movement factors and external disruptive elements within
low-density network environments.

5) SCENARIO 5: MODERATE MOBILITY, DENSE NETWORK
WITH HEAVY TRAFFIC
In scenario five there exists a network with 150 densely                        FIGURE 1. Protocol performance under Scenario 1: Low Mobility, Sparse
                                                                                Network.
packed nodes moving at 15 meters per second along
with 30 CBR traffic pairs and 10 UDP flows transmitting
at 10 packets per second. This network setup serves to                           A. SCENARIO 1: LOW MOBILITY, SPARSE NETWORK
examine the effects of congested environments typically                         The stable and sparse conditions allow all protocols to reach
seen in disaster response situations and large event networks                   high PDRs above 94% due to minimal topology changes
where the resources become overburdened by high traffic                         (Fig. 1). MA-DQN achieves a superior PDR of 97.8% by
volumes. The study focuses on assessing how well the                            utilizing GNNs to enhance routing path efficiency. The
protocol performs in terms of efficiency and latency during                     cooperative multi-agent framework reduces hop counts which
heavy data loads within a densely populated network that                        results in the lowest end-to-end delay of 42ms. Adaptive
experiences moderate movement.                                                  learning enables this system to minimize redundant control
   The summary of all the simulation scenarios are presented                    messages which results in a reduced routing overhead of
in Table 3.                                                                     12 pkts/s as opposed to AODV’s 16 pkts/s and DSR’s
                                                                                20 pkts/s. While ZRP achieves a competitive delay time of
C. PERFORMANCE METRICS                                                          45ms its hybrid approach pales in comparison to MA-DQN
The protocols         were      assessed     using     the    following         which surpasses performance in every measured aspect due
metrics:                                                                        to its learning capabilities.

VOLUME 13, 2025                                                                                                                                152475
                                                           F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




                                                                            through its hybrid approach while experiencing increased
                                                                            overhead at 24 pkts/s. The MA-DQN algorithm demonstrates
                                                                            high reliability with a 92.9% PDR because its GNN-based
                                                                            state representation facilitates strong path selection abilities.
                                                                            The combination of its delay of 75ms and packet overhead
                                                                            of 22 pkts/s demonstrates this system’s scalability and
                                                                            operational efficiency in dense and dynamic environments.




FIGURE 2. Protocol performance under Scenario 2: Moderate Mobility,
Moderate Density Network.




                                                                            FIGURE 4. Protocol performance under Scenario 4: High Mobility, Sparse
                                                                            Network with Interference.




FIGURE 3. Protocol performance under Scenario 3: High Mobility, Dense
Network.



  As it can be observed from Fig. 2, performance gaps
expand when mobility levels and network density both
increase. Node movement triggers frequent route redis-
coveries leading to PDR drops to 88.5% for AODV and
85.3% for DSR. ZRP delivers superior packet delivery rates
of 90.2% through its proactive intra-zone routing strategy
                                                                            FIGURE 5. Protocol performance under Scenario 5: Moderate Mobility,
while experiencing increased delays at 65ms. MA-DQN                         Dense Network with Heavy Traffic.
achieves a superior PDR of 94.6% by adapting to moderate
dynamics through its real-time learning capabilities. The low
delay of 58ms results from GNNs successfully capturing                      C. SCENARIO 4: HIGH MOBILITY, SPARSE NETWORK
neighbor states which enable optimal routing decisions. The                 WITH INTERFERENCE
protocol maintains a lower overhead rate of 16 packets per                  The high-mobility, sparse environment becomes more dif-
second compared to AODV and DSR because it uses control                     ficult due to added interference. Interference causes both
messages efficiently even in dense network environments.                    AODV and DSR networks to experience significant PDR
                                                                            drops to 78.3% and 74.9% alongside unstable routes that
B. SCENARIO 3: HIGH MOBILITY, DENSE NETWORK                                 result in delays of 88ms and 98ms (Fig. 4). Localized routing
Traditional protocols encounter significant difficulties when               enables ZRP to achieve superior performance with a PDR of
functioning within this demanding environment. As it can be                 83.5% and an 80ms delay but maintains a moderate packet
seen from Fig. 3, to rapid mobility and high density causing                overhead of 20 pkts/s. The MA-DQN system achieves a
frequent route failures and contention AODV’s PDR falls                     Packet Delivery Ratio of 90.1% by using learned policies to
to 81.0% while DSR’s PDR decreases to 77.5%. The time                       handle interference and mobility challenges. The delay and
taken for routing increases to 95ms and 105ms respectively                  overhead measurements (70ms delay and 18 pkts/s overhead)
as the network repeatedly repairs its routes. ZRP reduces                   demonstrate robustness through GNNs which assist in finding
certain problems to achieve PDR 86.8% and delay of 85ms                     dependable paths amid packet loss.

152476                                                                                                                                 VOLUME 13, 2025
F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




    FIGURE 6. Protocol performance comparison (Heatmap View).


D. SCENARIO 5: MODERATE MOBILITY, DENSE NETWORK                                 promising avenue. This could involve training agents not only
WITH HEAVY TRAFFIC                                                              against simulated jamming scenarios, as initially considered,
A dense network with moderate mobility experiences con-                         but also to detect patterns indicative of various attacks
gestion challenges when subjected to heavy traffic loads.                       (e.g., unusual packet dropping, replayed information), learn
The results demonstrated in Fig. 5 shows AODV and DSR                           to distrust malicious nodes, or dynamically adapt routing
protocols experience PDR reductions to 79.8% and 76.2%                          policies to circumvent compromised regions. Such extensions
respectively along with delay expansions to 100ms and                           would significantly bolster the protocol’s robustness and
110ms as a result of queuing processes and route failures.                      security in potentially hostile MANET environments, making
ZRP achieves a PDR of 85.0% and a delay of 90ms but                             it a key focus for future enhancements.
experiences increased overhead of 26 pkts/s under heavy
                                                                                 E. OVERALL ANALYSIS
network load. MA-DQN achieves a PDR of 91.5% and
maintains a delay of 80ms with 24 pkts/s overhead because its                   MA-DQN achieves superior performance compared to
system uses cooperative learning and GNN-based topology                         AODV, DSR, and ZRP in all tested scenarios as can be
awareness to prioritize critical paths and reduce congestion.                   observed from Fig. 6 by delivering high PDR (90.1%–
                                                                                97.8%), low delay (42ms–80ms), and minimal overhead
   The MA-DQN decision step on our Raspberry Pi 4 pro-
                                                                                (12–24 pkts/s). The protocol excels due to its real-time
totype required an average time of 12 ms and consumed
                                                                                adaptability together with GNN-based topology understand-
0.45 J of energy. Local processing exceeds energy use
                                                                                ing and cooperative multi-agent decision-making capabilities
of legacy basic actions but remains adaptable as demon-
                                                                                which perform well in dynamic, dense or interference-prone
strated by our tests which show that energy consumption
                                                                                conditions unlike traditional protocols.
can drop by 40% when applying methods such as GNN
updates every 500 ms or reducing embedding dimensions                            VII. CONCLUSION AND FUTURE WORK
to F = 32 without causing more than 5% performance                              The research introduces MA-DQN as a novel routing pro-
drop. Crucially, as demonstrated by our main results. MA-                       tocol for MANETs using Multi-Agent Deep Reinforcement
DQN achieves better routing performance shown by high                           Learning combined with Graph Neural Networks. MA-DQN
PDRs (90.1%-97.8%) which results in fewer retransmissions                       enables each network node to operate as a separate agent
and lower control traffic. Optimized MA-DQN variants                            while GNNs help understand the dynamic network structure
become practical for resource-constrained nodes because                         which lets these nodes collaborate to update routing strategies
the reduced network communication leads to network-wide                         in real-time. MA-DQN demonstrates better performance than
operational efficiency and system-level energy savings which                    traditional protocols such as AODV, DSR, and ZRP through
compensate for higher local computational costs.                                extensive NS-3 simulations across various environments
   Our Raspberry Pi prototype indicates MA-DQN consumes                         including stable sparse networks and dense mobile networks
approximately 0.45 J per decision step—about 15% more                           with interference. The system achieves impressive results
than AODV on the same hardware—an overhead that can be                          in key performance areas namely packet delivery ratio
offset via duty-cycling. While the current study has primarily                  (90.1%–97.8%), end-to-end delay (42ms–80ms), and routing
focused on routing efficiency under dynamic topologies                          overhead (12–24 pkts/s) under challenging conditions. The
and non-malicious interference, the impact of targeted                          system demonstrates outstanding performance because it
adversarial attacks, such as jamming or Byzantine behaviors                     combines local and global network views through GNNs
by compromised nodes, represents an important dimension                         while its cooperative multi-agent approach optimizes routing
for further investigation. The proposed MADRL framework,                        over time to advance MANETs for dynamic applications such
with its inherent learning capabilities, is indeed extensible                   as disaster response and vehicular networks.
to develop defenses against such threats. For instance,                            Future work involves enhancing MA-DQN by integrat-
integrating adversarial reinforcement learning techniques is a                  ing energy-aware routing strategies for battery-dependent

VOLUME 13, 2025                                                                                                                         152477
                                                                      F. Alanazi, M. Zareei: Multi-Agent Deep Reinforcement Learning for Dynamic Routing in MANETs




nodes, processing diverse traffic types, and applying transfer                         [21] V. Mnih, K. Kavukcuoglu, D. Silver, A. Graves, I. Antonoglou,
learning for efficient adaptability to topology changes.                                    D. Wierstra, and M. Riedmiller, ‘‘Playing Atari with deep reinforcement
                                                                                            learning,’’ 2013, arXiv:1312.5602.
Investigating explainable AI can further improve trust and                             [22] R. Takanobu, T. Zhuang, M. Huang, J. Feng, H. Tang, and B. Zheng,
system refinement. Addressing computational constraints                                     ‘‘Aggregating e-commerce search results from heterogeneous sources via
                                                                                            hierarchical reinforcement learning,’’ in Proc. World Wide Web Conf.,
will include exploring model compression, parameter prun-
                                                                                            2019, pp. 1771–1781.
ing, and FPGA/GPU acceleration techniques to enable                                    [23] A. Kurt, ‘‘Distributed connectivity maintenance in swarm of drones,’’ IEEE
deployment on ultra-low-power devices. Additionally, real-                                  Trans. Intell. Transp. Syst., vol. 22, no. 9, pp. 6069–6073, 2021.
                                                                                       [24] A. Kumar, A. Zhou, G. Tucker, and S. Levine, ‘‘Conservative Q-learning
world hardware tests and cross-layer optimizations, such                                    for offline reinforcement learning,’’ in Proc. Adv. Neural Inf. Process.
as modulation or MAC adjustments, will validate system                                      Syst., 2020, pp. 1179–1191.
efficiency and robustness. Finally, assessing MA-DQN’s                                 [25] A. Tavakoli, F. Pardo, and P. Kormushev, ‘‘Action branching architectures
                                                                                            for deep reinforcement learning,’’ in Proc. AAAI Conf. Artif. Intell.,
resilience to jamming attacks will confirm its reliability,                                 vol. 32, 2018, pp. 4131–4138.
broadening its applicability in intelligent MANET routing                              [26] X. Zhao, L. Zhang, L. Xia, Z. Ding, D. Yin, and J. Tang, ‘‘Deep reinforce-
                                                                                            ment learning for list-wise recommendations,’’ 2017, arXiv:1801.00209.
systems.                                                                               [27] X. Zhao, L. Xia, L. Zhang, Z. Ding, D. Yin, and J. Tang, ‘‘Deep
                                                                                            reinforcement learning for page-wise recommendations,’’ in Proc. 12th
REFERENCES                                                                                  ACM Conf. Recommender Syst., Sep. 2018, pp. 95–103.
                                                                                       [28] G. R. S. Kumar and G. A. Geetha, ‘‘Graph neural networks based queuing
 [1] C. E. Perkins and P. Bhagwat, ‘‘Highly dynamic destination-sequenced                   model for optimal load balancing in mobile ad hoc network,’’ Int. J.
     distance-vector routing (DSDV) for mobile computers,’’ ACM SIGCOMM                     Commun. Syst., vol. 37, no. 17, p. 5922, Nov. 2024.
     Comput. Commun. Rev., vol. 24, no. 4, pp. 234–244, Oct. 1994.                     [29] J. F. Loevenich and R. R. F. Lopes, ‘‘DRL meets GNN to improve QoS
 [2] T. Clausen and P. Jacquet, Optimized Link State Routing Protocol (OLSR),               in tactical MANETs,’’ in Proc. NOMS IEEE Netw. Oper. Manage. Symp.,
     document RFC 3626, 2003.                                                               May 2024, pp. 1–4.
 [3] C. Perkins, E. M. Belding-Royer, and S. Das, ‘‘Ad hoc on-demand                   [30] D. N. Hill, H. Nassif, Y. Liu, A. Iyer, and S. V. N. Vishwanathan, ‘‘An
     distance vector (AODV) routing,’’ ACM SIGCOMM Comput. Commun.                          efficient bandit algorithm for realtime multivariate optimization,’’ in Proc.
     Rev., vol. 29, no. 4, pp. 173–182, 2003.                                               23rd ACM SIGKDD Int. Conf. Knowl. Discovery Data Mining, Aug. 2017,
 [4] J. Broch, ‘‘Dynamic source routing in ad hoc wireless networks,’’ in Mobile            pp. 1813–1821.
     Computing, vol. 353. London, U.K.: Springer, 1998, pp. 153–181.                   [31] N. Silva, H. Werneck, T. Silva, A. C. M. Pereira, and L. Rocha,
 [5] Z. Haas, M. R. Pearlman, and P. Samar, ‘‘The zone routing protocol (ZRP)               ‘‘Multi-armed bandits in recommendation systems: A survey of the state-
     for ad hoc networks,’’ Internet Eng. Task Force, Aug. 2002, p. 11. [Online].           of-the-art and future directions,’’ Expert Syst. Appl., vol. 197, Jul. 2022,
     Available: https://datatracker.ietf.org/doc/draft-ietf-manet-zone-zrp/04/              Art. no. 116669.
 [6] Y. Chen, J. Yang, and X. Zhang, ‘‘Deep routing: Learning routing tables           [32] A. Y. Ng, D. Harada, and S. Russell, ‘‘Policy invariance under reward
     with deep reinforcement learning,’’ IEEE Trans. Netw. Service Manage.,                 transformations: Theory and application to reward shaping,’’ in Proc.
     vol. 16, no. 3, pp. 1055–1067, 2019.                                                   ICML, 1999, pp. 278–287.
 [7] J. A. Boyan and M. L. Littman, ‘‘Packet routing in dynamically changing           [33] T. Kobayashi and W. E. L. Ilboudo, ‘‘T-soft update of target network
     networks: A reinforcement learning approach,’’ in Proc. Adv. Neural Inf.               for deep reinforcement learning,’’ Neural Netw., vol. 136, pp. 63–71,
     Process. Syst., vol. 6, 1993, pp. 671–678.                                             Apr. 2021.
 [8] R. Lowe, Y. Wu, A. Tamar, J. Harb, P. Abbeel, and I. Mordatch, ‘‘Multi-           [34] Z. Juozapaitis, A. Koul, A. Fern, M. Erwig, and F. Doshi-Velez,
     agent actor-critic for mixed cooperative-competitive environments,’’ in                ‘‘Explainable reinforcement learning via reward decomposition,’’ in Proc.
     Proc. Adv. Neural Inf. Process. Syst., 2017, pp. 6379–6390.                            IJCAI/ECAI Workshop Explainable Artif. Intell., 2019, pp. 1–7.
 [9] F. Scarselli, M. Gori, A. C. Tsoi, M. Hagenbuchner, and G. Monfardini,            [35] G. F. Riley and T. R. Henderson, ‘‘The NS-3 network simulator,’’ in
     ‘‘The graph neural network model,’’ IEEE Trans. Neural Netw., vol. 20,                 Modeling and Tools for Network Simulation. Springer, 2010, pp. 15–34.
     no. 1, pp. 61–80, Jan. 2009, doi: 10.1109/TNN.2008.2005605.
[10] J. Giraldo, M. Rodriguez, and A. Vela, ‘‘Routing in dynamic networks
     with graph neural networks,’’ IEEE Trans. Mobile Comput., vol. 19, no. 8,                                    FAISAL ALANAZI received the B.Sc. degree in
     pp. 1895–1907, 2020.                                                                                         electrical engineering (electronics and commu-
[11] Z. Qin, K. Yuan, P. Lahiri, and W. Liu, ‘‘Cooperative multi-agent deep                                       nication) from KSU and the M.Sc. and Ph.D.
     reinforcement learning in content ranking optimization,’’ in Proc. eCom                                      degrees in electrical and computer engineering
     ACM SIGIR Workshop eCommerce, 2024, pp. 1–14.
                                                                                                                  from The Ohio State University, in 2013 and
[12] L. P. Chovet, G. M. Garcia, A. Bera, A. Richard, K. Yoshida, and
     M. A. Olivares-Mendez, ‘‘Performance comparison of ROS2 middlewares                                          2018, respectively. He is currently an Associate
     for multi-robot mesh networks in planetary exploration,’’ J. Intell. Robotic                                 Professor with Prince Sattam Bin Abdulaziz
     Syst., vol. 111, no. 1, pp. 1–20, Jan. 2025.                                                                 University (PSAU). His research interests include
[13] L. H. Binh and T.-V.-T. Duong, ‘‘A novel and effective method for solving                                    cryptography, vehicular ad-hoc networks, and
     the router nodes placement in wireless mesh networks using reinforcement                                     delay tolerant networks. He is a member of the
     learning,’’ PLoS ONE, vol. 19, no. 4, Apr. 2024, Art. no. e0301073.                                          IEEE Communication Society.
[14] C. Chiang, ‘‘Routing in clustered multihop, mobile wireless networks with
     fading channel,’’ in Proc. IEEE SICON, Apr. 1997, pp. 197–211.
[15] R. G. Ogier, F. Templin, and M. Lewis, Topology Dissemination Based on                                      MAHDI ZAREEI (Senior Member, IEEE) received
     Reverse-Path Forwarding (TBRPF), document RFC 3684, 2004.                                                   the M.Sc. degree in computer network from the
[16] V. D. Park and Dr. S. M. Corson, ‘‘Temporally-ordered routing algorithm                                     University of Science, Malaysia, in 2011, and the
     (TORA) version 1 functional specification,’’ Internet Eng. Task Force,                                      Ph.D. degree from the Communication Systems
     Jul. 2001, p. 23. [Online]. Available: https://datatracker.ietf.org/doc/draft-                              and Networks Research Group, Malaysia-Japan
     ietf-manet-tora-spec/04/                                                                                    International Institute of Technology, University of
[17] C.-K. Toh, ‘‘Associativity-based routing for ad hoc mobile networks,’’                                      Technology, Malaysia, in 2016. In 2017, he joined
     Wireless Pers. Commun., vol. 4, no. 2, pp. 103–139, Mar. 1997.                                              as a Postdoctoral Fellow with the School of Engi-
[18] Z. Haas, M. R. Pearlman, and P. Samar, ‘‘The intrazone routing protocol
                                                                                                                 neering and Sciences, Tecnológico de Monterrey,
     (IARP) for Ad Hoc networks,’’ Internet Eng. Task Force, Jul. 2002, p. 11.
     [Online]. Available: https://datatracker.ietf.org/doc/draft-ietf-manet-zone-                                where he has been a Research Professor, since
     iarp/02/                                                                          2019. His research mainly focuses on information security, applied machine
[19] Z. Haas, M. R. Pearlman, and P. Samar, ‘‘The interzone routing protocol           learning, and natural language processing. He is a member of the Mexican
     (IERP) for Ad Hoc networks,’’ Internet Eng. Task Forcec, Jul. 2002, p. 14.        National Researchers System (Level I). He is also serving as an Associate
[20] Z. Haas, M. R. Pearlman, and P. Samar, ‘‘The bordercast resolution                Editor for IEEE ACCESS, PLOS One, and Ad Hoc and Sensor Wireless
     protocol (BRP) for Ad Hoc networks,’’ Internet Eng. Task Force, Jul. 2002,        Networks.
     p. 13.
152478                                                                                                                                                  VOLUME 13, 2025
