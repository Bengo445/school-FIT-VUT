//
// ZIT - PROJ 2 - Samuel Bončo
//

// ==== LIBS ====
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


// ==== STRUCTS ====
typedef struct S_flow {
    unsigned int flow_id;                    //
    unsigned int total_bytes;       //whole bytes
    unsigned int flow_duration;     //whole seconds
    double avg_interarrival_time;   //non-whole seconds
    double avg_packet_len;          //non-whole bytes
} flow;

typedef struct S_cluster {
    int size;
    flow** flows;                   // array of pointers to flows (no need to search flows by ID this way)
} cluster;


// ==== FUNCS ====
// Validates ipv4 - checks each octet for invalid values.
int validate_ipv4(int* ip) {
    for (int i = 0; i < 4; i++) {
        if (ip[i] < 0 || ip[i] > 255) {
            return 0;
        } 
    }
    return 1;
}

// Free clusters_size clusters from CLUSTERS array.
// clusters_size is needed to properly access and free CLUSTER.flows array.
void free_all(cluster* clusters_array, int clusters_size) {
    if (clusters_array == NULL) 
        return;
    
    for (int i = 0; i < clusters_size; i++) {
        cluster* cluster = &clusters_array[i];

        for (int j = 0; j < cluster->size; j++)
            free(cluster->flows[j]);        //free invididual flow objects

        free(cluster->flows);               //free flows array in cluster
    }

    free(clusters_array);                   //free clusters array
}

// Calculate weighted euclidean distance between 2 flows.
// pow() sucks
double flow_distance(flow* F1, flow* F2, double* WEIGHTS) {
    double B = (double) F1->total_bytes      - (double) F2->total_bytes;
    double T = (double) F1->flow_duration    - (double) F2->flow_duration;
    double D = F1->avg_interarrival_time     - F2->avg_interarrival_time;
    double S = F1->avg_packet_len            - F2->avg_packet_len;
    return sqrt(B*B*WEIGHTS[0] + T*T*WEIGHTS[1] + D*D*WEIGHTS[2] + S*S*WEIGHTS[3]);
}

// Finds the "distance" between two clusters (minimum distance of any two flows from each cluster).
double cluster_distance(cluster* clusterA, cluster* clusterB, double* WEIGHTS) {
    double min_clusters_dist = -1;

    for (int i = 0; i < clusterA->size; i++) {
        flow* flow1 = clusterA->flows[i];

        for (int j = 0; j < clusterB->size; j++) {
            flow* flow2 = clusterB->flows[j];

            double dist = flow_distance(flow1, flow2, WEIGHTS);

            if (min_clusters_dist > dist || min_clusters_dist == -1)
                min_clusters_dist = dist;
        }
    }
    return min_clusters_dist;
}

// Finds 2 closest clusters in clusters array.
// Writes resulting cluster indexes into (int found_clusters[2]) array. Doesn't change array if distance is invalid.
void find_closest_clusters(cluster* clusters, int CLUSTER_SIZE, double* weights, int found_clusters[2]) {
    double nearest_dist = 0;

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        cluster* cluster1 = &clusters[i];
        
        for (int j = i+1; j < CLUSTER_SIZE; j++) {
            cluster* cluster2 = &clusters[j];

            // Find the minimal distance of the closest flows between cluster1 and cluster2
            double min_clusters_dist = cluster_distance(cluster1, cluster2, weights);
            if (min_clusters_dist == -1)
                continue;

            if (nearest_dist > min_clusters_dist || found_clusters[0] == -1) {
                nearest_dist = min_clusters_dist;
                found_clusters[0] = i;
                found_clusters[1] = j;
            }
        }
    }
}

