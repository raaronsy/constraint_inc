### Constraint INC

The INC tree building technique is described in `R. Zhang, S. Rao, and T. Warnow (2018). New absolute fast converging phylogeny estimation methods with improved scalability and accuracy. To appear, Proceedings of WABI (Workshop on Algorithms for Bioinformatics) 2018`. It is an absolute fast converging (AFC) method for the problem of estimating the model tree given a distance matrix between the sequences. 

Constraint INC is described in the same paper. In addition to a distance matrix input, it takes in a set of leaf-disjoint constraint trees and output a tree over the union of all leafsets that agrees with the constraint trees. When the constraint trees are derived under AFC methods (such as maximum likelihood), constraint INC is also AFC. 

The implementation design can be summarized as. 1/ Build an MST from the distance matrix and obtain the Prim ordering and the maximum edge weight, 2/ Build the tree incrementally following the Prim ordering: 2.1/ Start with a 3-leaf tree that is the first 3 taxa in the Prim ordering. 2.2/ For each new leaf added, identify the constraint tree it comes from, then use information in the constraint tree to limit the subtree on the growing tree that it can be added to, in order to agree with the constraint tree. 2.3/ Use the maximum edge weight of the MST to perform quartet `voting` and select the edge with the highest vote to add the new taxon into.  

## Requirement
Constraint INC was built from standard C packages 
1. Make
2. GCC or other C compile (code was developed and tested with GCC) 

If you want to use the maximum likelihood extension of constraint INC then the following is also currently required:
1. PASTA (working to remove this dependency) (https://github.com/smirarab/pasta) 
2. FastTree2 (http://www.microbesonline.org/fasttree/FastTree.c)
3. Newick Utils (https://github.com/tjunier/newick_utils)
4. Extra scripts in the tools folder
5. RAxML (only if needed to run RAxML)

## Installation
Run `make` to generate the binaries `constraint_inc` and `ml`, used for the constraint INC and its ML extension respectively. The command for constraint INC is 
```
constraint_inc -i <input_distance_matrix> -o <output_prefix> -t <constraint_tree1> <constraint_tree2> ... 
```
(Note that as of August 2018, constraint_inc is no longer generated by the make file as we work to update the file).

The command for INC-ML is 
```
ml -i <input_alignment> -o <output_prefix> -t <initial_tree> -r <recompute_constraint_trees> 
   -d <distance_type> -n <approx_constraint_tree_size> -c <constraint_tree_method> 
   -q <quartet_method> -g <guide_tree>
```
The `-t`, `-c`, `-r`, `-g` and `-q` flags to INC-ML is optional, whereas the `-i`, `-o`, `-d` and `-n` flags must always be entered. Read the Options field for details of other flags as well as their default values. 

If you want to call `constraint_inc` or `ml` from any directory, make sure they are on your PATH variable (if you are using UNIX-based machine) or equivalent. 

When running `ml`, also make sure that all the dependencies (PASTA, FastTree2, Newick Utils, extra scripts) are also on your PATH variable. 

## INC_ML options
1. `-i` specifies input alignment. Please specify the full path.
2. `-o` specifies output tree. Please specify the full path. Any file with the same name will be overwritten.
3. `-t` specifies a starting tree for PASTA decomposition. Please specifies the full path. If no `-t` flag is detected, the code will generate a FastTree2 tree as `<output_prefix>first_tree.tree` in the current directory.
4. `-c` accepts either `no` or `subtree` or `fasttree` or `raxml`. `no` specifies that ML-INC is run without constraint trees (or all constraint trees are singletons). `subtree` specifies that the induced subtree of the initial tree is used as constraint trees. `fasttree` specifies that FastTree2 is run on subalignments to generate the constraint trees. `raxml` specifies that `raxml` is run on subalignments to generate the cosntraint trees. __Default__: `fasttree`.
5. `-r` accepts either `0` or `1`. If this flag is set to `0` (default), the constraint trees will be constructed and the code will look for `<output_prefix>ctree<number>.tree` in the current directory and parse them as constraint trees. If this flag is set to `1`, the method specified in `-c` (or the default method) is used to compute the constraint trees. __Default__: `1`. 
6. `-d` accepts either `logDet` or `K2P` or `JC` or `P`, which are the different distance models. This field must be set.
7. `-n` accepts a positive number that approximates the constraint tree size. This field must be set. Recommended values is `min(number_of_sequences / 4, 1000)`. 
8. `-q` accepts either `fpm` or `subtree` or `raxml` or `ml`. `fpm` specifies that Four Point Method on the input distance matrix is used to compute quartet trees. `raxml` specifies that RAxML is evoked on 4 sequences to compute the quartet tree. `ml` specifies that RAxML is evoked on 4 sequences to compute the quartet tree, then change the weighting of the quartet trees to `likelihood_of_best_tree / likelihood_of_second_best_tree`. When `subtree` is used, a 'guide tree' must be specified with `-g`. The code then uses the induced quartet on the guide tree to compute the constraint trees. Weighting still uses the distance matrix. __Default__: `fpm`
9. Voting protocol. Work is being done to have a flag for voting protocol. For the time being, go to line 287 of `traversal.c`. There should be a single number on that line. Use `0` if you want `0-1` voting. Use `1` if you want to weight the vote by `1/diam`. Use `2` if you want to weight the vote by `1/diam^2`. __Default__: `2`. There are also options to use another voting round, just uncomment the next function call and change the voting scheme accordingly. 

## INC_ML outputs
1. The final tree, as specified in `-o`. 
2. The initial tree, at `<output_prefix>first_tree.tree`.
3. All constraint trees, at `<output_prefix>ctree<number>.tree`, where `number` counts from `0` until `number_of_constraint_trees - 1`.
4. All cosntraint trees label, at `<output_prefix>ctree<number>.lab`, where `number` counts from `0` until `number_of_constraint_trees - 1`.
5. All cosntraint trees msa, at `<output_prefix>ctree<number>.msa`, where `number` counts from `0` until `number_of_constraint_trees - 1`.
6. Distance matrix input to INC at `<output_prefix>c_inc_input`.
7. If you use `subtree` for `-c`, a secondary matrix used to quickly compute induced quartets on the guide tree is generate at `<output_prefix>secondary_matrix`. 

## Format
1. The input distance matrix in `constraint_inc` is in PHYLIP format. 
2. All trees (input / output) are in Newick.
3. The input alignment for `ml` is in PASTA.

## License and Copyright
See the attached LICENSE and COPYRIGHT file for details. Note that some part of the program (scripts in the tools folder) was distributed under a different license

## Contact
The package is under constant development. Contact `thienle2@illinois.edu` for implementation questions/suggestions for the C code. 

## Python Implementation

INC and INC_NJ have been implemented in `python3` and the README is available in the `src/python` folder.
