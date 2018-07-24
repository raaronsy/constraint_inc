// File in HMMDecompositionDecision, created by Thien Le in July 2018

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "utilities.h"
#include "options.h"


// Global variable for quickly intializing a tree
BT_edge adj_list[maxN][3];
int degree[maxN];
int parent_map[maxN];
int master_idx_map[maxN];

// Private functions
BT * tree_constructor(int n, BT_edge ** adj_list, int * degree, int * parent_map, char ** name_map);
BT * read_newick(MAP_GRP map, char * filename, int tree_idx);

/* Read all constraint trees at once into memory
 * Input:       meta        meta variables, including the pointers to constraint trees in memory
 *              map         map variables, including the indexing map and name map
 *              options     user input options, including the name of constraint trees
 * Output: 0 on success, ERROR otherwise
 * Effect: allocate some memories, build some trees, open some files, init 2 arrays in map and 1 in meta 
 */
int parse_tree(INC_GRP * meta, MAP_GRP * map, option_t * options){
    int i;  // loop variable

    // Init meta's field
    meta->n_ctree = options->num_tree;

    // Malloc sequence
    meta->ctree             = malloc(meta->n_ctree * sizeof(BT *));
    map->master_to_ctree    = malloc(meta->n_taxa  * sizeof(int));
    map->master_to_cidx     = malloc(meta->n_taxa  * sizeof(int))

    if(!meta->ctree)            PRINT_AND_RETURN("malloc failed for constraint trees in parse_tree", MALLOC_ERROR);
    if(!map->master_to_ctree)   PRINT_AND_RETURN("malloc failed for master_to_ctree in parse_tree", MALLOC_ERROR);
    if(!map->master_to_cidx)    PRINT_AND_RETURN("malloc failed for master_to_cidx in parse_tree", MALLOC_ERROR);

    // Init
    for(i = 0; i < meta->n_taxa; i++){
        map->master_to_ctree[i]     = -1;
        map->master_to_cidx[i]      = -1;
    }

    // Call the newick reader
    for(i = 0; i < meta->n_ctree; i++) 
        meta->ctree[i]      = read_newick(meta, options->tree_name[i], i);
    
    return 0;
}


int init_in_building(int * in_building, int n){
    int i;
    in_building = malloc(n * sizeof(int));
    for(i = 0; i < n; i++)
        in_building[i] = 0;

    return 0;
}

// Assuming that there is already an ordering with at least 3 nodes, create a star with 3 leaves
int init_growing_tree(BT * tree, int * ordering, int * in_building, int num_sequence, int adjacent_in_mst){ 
    int i;

    // TODO: check failure here
    tree = tree_constructor(4 * num_sequence,  NULL, NULL, NULL); // allocate the maximum number of nodes for mallocation to reserve enough space for tree addition

    // Override tree logistics since there are only 4 nodes in the tree, the rest can be viewed as singletons
    tree->n = 4;

    make_adjacent(0, 3, tree);
    make_adjacent(1, 3, tree);
    make_adjacent(2, 3, tree);

    for(i = 0; i < 3; i++){
        tree->master_idx_map[i] = ordering[i];
        in_building[ordering[i]] = 1;

        tree->adj_list[3][i].dest_sample = tree->adj_list[i][3].src_sample = i;
    }
    
    tree->adj_list[3][0].src_sample = tree->adj_list[0][3].dest_sample = 1;
    tree->adj_list[3][1].src_sample = tree->adj_list[1][3].dest_sample = 0;


    // TODO: figure out how to set this efficiently
    tree->adj_list[3][2].src_sample = tree->adj_list[2][3].dest_sample = adjacent_in_mst;


    return 0;
}


int attach_leaf_to_edge(BT * growing_tree, int x, int addition_edge_parent, int additional_edge_child, int adjacent_in_mst){
    // Changing tree logistics (if this fails or terminate in the middle of some process then we are screwed)
    // Because of the way we mallocate the growing tree, there are already memory for the extra 2 nodes 
    growing_tree->n += 2;

    // Subdivide the edge
    swap_adajcency(additional_edge_parent, growing_tree->n - 2, additional_egde_child, growing_tree);
    swap_adajcency(additional_edge_child, growing_tree->n - 2, additional_edge_parent, growing_tree);
    make_adjacent(growing_tree->n - 1, growing_tree->n - 2, growing_tree);

    // Set up the leaf sample properly
    growing_tree->adj_list[growing_tree->n - 2][growing_tree->n - 1].dest_sample = growing_tree->n - 1;
    growing_tree->adj_list[growing_tree->n - 1][growing_tree->n - 2].dest_sample = adjacent_in_mst;

}

