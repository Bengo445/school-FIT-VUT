
// ZIT - PROJ 2 - Samuel Bončo


// LIBS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


// STRUCTS
typedef struct S_flow {
    int flow_id;                    //
    unsigned int total_bytes;       //whole bytes
    unsigned int flow_duration;     //whole seconds
    double avg_interarrival_time;   //non-whole seconds
    double avg_packet_len;          //non-whole bytes
} flow;

typedef struct S_cluster {
    int size;
    flow** flows;                   // array of pointers to flows (no need to search flows by ID this way)
} cluster;


// FUNCS
//print cluster info
void print_cluster(int ID, cluster* CLUSTER) {
    printf("cluster %i:", ID);
    for (int j = 0; j < CLUSTER->size; j++) {
        printf(" %i", CLUSTER->flows[j]->flow_id);
    }
    printf("\n");
}

//calculate weighted euclidean distance between 2 flows
//fuck pow()
double flow_distance(flow* F1, flow* F2, double* WEIGHTS) {
    double B = (double) F1->total_bytes      - (double) F2->total_bytes;
    double T = (double) F1->flow_duration    - (double) F2->flow_duration;
    double D = F1->avg_interarrival_time     - F2->avg_interarrival_time;
    double S = F1->avg_packet_len            - F2->avg_packet_len;
    return sqrt(B*B*WEIGHTS[0] + T*T*WEIGHTS[1] + D*D*WEIGHTS[2] + S*S*WEIGHTS[3]);
}

//find cluster distance (minimum distance of flows between clusters)
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

//validate ip
int validate_ipv4(int* ip) {
    for (int i = 0; i < 4; i++) {
        if (ip[i] < 0 || ip[i] > 255) {
            return 0;
        } 
    }
    return 1;
}

//free CLUSTERS_SIZE clusters from CLUSTERS array
//CLUSTERS_SIZE is needed to properly access and free CLUSTER.flows array
void free_all(cluster* clusters_array, int clusters_size) {
    if (clusters_array == NULL) 
        return;
    
    for (int i = 0; i < clusters_size; i++) {
        cluster* cluster = &clusters_array[i];

        for (int j = 0; j < cluster->size; j++)
            free(cluster->flows[j]);        //free invididual flow objects

        free(cluster->flows);               //free flows array in cluster
    }

    free(clusters_array);                         //free clusters array
}

//compare functions for usage of qsort()
int comp_flows(const void *a, const void *b) {
    flow *F1 = (flow *) a;
    flow *F2 = (flow *) b;
    return (F1->flow_id - F2->flow_id);
}
int comp_clusters(const void *a, const void *b) {
    cluster *C1 = (cluster *) a;
    cluster *C2 = (cluster *) b;
    return (C1->flows[0]->flow_id - C2->flows[0]->flow_id);
}

int process_clusters(cluster* clusters, int* clusters_size, int GOAL_CLUSTERS_SIZE, double* WEIGHTS) {
    
    while (*clusters_size > GOAL_CLUSTERS_SIZE && GOAL_CLUSTERS_SIZE > 0) {
        // Find two closest clusters
        int nearest_cluster1 = -1; int nearest_cluster2 = -1; //indexes in clusters array
        double nearest_dist = 0;
        for (int i = 0; i < *clusters_size; i++) {
            cluster* cluster1 = &clusters[i];
            
            for (int j = i+1; j < *clusters_size; j++) {
                cluster* cluster2 = &clusters[j];

                // Find the minimal distance of the closest flows between cluster1 and cluster2
                double min_clusters_dist = cluster_distance(cluster1, cluster2, WEIGHTS);
                if (min_clusters_dist == -1) {
                    fprintf(stderr, "Unable to calculate cluster distance\n");
                    return 1;
                }

                if (nearest_dist > min_clusters_dist || nearest_cluster1 == -1) {
                    nearest_dist = min_clusters_dist;
                    nearest_cluster1 = i;
                    nearest_cluster2 = j;
                }
            }
        }
        
        // Combine clusters (into cluster1)
        cluster* cluster1 = &clusters[nearest_cluster1];
        cluster* cluster2 = &clusters[nearest_cluster2];
        void* realloc_temp = NULL;

        realloc_temp = realloc(cluster1->flows, (cluster1->size + cluster2->size) * sizeof(flow*));
        if (realloc_temp == NULL) {
            fprintf(stderr, "Failed to realloc flows when combining clusters\n");
            return 1;
        }
        cluster1->flows = realloc_temp;

        for (int i = 0; i < cluster2->size; i++) {
            cluster1->flows[cluster1->size + i] = cluster2->flows[i];
        }
        cluster1->size = cluster1->size + cluster2->size;

        // Free cluster2 (without freeing any actual flow objects)
        free(cluster2->flows);

        for (int i = nearest_cluster2+1; i < *clusters_size; i++) {
            clusters[i-1] = clusters[i];
        }
        *clusters_size -= 1;

        // (used to realloc clusters array to 1 lower, but removed for simplicity)
    }

    return 0;
}