// Combines 2 clusters (into clusterA) by copying flows from clusterB to clusterA.
int combine_clusters(cluster* clusterA, cluster* clusterB) {
    //alloc bigger clusterA 
    void* realloc_temp = realloc(clusterA->flows, (clusterA->size + clusterB->size) * sizeof(flow*));
    if (realloc_temp == NULL) {
        fprintf(stderr, "Failed to realloc flows when combining clusters\n");
        return 1;
    }
    clusterA->flows = realloc_temp;

    //copy flows
    for (int i = 0; i < clusterB->size; i++) {
        clusterA->flows[clusterA->size + i] = clusterB->flows[i];
    }
    clusterA->size = clusterA->size + clusterB->size;

    return 0;
}

// Main cluster processing - reduce size of clusters array to goal_clusters_size by combining clusters.
// Doesn't do anything if goal_clusters_size is 0 or less.
int process_clusters(cluster* clusters, int* clusters_size, int goal_clusters_size, double* weights) {

    if (goal_clusters_size <= 0) {
        return 0;
    }

    while (*clusters_size > goal_clusters_size) {

        // Find two closest clusters
        int found_clusters[2] = {-1, -1};   //indexes in clusters array
        find_closest_clusters(clusters, *clusters_size, weights, found_clusters);

        if (found_clusters[0] == -1 || found_clusters[1] == -1) {
            fprintf(stderr, "Failed to find nearest clusters\n");
            return 1;
        }

        // Combine clusters (into clusterA)
        cluster* clusterA = &clusters[found_clusters[0]];
        cluster* clusterB = &clusters[found_clusters[1]];
        
        int err_combine_clusters = combine_clusters(clusterA, clusterB);
        if (err_combine_clusters)
            return 1;

        // Free cluster2 (without freeing the flow objects stored in it)
        free(clusterB->flows);

        for (int i = found_clusters[1] + 1; i < *clusters_size; i++) {
            clusters[i-1] = clusters[i];
        }
        *clusters_size -= 1;

        // (used to realloc clusters array to 1 lower, but removed for simplicity)
    }

    return 0;
}

// Main cluster input - alloc all flows and save each flow into its' own cluster by pointer.
int input_flows(int flow_count, cluster* clusters, int* clusters_size, FILE* DATA) {

    for (int i = 0; i < flow_count; i++){
        // Initialize cluster of size 1 for current flow
        clusters[i].size = 1;
        clusters[i].flows = malloc(sizeof(flow*));
        if (clusters[i].flows == NULL) {
            fprintf(stderr, "Failed to malloc flow* array of size 1 in cluster %i\n", i);
            return 1;
        }
        *clusters_size += 1;

        flow* new_flow = malloc(sizeof(flow));      // current flow reference for easier work
        if (new_flow == NULL) {
            fprintf(stderr, "Failed to malloc flow object i:%i\n", i);
            return 1;
        }
        clusters[i].flows[0] = new_flow; 
        
        // Read flow data
        // (read ips only to check validity)
        int packet_count;
        int ip1[4];
        int ip2[4];

        //[FLOWID SRC_IP DST_IP TOTAL_BYTES FLOW_DURATION PACKET_COUNT AVG_INTERARRIVAL]
        int data_count = fscanf(DATA, "%u %i.%i.%i.%i %i.%i.%i.%i %u %u %i %lf",
            &new_flow->flow_id,
            &ip1[0], &ip1[1], &ip1[2], &ip1[3],
            &ip2[0], &ip2[1], &ip2[2], &ip2[3],
            &new_flow->total_bytes,
            &new_flow->flow_duration,
            &packet_count,
            &new_flow->avg_interarrival_time
        );
        if (data_count != 13) {
            fprintf(stderr, "Missing data values (data_count:%i)\n", data_count);
            return 1;
        }

        // Validate ips
        if (!validate_ipv4(ip1) || !validate_ipv4(ip2)) {
            fprintf(stderr, "Invalid IP in flow id:%i\n", new_flow->flow_id);
            return 1;
        };

        // Calculate average packet length for flow
        if (packet_count <= 0) {
            fprintf(stderr, "Invalid data in flow id:%i (packet_count <= 0)\n", new_flow->flow_id);
            return 1;
        }
        new_flow->avg_packet_len = (double) new_flow->total_bytes / packet_count;

    }

    return 0;
}