// INTERNAL FUNCTION IMPLEMENTATIONS
BT * tree_constructor(int n, BT_edge ** adj_list, int * degree, int * parent_map, char ** name_map){
    // The arrays mayeb null (in case of null construction)
    int i, j;

    BT * tree =     malloc(sizeof(BT)); 

    if(!tree)           PRINT_AND_RETURN("malloc error in tree_constructor", NULL);

    tree->n                     = n;

    tree->adj_list              = malloc(n * sizeof(BT_edge *));
    tree->degree                = malloc(n * sizeof(int));
    tree->master_idx_map        = malloc(n * sizeof(int));

    for(i = 0; i < n; i++){
        tree->adj_list[i] = malloc(3 * sizeof(BT_edge));
    }

    // Initialization 
    for(i = 0; i < n; i++){
        if(adj_list[i]) memcpy(tree->adj_list[i], adj_list[i], 3 * sizeof(BT_edge));
        else 
            for(j = 0; j < 3; j++){
                tree->adj_list[i][j].dest = -1;
                tree->adj_list[i][j].src_sample = -1;
                tree->adj_list[i][j].dest_sample = -1;
            }

        tree->master_idx_map[i]
                                    = -1;
        tree->degree[i]             = 0; 
    }

    if(degree)  
        memcpy(tree->degree, degree, n * sizeof(int));

    if(master_idx_map)
        memcpy(tree->master_idx_map, master_idx_map, n * sizeof(int));

    return tree;
}

#define READ_NAME_STATE 0
#define OTHER_STATE     1

#define incr_node(node, max_node)               node = ++max_node
#define incr_level(node_p, node_c, max_node)    do{node_p = node_c; incr_node(node_c, max_node);} while(0)
#define decr_level(node_p, node_c)              do{node_c = node_p; node_p = parent_map[node_c];} while(0)

BT * read_newick(MAP_GRP map, char * filename, int tree_idx){
    FILE *  f;

    // FSM variables
    int     cur_state;

    int     cur_node;
    int     max_node;
    int     parent_node;

    char    cur_char;
    char    cur_name[MAX_NAME_SIZE];

    // File opening
    f = freopen(filename, "r");
    if(!f)                      PRINT_AND_RETURN("fail to open in read_newick file",    NULL);

    // Intialization
    cur_state       = READ_NAME_STATE;

    node_counter    = 0; 
    cur_node        = 0;
    max_node        = 0;
    parent_node     = -1;

    cur_char        = 0;
    strclr(cur_name);

    // Initialize global variables
    init();

    while(fscanf(f, "%c", &cur_char) == 1){
        switch(cur_char){
            case '(': //start of a new level and start of a new node
                incr_level(parent_node, cur_node, max_node);
                if(make_parent(parent_node, cur_node, NULL)     != SUCCESS) PRINT_AND_RETURN("make parent failed in read newick\n", NULL);
                cur_state = READ_NAME_STATE;
                break;
            case ',': // start of new node end of old node
                if(save_name(cur_node, cur_name, map, tree_idx) != SUCCESS) PRINT_AND_RETURN("save name failed in read_newick\n", NULL);
                incr_node(cur_node, max_node);
                if(make_parent(parent_node, cur_node, NULL)     != SUCCESS) PRINT_AND_RETURN("make parent failed in read newick\n", NULL);
                cur_state = READ_NAME_STATE;
                break;
            case ')': // end of level
                if(save_name(cur_node, cur_name, map, tree_idx) != SUCCESS) PRINT_AND_RETURN("save name failed in read_newick\n", NULL);
                decr_level(parent_node, cur_node); 
                break;
            case ':': 
                cur_state = OTHER_STATE;
                break;
            default:
                if(cur_state == READ_NAME_STATE){
                    cat_c_to_a(cur_name, cur_char);
                }
        }
    }

    fclose(f);

    // Make the binary tree and return
    return tree_constructor(max_node + 1, adj_list, degree, NULL; name_map);
}


