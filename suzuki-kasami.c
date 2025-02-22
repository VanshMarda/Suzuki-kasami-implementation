#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h> 
#include <stdbool.h>
#include<string.h>

#define MAX_PROCESS_NUM 10
#define MAX_QSIZE 100
#define MAX(a,b) (((a)>(b))?(a):(b))

struct Token{
    int LN[MAX_PROCESS_NUM];
    int Q[MAX_QSIZE];
    int count;
    int active_process;
};

enum Process_State { INACTIVE, RELEASED, REQUESTED, GRANTED };

int main(int argc, char *argv[])
{
    int rank, rank_size, initial_token;
    int RN[MAX_PROCESS_NUM], SN=0;
    struct Token token;
    int sn_tag = 0, token_tag = 1;
    enum Process_State proc_state;
    bool hold_token;
    MPI_Datatype token_type;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &rank_size);

    MPI_Datatype types[4] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    int blocklengths[4] = {MAX_PROCESS_NUM, MAX_QSIZE, 1, 1};

    MPI_Aint displacements[4], address, start_address;
    MPI_Get_address(&token, &start_address); 
    displacements[0] = (MPI_Aint) 0;
    MPI_Get_address(&token.Q, &address); 
    displacements[1] = address - start_address;
    MPI_Get_address(&token.count, &address);
    displacements[2] = address - start_address;
    MPI_Get_address(&token.active_process, &address);
    displacements[3] = address - start_address;
    MPI_Type_create_struct(4, blocklengths, displacements, types, &token_type);
    MPI_Type_commit(&token_type);

    if(rank_size > MAX_PROCESS_NUM)
    {
        printf("Number of processes should be 10! \n");
        return MPI_Abort(MPI_COMM_WORLD,1);
    }

    // Initialize
    for(int i = 0; i < MAX_PROCESS_NUM; i++)
    {
        token.LN[i] = 0;
        RN[i] = 0;
    }

    for(int i = 0; i < MAX_QSIZE; i++){
        token.Q[i] = -1;
    }

    token.count = 0;
    // token.active_process = 4;
    int initial_active_process;
    sscanf(argv[3], "%d", &initial_active_process);
    token.active_process = initial_active_process;

    // Set the initial state for each process
    if (rank < atoi(argv[3])) {
        proc_state = RELEASED;
    } else {
        proc_state = INACTIVE;
    }

    // Randomly choose a process to hold the initial token
    srand(time(0));
    if(rank == 0)
    {
        initial_token = rand() % token.active_process;
        hold_token = (initial_token == 0) ? true : false;
        bool temp_token = false;
        for(int i = 1; i < rank_size; i++)
        {   
            temp_token = (initial_token == i) ? true : false;
            MPI_Send(&temp_token, 1, MPI_INT, i, token_tag, MPI_COMM_WORLD); 
        }
    }
    else
    {
        MPI_Recv(&hold_token, 1, MPI_INT, 0, token_tag, MPI_COMM_WORLD, &status);
    }
    if(hold_token)
    {
        printf("Rank %d: has the token initially \n", rank);
    } 
    MPI_Barrier(MPI_COMM_WORLD);     

    /**
     * 
     * Simulation Part:
     * 
     * */
    float simulation_time, random_wait_time, critical_end_time;
    int is_message, max_wait_time, use_critical_time = 1;
    sscanf(argv[1], "%f", &simulation_time);
    sscanf(argv[2], "%d", &max_wait_time);
    if(simulation_time > 30)
    {
        printf("The simulation time should be less than 30 seconds! \n");
        return MPI_Abort(MPI_COMM_WORLD,1);
    }
    if(max_wait_time > 5)
    {
        printf("The maximum waiting time should be less than 5 seconds! \n");
        return MPI_Abort(MPI_COMM_WORLD,1);
    }

    simulation_time =  MPI_Wtime() + simulation_time; 
    while(true)
    {    
        /**
         * 
         * Process in RELEASED state
         * 
         * */
        if(proc_state == RELEASED)
        {
            if(MPI_Wtime() >= simulation_time)
                break;
            srand(time(0));    
            random_wait_time = MPI_Wtime() + (rand() % (max_wait_time + 1));    
            while(true)
            {
                MPI_Iprobe( MPI_ANY_SOURCE, sn_tag, MPI_COMM_WORLD, &is_message, &status);
                if(is_message && status.MPI_SOURCE < rank_size && status.MPI_SOURCE >= 0)
                {
                    MPI_Recv(&SN, 1, MPI_INT, MPI_ANY_SOURCE, sn_tag, MPI_COMM_WORLD, &status); 
                    RN[status.MPI_SOURCE] = MAX(RN[status.MPI_SOURCE], SN);    
                    printf("Rank %d received critical section request from rank %d\n", rank, status.MPI_SOURCE);
                    if(hold_token && RN[status.MPI_SOURCE] == token.LN[status.MPI_SOURCE] + 1)
                    {
                        printf("Rank %d is sending the token to rank %d\n", rank, status.MPI_SOURCE);
                        MPI_Send(&token, 1, token_type, status.MPI_SOURCE, token_tag, MPI_COMM_WORLD);
                        hold_token = false;
                    }
                }
                if(MPI_Wtime() > simulation_time || MPI_Wtime() > random_wait_time)
                {
                    break;
                }
            }  
            if(MPI_Wtime() <= simulation_time)
            {
                if(hold_token)
                {
                    proc_state = GRANTED;
                }
                else
                {
                    RN[rank] = RN[rank] + 1;
                    for(int i = 0; i < rank_size; i++)
                    {
                        if(i != rank)
                        {
                            MPI_Send(&RN[rank], 1, MPI_INT, i, sn_tag, MPI_COMM_WORLD);
                        }
                    }
                    proc_state = REQUESTED;
                    printf("Process with rank %d and sequence number %d is requesting critical section\n",rank, RN[rank]);
                    printf("Broadcast message (%d:%d)\n", rank, RN[rank]);
                } 
            }
            else
            {
                break;
            }
        }

        /**
         * 
         * Process in REQUESTED state
         * 
         * */
        if(proc_state == REQUESTED)
        {
            if(MPI_Wtime() >= simulation_time)
            {
                break;
            }
            MPI_Iprobe( MPI_ANY_SOURCE, sn_tag, MPI_COMM_WORLD, &is_message, &status);
            if(is_message && status.MPI_SOURCE < rank_size && status.MPI_SOURCE >= 0)
            {
                MPI_Recv(&SN, 1, MPI_INT, MPI_ANY_SOURCE, sn_tag, MPI_COMM_WORLD, &status);
                RN[status.MPI_SOURCE] = MAX(RN[status.MPI_SOURCE], SN);  
                printf("Rank %d received critical section request from rank %d\n", rank, status.MPI_SOURCE);
            }
            MPI_Iprobe( MPI_ANY_SOURCE, token_tag, MPI_COMM_WORLD, &is_message, &status);
            if(is_message && status.MPI_SOURCE < rank_size && status.MPI_SOURCE >= 0)
            {
                MPI_Recv(&token, 1, token_type, MPI_ANY_SOURCE, token_tag, MPI_COMM_WORLD, &status);
                proc_state = GRANTED;
                hold_token = true; 
                printf("Rank %d has received the token from Rank %d and entering into critical section\n", rank, status.MPI_SOURCE);

                // Decide whether to activate an inactive process
                if (token.active_process < rank_size && rand() % 2 == 0) {
                    int inactive_rank = token.active_process;
                    token.active_process++;
                    // Send message to the inactive process to activate itself
                    MPI_Send(&proc_state, 1, MPI_INT, inactive_rank, token_tag, MPI_COMM_WORLD);
                }
            }
        
        }

        /**
         * 
         * Process in GRANTED state
         * 
         * */
        if(proc_state == GRANTED)
        {
            if(MPI_Wtime() >= simulation_time)
            {
                break;
            }
            critical_end_time = MPI_Wtime() + use_critical_time; 
            while(MPI_Wtime() < critical_end_time)
            {
                    MPI_Iprobe( MPI_ANY_SOURCE, sn_tag, MPI_COMM_WORLD, &is_message, &status);
                    if(is_message && status.MPI_SOURCE < rank_size && status.MPI_SOURCE >= 0)
                    {
                        MPI_Recv(&SN, 1, MPI_INT, MPI_ANY_SOURCE, sn_tag, MPI_COMM_WORLD, &status);
                        RN[status.MPI_SOURCE] = MAX(RN[status.MPI_SOURCE],SN);   
                        printf("Rank %d received critical section request from rank %d\n", rank, status.MPI_SOURCE);
                    }
            
            }
            token.LN[rank] = RN[rank];
            for(int i = 0; i < rank_size; i++)
            {
                if(i != rank && RN[i] == token.LN[i] + 1)
                {
                    token.Q[token.count] = i;
                    token.count++;
                }
            }
            printf("Rank %d has exited critical section\n", rank);
            if(MPI_Wtime() >= simulation_time)
            {
                proc_state = RELEASED;
                break;
            }
            if(token.count > 0 && token.Q[0] != -1)
            {
                int next_rank = token.Q[0];
                for(int i = 1; i < token.count + 1; i++)
                    token.Q[i - 1] = token.Q[i];
                token.count--;
                if(token.Q[0] >= 0 && token.Q[0] < rank_size)
                {
                    printf("Rank %d is sending the token to rank %d\n", rank, next_rank );
                    MPI_Send(&token, 1, token_type, next_rank, token_tag, MPI_COMM_WORLD);
                    hold_token = false;
                }
            }
            proc_state = RELEASED;
        }

        /**
         * 
         * Process in INACTIVE state
         * 
         * */
        if(proc_state == INACTIVE)
        {
            if(MPI_Wtime() >= simulation_time)
            {
                break;
            }
            // Check if there's a message to activate the process
            MPI_Iprobe( MPI_ANY_SOURCE, token_tag, MPI_COMM_WORLD, &is_message, &status);
            if(is_message && status.MPI_SOURCE < rank_size && status.MPI_SOURCE >= 0)
            {
                // Receive the activation message
                printf("One process added of %d rank\n", rank);
                MPI_Recv(&proc_state, 1, MPI_INT, MPI_ANY_SOURCE, token_tag, MPI_COMM_WORLD, &status);
                // Change state to RELEASED
                proc_state = RELEASED;
            }
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    if (proc_state !=INACTIVE)
    printf("Rank:%d Simulation Finished!\n", rank);
    MPI_Finalize();
    return 0;
}