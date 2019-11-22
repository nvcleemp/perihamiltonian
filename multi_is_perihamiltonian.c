/*
 * 
 * Copyright (C) 2019 Ghent University.
 */

/* This program reads graphs in multicode format from 
 * standard in and determines whether they are perihamiltonian.
 * 
 * 
 * Compile with:
 *     
 *     cc -o multi_is_perihamiltonian -O4 multi_is_perihamiltonian.c shared/multicode_base.c shared/multicode_input.c shared/multicode_output.c
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "shared/multicode_base.h"
#include "shared/multicode_input.h"
#include "shared/multicode_output.h"

unsigned long long int graph_count = 0;
unsigned long long int filtered_count = 0;

//================ PERIHAMILTONIAN ===================

boolean current_cycle[MAXN+1];
int vertex_status[MAXN+1];

/**
  * 
  */
boolean continue_cycle(GRAPH graph, ADJACENCY adj, int last, int remaining, int first) {
    int i;
    
    if(remaining==0){
        //TODO: use bitsets (although it appears this does not give a significant gain)
        for(i = 0; i < adj[last]; i++){
            if(graph[last][i]==first){
                return TRUE;
            }
        }
        return FALSE;
    }
    
    for(i = 0; i < adj[last]; i++){
        if(!current_cycle[graph[last][i]]){
            current_cycle[graph[last][i]]=TRUE;
            if(continue_cycle(graph, adj, graph[last][i], remaining - 1, first)){
                return TRUE;
            }
            current_cycle[graph[last][i]]=FALSE;
        }
    }
    
    return FALSE;
}

boolean start_cycle(GRAPH graph, ADJACENCY adj, int startVertex, int order){
    int i;
    //mark the start vertex as being in the cycle
    current_cycle[startVertex] = TRUE;
    for(i = 0; i < adj[startVertex]; i++){
        if(!current_cycle[graph[startVertex][i]]){
            current_cycle[graph[startVertex][i]]=TRUE;
            //search for cycle containing the edge (v, graph[v][i])
            if(continue_cycle(graph, adj, graph[startVertex][i], order - 2, startVertex)){
                return TRUE;
            }
            current_cycle[graph[startVertex][i]]=FALSE;
        }
    }
    current_cycle[startVertex] = FALSE;
    
    return FALSE;
}

boolean vertex_deleted_graph_is_hamiltonian(GRAPH graph, ADJACENCY adj, int remaining_order, int removed_vertex){
    if(removed_vertex==1){
        if(start_cycle(graph, adj, 2, remaining_order)){
            return TRUE;
        }
    } else if(start_cycle(graph, adj, 1, remaining_order)){
        return TRUE;
    }
    
    return FALSE;
}

boolean is_exceptional_vertex(GRAPH graph, ADJACENCY adj, int vertex){
    int i;
    //clear possible previous cycle
    for(i=0; i<=MAXN; i++){
        current_cycle[i] = FALSE;
    }

    current_cycle[vertex] = TRUE;
    //we mark v as visited, so it is as if it got removed
    return !vertex_deleted_graph_is_hamiltonian(graph, adj, graph[0][0]-1, vertex);
}

boolean original_graph_is_hamiltonian(GRAPH graph, ADJACENCY adj, int order){
    return start_cycle(graph, adj, 1, order);
}

#define EXCEPTIONAL 1
#define NONEXCEPTIONAL -1
#define UNKNOWN 0

boolean check_hamiltonicity = TRUE;

