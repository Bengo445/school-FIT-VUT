#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// STRUCTS
typedef struct S_flow {
    int flow_id;
    unsigned int total_bytes;    //whole bytes
    unsigned int flow_duration;  //whole seconds
    float avg_interarrival_time; //seconds
    float avg_packet_len;        //bytes
} flow;

typedef struct S_cluster {
    int size;
    int* flowIDs;
} cluster;


// FUNCS



// MAIN
int main(int argc, char* argv[]) { //syntax: ./flows SOUBOR [N WB WT WD WS]
    
    // Check if input file provided
    if (argc < 2) {
        printf("No input file\n");
        return 1;
    }

    // Read args
    int N = 0;
    float WB = 1; float WT = 1; float WD = 1; float WS = 1;
    if (argc == 7) {
        N = atoi(argv[2]);
        WB = atof(argv[3]); WT = atof(argv[4]); WD = atof(argv[5]); WS = atof(argv[6]);
    }

    // Open file
    FILE* DATA = fopen(argv[1], "r");
    if (DATA == NULL) {
        printf("Failed to open input file\n");
        return 1;
    }

    //INPUT 1 - flow count
    int flow_count;
    fscanf(DATA, "count=%i", &flow_count);


    //INPUT 2 - flows and clusters
    flow* flows = malloc(flow_count * sizeof(flow));
    if (flows == NULL) {
        printf("Failed to malloc %i flows\n", flow_count);
        return 1;
    }

    int cluster_count = flow_count;
    cluster* clusters = malloc(cluster_count * sizeof(cluster));
    if (clusters == NULL) {
        printf("Failed to malloc %i clusters\n", cluster_count);
        return 1;
    }

    // Write flow data into flows array
    for (int i = 0; i < flow_count; i++){
        unsigned int packet_count;
        fscanf(DATA, "%i %*i.%*i.%*i.%*i %*i.%*i.%*i.%*i %u %u %u %f",
            &flows[i].flow_id,
            &flows[i].total_bytes,
            &flows[i].flow_duration,
            &packet_count,
            &flows[i].avg_interarrival_time
        );
        flows[i].avg_packet_len = (float)flows[i].total_bytes / packet_count;
        //printf("flow(flow_id:%i, total_bytes:%i, flow_duration:%i, avg_interarrival_time:%f, avg_packet_len:%f)\n",
        //    flows[i].flow_id, flows[i].total_bytes, flows[i].flow_duration, flows[i].avg_interarrival_time, flows[i].avg_packet_len);

        // Initialize cluster of size 1 for current flow
        clusters[i].size = 1;
        clusters[i].flowIDs = malloc(sizeof(int));
        if (clusters[i].flowIDs == NULL) {
            printf("Failed to malloc flowIDs array in cluster %i\n", i);
            return 1;
        }
        clusters[i].flowIDs[0] = flows[i].flow_id;
    }

    // Close file
    fclose(DATA);


    //PROCESS CLUSTERS
    while (cluster_count > N){

        // Find two closest clusters
        int nearest_cluster1 = -1; int nearest_cluster2 = -1; //indexes in clusters array
        float nearest_dist;
        for (int i = 0; i < cluster_count; i++) {
            cluster* cluster1 = &clusters[i];
            
            for (int j = i+1; j < cluster_count; j++) {
                cluster* cluster2 = &clusters[j];

                // Find the minimal distance of the closest flows between cluster1 and cluster2
                float min_clusters_dist = -1;
                for (int k = 0; k < cluster1->size; k++) {
                    flow* flow1 = &flows[cluster1->flowIDs[k]];

                    for (int l = 0; l < cluster2->size; l++) {
                        flow* flow2 = &flows[cluster2->flowIDs[l]];

                        // Calculate weighted euclidean distance
                        float dist = sqrt(
                            pow((flow1->total_bytes - flow2->total_bytes), 2) * WB +
                            pow((flow1->flow_duration - flow2->flow_duration), 2) * WT +
                            pow((flow1->avg_interarrival_time - flow2->avg_interarrival_time), 2) * WD +
                            pow((flow1->avg_packet_len - flow2->avg_packet_len), 2) * WS
                        );

                        if (min_clusters_dist > dist || min_clusters_dist == -1) {
                            min_clusters_dist = dist;
                        }
                    }
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

        cluster1->flowIDs = realloc(cluster1->flowIDs, (cluster1->size + cluster2->size) * sizeof(int));
        if (cluster1->flowIDs == NULL) {
            printf("Failed to realloc flowIDs when combining clusters %i, %i\n", nearest_cluster1, nearest_cluster2);
            return 1;
        }

        //(write cluster2 into cluster1 and update size)
        for (int i = 0; i < cluster2->size; i++) {
            cluster1->flowIDs[cluster1->size - 1 + i] = cluster2->flowIDs[i];
        }
        cluster1->size = cluster1->size + cluster2->size;

        // Delete cluster2
        free(cluster2->flowIDs);
        //free(cluster2);

        for (int i = nearest_cluster2+1; i < cluster_count; i++) {
            clusters[i-1] = clusters[i];
        }
        cluster_count--;
        clusters = realloc(clusters, cluster_count * sizeof(cluster));
        if (clusters == NULL) {
            printf("Failed to realloc clusters array after combining\n");
            return 1;
        }
    }

    //OUTPUT
    printf("Clusters:\n");
    for (int i = 0; i < cluster_count; i++) {
        printf("cluster %i:", i);
        for (int j = 0; j < clusters[i].size; j++) {
            printf(" %i", clusters[i].flowIDs[j]);
        }
        printf("\n");
    }

    return 0;
}
