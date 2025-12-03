#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// STRUCTS
typedef struct S_flow {
    int flow_id;
    unsigned int total_bytes;    //whole bytes
    unsigned int flow_duration;  //whole seconds
    double avg_interarrival_time; //seconds
    double avg_packet_len;        //bytes
} flow;

typedef struct S_cluster {
    int size;
    int* flowIDs;
} cluster;


// FUNCS

//get flow index by ID
int get_flow_index(flow* FLOWS, int FLOWS_SIZE, int ID) {
    for (int i = 0; i < FLOWS_SIZE; i++) {
        if (FLOWS[i].flow_id == ID) {
            return i;
        }
    }
    return -1;
}

//print cluster info
void print_cluster(int ID, cluster* CLUSTER) {
    printf("cluster %i:", ID);
    for (int j = 0; j < CLUSTER->size; j++) {
        printf(" %i", CLUSTER->flowIDs[j]);
    }
    printf("\n");
}

//find cluster distance (minimum distance of flows between clusters)
double cluster_distance(flow* FLOWS, int FLOWS_SIZE, cluster* C1, cluster* C2, double WB, double WT, double WD, double WS) {
    double min_clusters_dist = -1;

    for (int k = 0; k < C1->size; k++) {
        int flow1_index = get_flow_index(FLOWS, FLOWS_SIZE, C1->flowIDs[k]);
        if (flow1_index == -1) {continue;}

        flow* flow1 = &FLOWS[flow1_index];

        for (int l = 0; l < C2->size; l++) {
            int flow2_index = get_flow_index(FLOWS, FLOWS_SIZE, C2->flowIDs[l]);
            if (flow2_index == -1) {continue;}

            flow* flow2 = &FLOWS[flow2_index];

            // Calculate weighted euclidean distance
            double powB = pow(((double)flow1->total_bytes - (double)flow2->total_bytes), 2);
            double powT = pow(((double)flow1->flow_duration - (double)flow2->flow_duration), 2);
            double powD = pow((flow1->avg_interarrival_time - flow2->avg_interarrival_time), 2);
            double powS = pow((flow1->avg_packet_len - flow2->avg_packet_len), 2);
            double dist = sqrt(powB*WB + powT*WT + powD*WD + powS*WS);

            if (min_clusters_dist > dist || min_clusters_dist == -1) {
                min_clusters_dist = dist;
            }
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

void free_clusters(cluster* CLUSTERS, int CLUSTERS_SIZE) {
    for (int i = 0; i < CLUSTERS_SIZE; i++) {
        free(CLUSTERS[i].flowIDs);
    }
    free(CLUSTERS);
}


// MAIN
int main(int argc, char* argv[]) { //syntax: ./flows SOUBOR [N WB WT WD WS]
    
    // Check if input file provided
    if (argc < 2) {
        printf("No input file\n");
        return 1;
    }

    // Read args
    int N = 0;
    double WB = 1; double WT = 1; double WD = 1; double WS = 1;
    if (argc == 7) {
        N = atoi(argv[2]);
        WB = atof(argv[3]); WT = atof(argv[4]); WD = atof(argv[5]); WS = atof(argv[6]);

        if (WB < 0 || WT < 0 || WD < 0 || WS < 0) {
            printf("Invalid weights (cannot be negative)\n");
            return 1;
        }
    }

    // Open file
    FILE* DATA = fopen(argv[1], "r");
    if (DATA == NULL) {
        printf("Failed to open input file\n");
        return 1;
    }

    //INPUT 1 - flow count
    int flow_count = 0;
    fscanf(DATA, "count=%i", &flow_count);
    if (flow_count <= 0) {
        printf("Invalid flow count (%i)\n", flow_count);
        fclose(DATA);
        return 1;
    }

    //INPUT 2 - flows and clusters
    flow* flows = malloc(flow_count * sizeof(flow));
    if (flows == NULL) {
        printf("Failed to malloc %i flows\n", flow_count);
        fclose(DATA);
        return 1;
    }

    int cluster_count = 0; //starts at 0, is incremented as clusters are actually added, to 
    cluster* clusters = malloc(flow_count * sizeof(cluster));
    if (clusters == NULL) {
        printf("Failed to malloc %i clusters\n", flow_count);
        free(flows);
        fclose(DATA);
        return 1;
    }

    // Write flow data into flows array
    for (int i = 0; i < flow_count; i++){
        flow* flow = &flows[i]; // current flow
        unsigned int packet_count;
        int ip1[4];
        int ip2[4];

        //[FLOWID SRC_IP DST_IP TOTAL_BYTES FLOW_DURATION PACKET_COUNT AVG_INTERARRIVAL]
        // (read ips only to check validity)
        int scanned_count = fscanf(DATA, "%i %i.%i.%i.%i %i.%i.%i.%i %u %u %u %lf",
            &flow->flow_id,
            &ip1[0],&ip1[1],&ip1[2],&ip1[3],
            &ip2[0],&ip2[1],&ip2[2],&ip2[3],
            &flow->total_bytes,
            &flow->flow_duration,
            &packet_count,
            &flow->avg_interarrival_time
        );
        if (scanned_count != 13) {
            printf("Missing data values (scanned_count:%i)\n", scanned_count);
            free(flows); free_clusters(clusters, cluster_count);
            fclose(DATA);
            return 1;
        }

        // Check ips
        if (!validate_ipv4(ip1) || !validate_ipv4(ip2)) {
            printf("Invalid IP in flow id:%i\n", flow->flow_id);
            free(flows); free_clusters(clusters, cluster_count);
            fclose(DATA);
            return 1;
        };

        // Calculate average packet length for flow
        if (packet_count == 0) {
            printf("Invalid data in flow id:%i (packet_count = 0)\n", flow->flow_id);
            free(flows); free_clusters(clusters, cluster_count);
            fclose(DATA);
            return 1;
        }
        flow->avg_packet_len = (double)flow->total_bytes / packet_count;
        //printf("flow %i: %u %u %lf %lf (packet_count = %u)\n", flow->flow_id, flow->total_bytes, flow->flow_duration, flow->avg_interarrival_time, flow->avg_packet_len, packet_count);
        //printf("%i\n", scanned_count);

        // Initialize cluster of size 1 for current flow
        clusters[i].size = 1;
        clusters[i].flowIDs = malloc(sizeof(int));
        if (clusters[i].flowIDs == NULL) {
            printf("Failed to malloc flowIDs array of size 1 in cluster %i\n", i);
            free(flows); free_clusters(clusters, cluster_count);
            fclose(DATA);
            return 1;
        }
        clusters[i].flowIDs[0] = flows[i].flow_id;
        cluster_count++;
    }

    // Close file
    fclose(DATA);


    //PROCESS CLUSTERS
    //printf("----- Processing clusters\n");

    while (cluster_count > N && N > 0){
        //printf("--- Loop %i\n", cluster_count-N);

        // Find two closest clusters
        //printf("-- Finding nearest clusters\n");
        int nearest_cluster1 = -1; int nearest_cluster2 = -1; //indexes in clusters array
        double nearest_dist = 0;
        for (int i = 0; i < cluster_count; i++) {
            cluster* cluster1 = &clusters[i];
            
            for (int j = i+1; j < cluster_count; j++) {
                cluster* cluster2 = &clusters[j];

                // Find the minimal distance of the closest flows between cluster1 and cluster2
                double min_clusters_dist = cluster_distance(flows, flow_count, cluster1, cluster2, WB, WT, WD, WS);
                if (min_clusters_dist == -1) {
                    printf("Unable to calculate cluster distance\n");
                    free(flows); free_clusters(clusters, cluster_count);
                    return 1;
                }

                //printf("- Comparing clusters: \n");
                //print_cluster(i, cluster1);
                //print_cluster(j, cluster2);
                //printf("Distance: %lf\n", min_clusters_dist);

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

        //printf("-- Final nearest clusters:\n");
        //print_cluster(nearest_cluster1, cluster1);
        //print_cluster(nearest_cluster2, cluster2);

        void* realloc_temp = realloc(cluster1->flowIDs, (cluster1->size + cluster2->size) * sizeof(int));
        if (realloc_temp == NULL) {
            printf("Failed to realloc flowIDs when combining clusters %i, %i\n", nearest_cluster1, nearest_cluster2);
            free(flows); free_clusters(clusters, cluster_count);
            return 1;
        }
        cluster1->flowIDs = realloc_temp;

        for (int i = 0; i < cluster2->size; i++) {
            cluster1->flowIDs[cluster1->size + i] = cluster2->flowIDs[i];
        }
        cluster1->size = cluster1->size + cluster2->size;

        //printf("-- Combined cluster:\n");
        //print_cluster(nearest_cluster1, cluster1);

        // Delete cluster2
        free(cluster2->flowIDs);

        for (int i = nearest_cluster2+1; i < cluster_count; i++) {
            clusters[i-1] = clusters[i];
        }
        cluster_count--;

        //(shouldnt fail 99% of the time, but just in case)
        realloc_temp = realloc(clusters, cluster_count * sizeof(cluster)); 
        if (realloc_temp == NULL) {
            printf("Failed to realloc clusters array after combining\n");
            free(flows); free_clusters(clusters, cluster_count);
            return 1;
        }
        clusters = realloc_temp; 
    }

    //OUTPUT
    //printf("\ndbg: FINAL OUTPUT:\n");
    printf("Clusters:\n");
    for (int i = 0; i < cluster_count; i++) {
        print_cluster(i, &clusters[i]);
    }
    
    //free allocated memory
    free(flows);
    free_clusters(clusters, cluster_count);

    return 0;
}