boolean is_perihamiltonian(GRAPH graph, ADJACENCY adj){
    int i, j, v;
    
    int order = graph[0][0];
    
    //clear possible previous cycle
    for(i=0; i<=MAXN; i++){
        current_cycle[i] = FALSE;
    }
    
    if(check_hamiltonicity && original_graph_is_hamiltonian(graph, adj, order)){
        return FALSE;
    }
    
    ADJACENCY count_unknown_neighbours;
    int max_degree = 0;
    int max_degree_vertex = INT_MAX;
    
    for(v = 1; v <= order; v++){
        vertex_status[v] = UNKNOWN;
        count_unknown_neighbours[v] = adj[v];
        if(adj[v] > max_degree){
            max_degree = adj[v];
            max_degree_vertex = v;
        }
    }
    
    while(max_degree > 0){
        if(is_exceptional_vertex(graph, adj, max_degree_vertex)){
            vertex_status[max_degree_vertex] = EXCEPTIONAL;
            for(i = 0; i < adj[max_degree_vertex]; i++){
                int current_vertex = graph[max_degree_vertex][i];
                count_unknown_neighbours[current_vertex]--;
                if(vertex_status[current_vertex]==UNKNOWN){
                    if(is_exceptional_vertex(graph, adj, current_vertex)){
                        vertex_status[current_vertex]=EXCEPTIONAL;
                    } else {
                        vertex_status[current_vertex]=NONEXCEPTIONAL;
                        for(j = 0; j < adj[current_vertex]; j++){
                            count_unknown_neighbours[graph[current_vertex][j]]--;
                        }
                    }
                }
                if(vertex_status[current_vertex]==EXCEPTIONAL){
                    return FALSE;
                }
            }
        } else {
            vertex_status[max_degree_vertex] = NONEXCEPTIONAL;
            for(i = 0; i < adj[max_degree_vertex]; i++){
                count_unknown_neighbours[graph[max_degree_vertex][i]]--;
            }
        }
        
        max_degree = 0;
        for(v = 1; v <= order; v++){
            if(vertex_status[v] == UNKNOWN && count_unknown_neighbours[v] > max_degree){
                max_degree = count_unknown_neighbours[v];
                max_degree_vertex = v;
            }
        }
    }
    
    return TRUE;
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "Checks graphs for being perihamiltonian.\n\n");
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options]\n\n", name);
    fprintf(stderr, "\nThis program can handle graphs up to %d vertices. Recompile with a larger\n", MAXN);
    fprintf(stderr, "value for MAXN if you need to handle larger graphs.\n\n");
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -s, --skip-ham-check\n");
    fprintf(stderr, "       Do not check whether the original graph is not hamiltonian.\n");
    fprintf(stderr, "    -f, --filter\n");
    fprintf(stderr, "       Filter graphs that are perihamiltonian.\n");
    fprintf(stderr, "    -i, --invert\n");
    fprintf(stderr, "       Invert the filter.\n");
    fprintf(stderr, "    -u n, --update n\n");
    fprintf(stderr, "       Give an update every n graphs.\n");
    fprintf(stderr, "    -h, --help\n");
    fprintf(stderr, "       Print this help and return.\n");
}

void usage(char *name) {
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "For more information type: %s -h \n\n", name);
}

/*
 * 
 */
int main(int argc, char** argv) {
    
    GRAPH graph;
    ADJACENCY adj;
    
    boolean do_filtering = FALSE;
    boolean invert = FALSE;
        
    int update = 0;
    

    /*=========== commandline parsing ===========*/

    int c;
    char *name = argv[0];
    static struct option long_options[] = {
        {"skip-ham-check", no_argument, NULL, 's'},
        {"update", required_argument, NULL, 'u'},
        {"invert", no_argument, NULL, 'i'},
        {"filter", no_argument, NULL, 'f'},
        {"help", no_argument, NULL, 'h'}
    };
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "hfiu:s", long_options, &option_index)) != -1) {
        switch (c) {
            case 's':
                check_hamiltonicity = FALSE;
                break;
            case 'u':
                update = atoi(optarg);
                break;
            case 'i':
                invert = TRUE;
                break;
            case 'f':
                do_filtering = TRUE;
                break;
            case 'h':
                help(name);
                return EXIT_SUCCESS;
            case '?':
                usage(name);
                return EXIT_FAILURE;
            default:
                fprintf(stderr, "Illegal option %c.\n", c);
                usage(name);
                return EXIT_FAILURE;
        }
    }
    if(!do_filtering && update){
        fprintf(stderr, "Updates are only available when filtering is enabled.\n");
        update = 0;
    }

    unsigned short code[MAXCODELENGTH];
    int length;
    while (readMultiCode(code, &length, stdin)) {
        decodeMultiCode(code, length, graph, adj);
        
        graph_count++;

        boolean value = is_perihamiltonian(graph, adj);
        if(do_filtering){
            if(invert && !value){
                filtered_count++;
                writeMultiCode(graph, adj, stdout);
            } else if(!invert && value){
                filtered_count++;
                writeMultiCode(graph, adj, stdout);
            }
            if(update && !(graph_count % update)){
                fprintf(stderr, "Read: %llu. Filtered: %llu\n", graph_count, filtered_count);
            }
        } else {
            if(value){
                fprintf(stdout, "Graph %llu is perihamiltonian.\n", graph_count);
            } else {
                fprintf(stdout, "Graph %llu is not perihamiltonian.\n", graph_count);
            }
        }
    }
    
    fprintf(stderr, "Read %llu graph%s.\n", graph_count, graph_count==1 ? "" : "s");
    if(do_filtering){
        fprintf(stderr, "Filtered %llu graph%s.\n", filtered_count, filtered_count==1 ? "" : "s");
    }

    return (EXIT_SUCCESS);
}