// MAIN
int main(int argc, char* argv[]) { //syntax: ./flows SOUBOR [N WB WT WD WS]
    
    // Read args
    int GOAL_CLUSTERS_SIZE = 0;         // N
    double WEIGHTS[4] = {1, 1, 1, 1};   // {WB, WT, WD, WS}
    if (argc == 7) {
        GOAL_CLUSTERS_SIZE = atoi(argv[2]);
        WEIGHTS[0] = atof(argv[3]);
        WEIGHTS[1] = atof(argv[4]);
        WEIGHTS[2] = atof(argv[5]);
        WEIGHTS[3] = atof(argv[6]);

        if (WEIGHTS[0] < 0 || WEIGHTS[1] < 0 || WEIGHTS[2] < 0 || WEIGHTS[3] < 0) {
            fprintf(stderr, "Invalid weights (cannot be negative)\n");
            return 1;
        }
    }

    // Open file
    if (argc < 2) {
        fprintf(stderr, "No input file\n");
        return 1;
    }
    FILE* DATA = fopen(argv[1], "r");
    if (DATA == NULL) {
        fprintf(stderr, "Failed to open input file\n");
        return 1;
    }

    //INPUT 1 - flow count
    int flow_count = 0;
    if (fscanf(DATA, "count=%i", &flow_count) != 1 || flow_count <= 0) {
        fprintf(stderr, "Invalid flow count (%i)\n", flow_count);
        fclose(DATA);
        return 1;
    }

    //INPUT 2 - flows and clusters
    int clusters_size = 0;
    cluster* clusters = malloc(flow_count * sizeof(cluster));
    if (clusters == NULL) {
        fprintf(stderr, "Failed to malloc %i clusters\n", flow_count);
        fclose(DATA);
        return 1;
    }

    // Write flow data into flows array
    for (int i = 0; i < flow_count; i++){

        // Initialize cluster of size 1 for current flow
        clusters[i].size = 1;
        clusters[i].flows = malloc(sizeof(flow*));
        if (clusters[i].flows == NULL) {
            fprintf(stderr, "Failed to malloc flow* array of size 1 in cluster %i\n", i);
            free_all(clusters, clusters_size);
            fclose(DATA);
            return 1;
        }
        clusters_size++;

        flow* new_flow = malloc(sizeof(flow));      // current flow reference for easier work
        if (new_flow == NULL) {
            fprintf(stderr, "Failed to malloc flow object i:%i\n", i);
            free_all(clusters, clusters_size);
            fclose(DATA);
            return 1;
        }
        clusters[i].flows[0] = new_flow; 
        
        // Read flow data
        //[FLOWID SRC_IP DST_IP TOTAL_BYTES FLOW_DURATION PACKET_COUNT AVG_INTERARRIVAL]
        // (read ips only to check validity)
        unsigned int packet_count;
        int ip1[4];
        int ip2[4];

        int data_count = fscanf(DATA, "%i %i.%i.%i.%i %i.%i.%i.%i %u %u %u %lf",
            &new_flow->flow_id,
            &ip1[0],&ip1[1],&ip1[2],&ip1[3],
            &ip2[0],&ip2[1],&ip2[2],&ip2[3],
            &new_flow->total_bytes,
            &new_flow->flow_duration,
            &packet_count,
            &new_flow->avg_interarrival_time
        );
        if (data_count != 13) {
            fprintf(stderr, "Missing data values (data_count:%i)\n", data_count);
            free_all(clusters, clusters_size);
            fclose(DATA);
            return 1;
        }

        // Check ips
        if (!validate_ipv4(ip1) || !validate_ipv4(ip2)) {
            fprintf(stderr, "Invalid IP in flow id:%i\n", new_flow->flow_id);
            free_all(clusters, clusters_size);
            fclose(DATA);
            return 1;
        };

        // Calculate average packet length for flow
        if (packet_count == 0) {
            fprintf(stderr, "Invalid data in flow id:%i (packet_count = 0)\n", new_flow->flow_id);
            free_all(clusters, clusters_size);
            fclose(DATA);
            return 1;
        }
        new_flow->avg_packet_len = (double) new_flow->total_bytes / packet_count;

    }

    // Close file
    fclose(DATA);


    //PROCESS CLUSTERS
    int err_process_clusters = process_clusters(clusters, &clusters_size, GOAL_CLUSTERS_SIZE, WEIGHTS);
    if (err_process_clusters) {
        free_all(clusters, clusters_size);
        return 1;
    };

    //SORT RESULT
    // Sort flows for every cluster
    for (int i = 0; i < clusters_size; i++) {
        qsort(clusters[i].flows, clusters[i].size, sizeof(flow*), comp_flows);
    }
    // Sort clusters by first flow
    qsort(clusters, clusters_size, sizeof(cluster), comp_clusters);

    //OUTPUT
    printf("Clusters:\n");
    for (int i = 0; i < clusters_size; i++) {
        print_cluster(i, &clusters[i]);
    }
    
    //FREE ALLOCATED
    free_all(clusters, clusters_size);

    return 0;
}
