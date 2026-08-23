
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdbool.h>
    #include <time.h>

    #define MAX_NODES 600

    typedef struct {
        int active_nodes_left;
        int total_failed_nodes;
        int cascade_rounds;
        double execution_time_ms;
    } CascadeResult;

    double run_cascade_simulation_c_v3(const int* adj_matrix, const double* initial_loads, 
                                      int N, double alpha, const int* initial_failed_nodes, 
                                      int num_initial_fails, CascadeResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        double capacity[MAX_NODES];
        double current_load[MAX_NODES];
        bool alive[MAX_NODES];

        for (int i = 0; i < N; i++) {
            current_load[i] = initial_loads[i];
            capacity[i] = (1.0 + alpha) * initial_loads[i];
            alive[i] = true;
        }

        int total_failed = 0;
        for (int f = 0; f < num_initial_fails; f++) {
            int node_id = initial_failed_nodes[f];
            if (node_id >= 0 && node_id < N && alive[node_id]) {
                alive[node_id] = false;
                total_failed++;
            }
        }

        int rounds = 0;
        bool new_failure = true;

        while (new_failure && total_failed < N) {
            new_failure = false;
            rounds++;
            double load_to_redistribute[MAX_NODES] = {0.0};

            for (int i = 0; i < N; i++) {
                if (!alive[i] && current_load[i] > 0.0) {
                    load_to_redistribute[i] = current_load[i];
                    current_load[i] = 0.0;
                }
            }

            for (int src = 0; src < N; src++) {
                if (load_to_redistribute[src] > 0.0) {
                    int active_neighbors = 0;
                    for (int dst = 0; src != dst && dst < N; dst++) {
                        if (adj_matrix[src * N + dst] == 1 && alive[dst]) active_neighbors++;
                    }

                    if (active_neighbors > 0) {
                        double share = load_to_redistribute[src] / (double)active_neighbors;
                        for (int dst = 0; dst < N; dst++) {
                            if (adj_matrix[src * N + dst] == 1 && alive[dst]) {
                                current_load[dst] += share;
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < N; i++) {
                if (alive[i] && current_load[i] > capacity[i]) {
                    alive[i] = false;
                    total_failed++;
                    new_failure = true;
                }
            }
        }

        out_res->active_nodes_left = N - total_failed;
        out_res->total_failed_nodes = total_failed;
        out_res->cascade_rounds = rounds;

        clock_gettime(CLOCK_MONOTONIC, &end);
        return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    }
    