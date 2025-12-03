
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
double flow_distance(flow* F1, flow* F2, double WB, double WT, double WD, double WS) {
    double powB = pow(( (double) F1->total_bytes      - (double) F2->total_bytes   ), 2);
    double powT = pow(( (double) F1->flow_duration    - (double) F2->flow_duration ), 2);
    double powD = pow(( F1->avg_interarrival_time     - F2->avg_interarrival_time  ), 2);
    double powS = pow(( F1->avg_packet_len            - F2->avg_packet_len         ), 2);
    return sqrt(powB*WB + powT*WT + powD*WD + powS*WS);
}

//find cluster distance (minimum distance of flows between clusters)
double cluster_distance(cluster* C1, cluster* C2, double WB, double WT, double WD, double WS) {
    double min_clusters_dist = -1;

    for (int i = 0; i < C1->size; i++) {
        flow* flow1 = C1->flows[i];

        for (int j = 0; j < C2->size; j++) {
            flow* flow2 = C2->flows[j];

            double dist = flow_distance(flow1, flow2, WB, WT, WD, WS);

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

//free everything the program creates (flows array, clusters array, closes file)
//CLUSTERS_SIZE is needed to properly access and free CLUSTER.flows array
void free_all(FILE* DATAFILE, flow* FLOWS, cluster* CLUSTERS, int CLUSTERS_SIZE) {
    if (DATAFILE != NULL)
        fclose(DATAFILE);

    free(FLOWS);

    if (CLUSTERS != NULL) {
        for (int i = 0; i < CLUSTERS_SIZE; i++)
            free(CLUSTERS[i].flows);
        free(CLUSTERS);
    }
}

//compare functions for usage of qsort()
int comp_ints(int A, int B){
    if (A <= B) {
        if (A == B){
            return 0;
        }
        return -1;
    }
    return 1;
}
int comp_flows(const void *a, const void *b) {
    flow *F1 = (flow *) a;
    flow *F2 = (flow *) b;
    return comp_ints(F1->flow_id, F2->flow_id);
}
int comp_clusters(const void *a, const void *b){
    cluster *C1 = (cluster *) a;
    cluster *C2 = (cluster *) b;
    return comp_ints(C1->flows[0]->flow_id, C2->flows[0]->flow_id);
}


// MAIN
int main(int argc, char* argv[]) { //syntax: ./flows SOUBOR [N WB WT WD WS]
    
    // Check if input file provided
    if (argc < 2) {
        fprintf(stderr, "No input file\n");
        return 1;
    }

    // Read args
    int N = 0;
    double WB = 1; double WT = 1; double WD = 1; double WS = 1;
    if (argc == 7) {
        N = atoi(argv[2]);
        WB = atof(argv[3]); WT = atof(argv[4]); WD = atof(argv[5]); WS = atof(argv[6]);

        if (WB < 0 || WT < 0 || WD < 0 || WS < 0) {
            fprintf(stderr, "Invalid weights (cannot be negative)\n");
            return 1;
        }
    }

    // Open file
    FILE* DATA = fopen(argv[1], "r");
    if (DATA == NULL) {
        fprintf(stderr, "Failed to open input file\n");
        return 1;
    }

    //INPUT 1 - flow count
    int flow_count = 0;
    fscanf(DATA, "count=%i", &flow_count);
    if (flow_count <= 0) {
        fprintf(stderr, "Invalid flow count (%i)\n", flow_count);
        fclose(DATA);
        return 1;
    }

    //INPUT 2 - flows and clusters
    flow* flows = malloc(flow_count * sizeof(flow));
    if (flows == NULL) {
        fprintf(stderr, "Failed to malloc %i flows\n", flow_count);
        fclose(DATA);
        return 1;
    }

    int cluster_count = 0; //starts at 0, is incremented as clusters are actually added, to 
    cluster* clusters = malloc(flow_count * sizeof(cluster));
    if (clusters == NULL) {
        fprintf(stderr, "Failed to malloc %i clusters\n", flow_count);
        free_all(DATA, flows, NULL, 0);
        return 1;
    }

    // Write flow data into flows array
    for (int i = 0; i < flow_count; i++){
        flow* new_flow = &flows[i]; // current flow
        unsigned int packet_count;
        int ip1[4];
        int ip2[4];

        //[FLOWID SRC_IP DST_IP TOTAL_BYTES FLOW_DURATION PACKET_COUNT AVG_INTERARRIVAL]
        // (read ips only to check validity)
        int scanned_count = fscanf(DATA, "%i %i.%i.%i.%i %i.%i.%i.%i %u %u %u %lf",
            &new_flow->flow_id,
            &ip1[0],&ip1[1],&ip1[2],&ip1[3],
            &ip2[0],&ip2[1],&ip2[2],&ip2[3],
            &new_flow->total_bytes,
            &new_flow->flow_duration,
            &packet_count,
            &new_flow->avg_interarrival_time
        );
        if (scanned_count != 13) {
            fprintf(stderr, "Missing data values (scanned_count:%i)\n", scanned_count);
            free_all(DATA, flows, clusters, cluster_count);
            return 1;
        }

        // Check ips
        if (!validate_ipv4(ip1) || !validate_ipv4(ip2)) {
            fprintf(stderr, "Invalid IP in flow id:%i\n", new_flow->flow_id);
            free_all(DATA, flows, clusters, cluster_count);
            return 1;
        };

        // Calculate average packet length for flow
        if (packet_count == 0) {
            fprintf(stderr, "Invalid data in flow id:%i (packet_count = 0)\n", new_flow->flow_id);
            free_all(DATA, flows, clusters, cluster_count);
            return 1;
        }
        new_flow->avg_packet_len = (double)new_flow->total_bytes / packet_count;
        //printf("flow %i: %u %u %lf %lf (packet_count = %u)\n", flow->flow_id, flow->total_bytes, flow->flow_duration, flow->avg_interarrival_time, flow->avg_packet_len, packet_count);
        //printf("%i\n", scanned_count);

        // Initialize cluster of size 1 for current flow
        clusters[i].size = 1;
        clusters[i].flows = malloc(sizeof(flow*));
        if (clusters[i].flows == NULL) {
            fprintf(stderr, "Failed to malloc flowIDs array of size 1 in cluster %i\n", i);
            free_all(DATA, flows, clusters, cluster_count);
            return 1;
        }
        clusters[i].flows[0] = &flows[i];
        cluster_count++;
    }

    // Close file
    fclose(DATA);


    //PROCESS CLUSTERS

    while (cluster_count > N && N > 0){

        // Find two closest clusters
        int nearest_cluster1 = -1; int nearest_cluster2 = -1; //indexes in clusters array
        double nearest_dist = 0;
        for (int i = 0; i < cluster_count; i++) {
            cluster* cluster1 = &clusters[i];
            
            for (int j = i+1; j < cluster_count; j++) {
                cluster* cluster2 = &clusters[j];

                // Find the minimal distance of the closest flows between cluster1 and cluster2
                double min_clusters_dist = cluster_distance(cluster1, cluster2, WB, WT, WD, WS);
                if (min_clusters_dist == -1) {
                    fprintf(stderr, "Unable to calculate cluster distance\n");
                    free_all(NULL, flows, clusters, cluster_count);
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

        void* realloc_temp = realloc(cluster1->flows, (cluster1->size + cluster2->size) * sizeof(flow *));
        if (realloc_temp == NULL) {
            fprintf(stderr, "Failed to realloc flowIDs when combining clusters %i, %i\n", nearest_cluster1, nearest_cluster2);
            free_all(NULL, flows, clusters, cluster_count);
            return 1;
        }
        cluster1->flows = realloc_temp;

        for (int i = 0; i < cluster2->size; i++) {
            cluster1->flows[cluster1->size + i] = cluster2->flows[i];
        }
        cluster1->size = cluster1->size + cluster2->size;

        // Delete cluster2
        free(cluster2->flows);

        for (int i = nearest_cluster2+1; i < cluster_count; i++) {
            clusters[i-1] = clusters[i];
        }
        cluster_count--;

        //(realloc shouldnt fail 99% of the time here, but just in case)
        realloc_temp = realloc(clusters, cluster_count * sizeof(cluster)); 
        if (realloc_temp == NULL) {
            fprintf(stderr, "Failed to realloc clusters array after combining\n");
            free_all(NULL, flows, clusters, cluster_count);
            return 1;
        }
        clusters = realloc_temp; 
    }

    //SORT RESULT
    // Sort flows in clusters
    for (int i = 0; i < cluster_count; i++) {
        qsort(clusters[i].flows, clusters[i].size, sizeof(flow), comp_flows);
    }
    // Sort clusters by first flow
    qsort(clusters, cluster_count, sizeof(cluster), comp_clusters);

    //OUTPUT
    //printf("\ndbg: FINAL OUTPUT:\n");
    printf("Clusters:\n");
    for (int i = 0; i < cluster_count; i++) {
        print_cluster(i, &clusters[i]);
    }
    
    //free allocated memory
    free_all(NULL, flows, clusters, cluster_count);

    return 0;
}