// Main input - handles entire input file.
// Allocates main clusters array based on flow count, then writes flow data into clusters.
int input_all(char* input_filename, int* clusters_size, cluster** clusters) {
    //open file
    FILE* DATA = fopen(input_filename, "r");
    if (DATA == NULL) {
        fprintf(stderr, "Failed to open input file\n");
        return 1;
    }

    //input flow count
    int flow_count = 0;
    if (fscanf(DATA, "count=%i", &flow_count) != 1 || flow_count <= 0) {
        fprintf(stderr, "Invalid flow count (%i)\n", flow_count);
        fclose(DATA);
        return 1;
    }

    //alloc main clusters array based on flow count
    *clusters = malloc(flow_count * sizeof(cluster));
    if (*clusters == NULL) {
        fprintf(stderr, "Failed to malloc %i clusters\n", flow_count);
        fclose(DATA);
        return 1;
    }

    //input flows into clusters in clusters array
    int err_input_flows = input_flows(flow_count, *clusters, clusters_size, DATA);
    if (err_input_flows) {
        fclose(DATA);
        return 1;
    }

    //close file
    fclose(DATA);
    return 0;
}

// Compare flows function for qsort().
int comp_flows(const void *a, const void *b) {
    flow* F1 = *(flow**) a;
    flow* F2 = *(flow**) b;
    return (F1->flow_id - F2->flow_id);
}
// Compare clusters function for qsort().
int comp_clusters(const void *a, const void *b) {
    cluster* C1 = (cluster*) a;
    cluster* C2 = (cluster*) b;
    return (C1->flows[0]->flow_id - C2->flows[0]->flow_id);
}

// Main output - prints each cluster and its flows.
void print_clusters(cluster* clusters, int clusters_size) {

    printf("Clusters:\n");
    for (int i = 0; i < clusters_size; i++) {
        cluster cluster = clusters[i];

        printf("cluster %i:", i);
        for (int j = 0; j < cluster.size; j++) {
            printf(" %i", cluster.flows[j]->flow_id);
        }
        printf("\n");
    }
}


// ==== MAIN ====
int main(int argc, char* argv[]) { //syntax: ./flows FILE [N WB WT WD WS]
    
    // READ ARGS
    if (argc < 2) {
        fprintf(stderr, "No input file\n");
        return 1;
    }
    char* input_filename = argv[1];

    int goal_clusters_size = 0;         // N
    double weights[4] = {1, 1, 1, 1};   // {WB, WT, WD, WS}
    if (argc == 7) {
        goal_clusters_size = atoi(argv[2]);
        weights[0] = atof(argv[3]); //WB
        weights[1] = atof(argv[4]); //WT
        weights[2] = atof(argv[5]); //WD
        weights[3] = atof(argv[6]); //WS

        if (weights[0] < 0 || weights[1] < 0 || weights[2] < 0 || weights[3] < 0) {
            fprintf(stderr, "Invalid weights (cannot be negative)\n");
            return 1;
        }
    }

    // INPUT
    int clusters_size = 0;
    cluster* clusters = NULL;

    int err_input_all = input_all(input_filename, &clusters_size, &clusters);
    if (err_input_all) {
        free_all(clusters, clusters_size);
        return 1;
    }

    // PROCESS CLUSTERS
    int err_process_clusters = process_clusters(clusters, &clusters_size, goal_clusters_size, weights);
    if (err_process_clusters) {
        free_all(clusters, clusters_size);
        return 1;
    };

    // SORT RESULT
    //sort flows for every cluster
    for (int i = 0; i < clusters_size; i++) {
        qsort(clusters[i].flows, clusters[i].size, sizeof(flow*), comp_flows);
    }
    //sort clusters by first flow
    qsort(clusters, clusters_size, sizeof(cluster), comp_clusters);

    // OUTPUT
    print_clusters(clusters, clusters_size);
    
    // FREE ALLOCATED
    free_all(clusters, clusters_size);

    return 0;
}