int init(){
    memset(adj_list, -1, sizeof(adj_list));
    memset(degree, 0, sizeof(degree));
    memset(master_idx_map, -1, sizeof(degree));
}

/* Helper function to check whether a node is in range
 * Input:   node to check
 * Output:  1 if the node is in range, 0 otherwise
 * Effect:  none
 */ 
int check(int node_a, BT * tree){
    if(0 <= node_a && node_a < tree->n) return 1;
    else return 0;
}

int delete_from_adj_list(BT_edge * list, int list_size, int item){
    int i;
    f
}

// Delete a-c and make a-b, b-c 
int swap_adajcency(int nod_a, int node_b, int node_c, BT* tree){
    int i, j;
    if(!check(node_a) || !check(node_b))  PRINT_AND_RETURN("node out of range in make_non_adjacent", GENERAL_ERROR);

    if(tree){
        for(i = 0; i < tree->deree[node_a]; i++)
            if(tree->adj_list[node_a][i].dest == node_c){
                tree->adj_list[node_a][i].dest = node_b; // this is the only thing we need to change since the index will be refreshed in the next iteration
                                                         // and the leaf sample remains the same
                break; // preserve i for later use
            }
        for(j = 0; j < tree->deree[node_c]; j++)
            if(tree->adj_list[node_c][j].dest == node_a){
                tree->adj_list[node_c][j].dest = node_b;
                break; // preserve j for later use
            }

        // Assuming b is new so its fields are empty
        tree->adj_list[node_b][degree[node_b]].dest = node_a;
        tree->adj_list[node_b][degree[node_b]].dest_sample = tree->adj_list[node_c][j].dest_sample; // we can do this since we know b is not a leaf
        degree[node_b]++;

        tree->adj_list[node_b][degree[node_b]++].dest = node_c;
        tree->adj_list[node_b][degree[node_b]].dest_sample = tree->adj_list[node_a][i].dest_sample; // we can do this since we know b is not a leaf
        degree[node_b]++;

    }
    return 0;
}

/* Helper function to make a node adjacent to another in the adjacency matrix
 * Input:   2 nodes
 * Output:  0 on success, ERROR otherwise
 * Effect:  none
 */ 
int make_adjacent(int node_a, int node_b, BT * tree){
    if(!check(node_a) || !check(node_b))  PRINT_AND_RETURN("node out of range in make_adjacent", GENERAL_ERROR);

    if(tree){
        tree->adj_list[node_a][degree[node_a]++].dest = node_b;
        tree->adj_list[node_b][degree[node_b]++].dest = node_a;
    } else {
        adj_list[node_a][degree[node_a]++].dest = node_b;
        adj_list[node_b][degree[node_b]++].dest = node_a;
    }
    

    return 0;
}

/* Helper function to make a node a parent of another 
 * Input:   2 nodes (order matters, the parent node comes first)
 * Output:  0 on success, ERROR otherwise
 * Effect:  none
 */ 
int make_parent(int node_p, int node_c, BT * tree){
    if(make_adjacent(node_p, node_c, tree) != SUCCESS)    PRINT_AND_RETURN("node out of range in make_parent", GENERAL_ERROR);

    parent_map[node_c] = node_p;
    return 0;
}

/* Helper function to save name for a numeric node and map it back to the master ordering
 * Input:   node number and its name 
 * Output:  0 on success
 * Effect:  none
 */ 
int save_name(int cur_node, char * name, MAP_GRP map, int tree_idx){
    int i; // store the master index

    for(i = 0; i < map->n_taxa; i++){
        if(strcmp(name, map->master_to_name[i]) != 0){
            if(i == num_sequence - 1) PRINT_AND_RETURN("no name matches when trying to map taxa in constraint trees", GENERAL_ERROR); // no name matches
        } else break;
    }

    map->master_to_ctree[i]     = tree_idx;
    map->master_to_cidx[i]      = cur_node;
    master_idx_map[cur_node]    = i;

    return 0;
}

