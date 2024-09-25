# SUZUKI KASAMI ALGORITHM

**Arya Marda**: 2021102021 | **Vansh Marda**: 2021101089

The Suzuki-Kasami algorithm is a token-based algorithm designed for achieving mutual exclusion in distributed systems. In this algorithm, only the process holding the token can access its critical section.

## **Algorithm Overview**:

1. **Basic Principle**:
   - The algorithm operates on the concept of circulating a single token among processes in the system to ensure mutual exclusion.

2. **Token Acquisition**:
   - Processes request the token by broadcasting a request message to all other processes. The token holder may grant the token if not needed.


4. **Token Structure**:
   - The token comprises a FIFO queue of requesting processes and an array storing sequence numbers of their recent critical section executions.

5. **Request Messages**:
   - Processes send request messages containing their ID and sequence number to request the token.

6. **Data Structures**:
   - Each node maintains a Request Number (RN) array to track the highest sequence number received from other nodes.

7. **Requesting Critical Section**:
   - Processes increment their request number and broadcast a request if they lack the token. If holding the token, they enter the critical section or release it.

8. **Handling Requests**:
   - Upon receiving a request message, a process updates its RN and decides whether to transfer the token.

9.  **Entering Critical Section**:
    - Processes enter their critical section only when possessing the token.

10. **Releasing Critical Section**:
    - After exiting the critical section, a process updates its Last Executed (LN) array and manages the token queue.

11. **Token Transfer**:
    - If the token queue isn't empty, the next process in line receives the token.

## **Code Execution:**

- Compiling the code
    `mpicc sukuzi-kasami.c -o suzuki-kasami`
- Run the code using
   `mpirun -np <total processes> ./suzuki-kasami <simulation time> <wait time for critical section> <initial active processes>`.

- For instance,
   `mpirun -np 8 ./suzuki 30 5 3`.

- The simulation time is limited to 30 seconds with a minimum of 10 seconds, and each process can wait up to 5 seconds before re-requesting access to the critical section.
- Initially, only 3 processes are active.


## **Code Explanation:**

1. **Initialization and Setup**:
    - The code initializes necessary variables, including the rank of each process, the size of the MPI communicator, and structs representing the token and process state.
    - It also sets up MPI data types for sending and receiving token information.
2. **Token Initialization**:
    - The token struct is initialized with initial values for the LN array, Q array, count, and active_process.
3. **Setting Initial State and Token Holder**:
    - Processes are divided into two categories: Inactive and otherwise.
    - A random process is chosen to hold the initial token. This process then informs other processes about whether it holds the token or not.
4. **Simulation Execution**:
    - The main simulation loop runs until the specified simulation time is reached.
    - Processes transition through different states: RELEASED, REQUESTED, GRANTED, and INACTIVE.
5. **Handling RELEASED State**:
    - Processes in the RELEASED state either wait for a random time or respond to incoming requests for the critical section.
    - They update their RN (Request Number) based on received request messages and decide whether to send the token to requesting processes.
6. **Handling REQUESTED State**:
    - Processes in the REQUESTED state wait for either a request message or the token.
    - They update their RN upon receiving a request message and transition to the GRANTED state upon receiving the token.
7. **Handling GRANTED State**:
    - Processes in the GRANTED state execute the critical section for a fixed amount of time.
    - They update their LN (Last Executed) array and determine whether to add other processes to the token's queue for future requests.
8. **Handling INACTIVE State**:
    - Processes in the INACTIVE state wait for activation messages.
    - Upon receiving an activation message, they transition to the RELEASED state.
9.  **Simulation Termination**:
    - The simulation loop continues until the specified simulation time is reached.
    - After the simulation ends, processes print a message indicating the end of the simulation.

**Assuptions**:
1. Maximumm processes can be less than or equal to 10
2. Maximum waiting time can be 5
3. Maximum simulation time can be 30 
4. Probablity of adding a new process is 50%, and can only be added added if some processes is in GRANTED state.


This code effectively implements the Suzuki-Kasami algorithm, providing mutual exclusion in a distributed system while managing token circulation and process states efficiently.