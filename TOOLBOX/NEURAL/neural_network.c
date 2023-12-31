#include  "include/neural_network.h"

#include "../GENETIC/include/population.h"
#include "../GENERAL/include/sort.h"
#include "../GENERAL/include/matrix_math.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void create_neural_network(struct NNInput *input, struct NN *neural_network) {
    // create list of neurons sizes
    neural_network->layer_number = input->layer_number;
    neural_network->neurons_size = (int*)malloc(input->layer_number * sizeof(int));
    memcpy(neural_network->neurons_size, input->neurons_size, neural_network->layer_number * sizeof(int));

    // copy all layer info and neurons type
    neural_network->layer_type = (int*)malloc(neural_network->layer_number * sizeof(int));
    memcpy(neural_network->layer_type, input->layer_type, neural_network->layer_number * sizeof(int));

    neural_network->sd_neurons_types = (int**)malloc(input->sd_number * sizeof(int*));
    neural_network->SD_memory = (struct Matrix **)malloc(input->sd_number * sizeof(struct Matrix *));

    // here the SD memory is allocated
    int global_index_sd = 0;
    for(int i=0; i<neural_network->layer_number; i++){
        if(neural_network->layer_type[i] == 1){
            // first the sd_neurons_types is created to hold matrix of types of each neuron for future calculations
            // the logis is: 50% straight links, 25% S links and 25% D links
            neural_network->sd_neurons_types[global_index_sd] = (int*)malloc(neural_network->neurons_size[i] * sizeof(int));

            for(int j=0; j<neural_network->neurons_size[i]/2; j++){
                neural_network->sd_neurons_types[global_index_sd][j] = 0; // straight links
            }
            int mid_index_slice = neural_network->neurons_size[i]/2 + (neural_network->neurons_size[i] - neural_network->neurons_size[i]/2)/2;
            for(int j=neural_network->neurons_size[i]/2; j < mid_index_slice; j++){
                neural_network->sd_neurons_types[global_index_sd][j] = 1; // s links
            } for(int j=mid_index_slice; j<neural_network->neurons_size[i]; j++){
                neural_network->sd_neurons_types[global_index_sd][j] = 2; // d link
            }
            
            // now the memory matrix is created for future usage in caculations
            neural_network->SD_memory[global_index_sd] = (struct Matrix *)malloc(sizeof(struct Matrix));
            int *sizes_sd_matrix = (int*)malloc(2*sizeof(int));

            sizes_sd_matrix[0] = neural_network->neurons_size[i];
            sizes_sd_matrix[1] = 1;

            create_matrix(neural_network->SD_memory[global_index_sd], sizes_sd_matrix);

            // now all matrix is set to .0 to not crash first calculations
            for(int j=0; j<neural_network->neurons_size[i]; j++){
                neural_network->SD_memory[global_index_sd]->matrix[j][0] = .0;
            }
            global_index_sd++;
        }
    }

    // create de/normalization matrixes of the system
    neural_network->normalization_matrix   = (float**)malloc(2 * sizeof(float*));
    neural_network->denormalization_matrix = (float**)malloc(2 * sizeof(float*));

    for(int i = 0; i < 2; i++){
        neural_network->normalization_matrix[i]   = (float*)malloc(neural_network->neurons_size[0] * sizeof(float));
        neural_network->denormalization_matrix[i] = (float*)malloc(neural_network->neurons_size[neural_network->layer_number - 1] * sizeof(float));
        
        memcpy(neural_network->normalization_matrix[i],   input->normalization_matrix[i],   neural_network->neurons_size[0] * sizeof(float));
        memcpy(neural_network->denormalization_matrix[i], input->denormalization_matrix[i], neural_network->neurons_size[neural_network->layer_number - 1] * sizeof(float));
    }

    select_activation_function(&neural_network->func_ptr);

    // delete input structure
    clear_neural_network_input(input);

    neural_network->AW = (struct Matrix **)malloc((neural_network->layer_number - 1) * sizeof(struct Matrix *));
    neural_network->BW = (struct Matrix **)malloc((neural_network->layer_number - 1) * sizeof(struct Matrix *));

    // create size for all matrixes
    int layer_index = 0;
    neural_network->count_of_values = 0; 
    for(int i=0; i<neural_network->layer_number - 2; i++){
        neural_network->AW[i] = (struct Matrix*)malloc(sizeof(struct Matrix));
        neural_network->BW[i] = (struct Matrix*)malloc(sizeof(struct Matrix));

        int *sizes_AW = (int*)malloc(2*sizeof(int));
        int *sizes_BW = (int*)malloc(2*sizeof(int));

        sizes_AW[0] = neural_network->neurons_size[layer_index + 1];
        sizes_AW[1] = neural_network->neurons_size[layer_index];

        sizes_BW[0] = neural_network->neurons_size[layer_index + 1];
        sizes_BW[1] = 1;

        // add number of genes needed
        neural_network->count_of_values += neural_network->neurons_size[layer_index + 1] * neural_network->neurons_size[layer_index] + neural_network->neurons_size[layer_index + 1];

        create_matrix(neural_network->AW[i], sizes_AW);
        create_matrix(neural_network->BW[i], sizes_BW);

        layer_index += 2;
    }
}

void clear_neural_network(struct NN *neural_network) {
    // free matrixes
    for(int i=0; i<neural_network->layer_number - 2; i++){
       matrix_delete(neural_network->AW[i]);
       matrix_delete(neural_network->BW[i]);
    }
    free(neural_network->AW);
    free(neural_network->BW);

    // free de_normalization matrixes
    for(int i = 0; i < 2; i++){
        free(neural_network->normalization_matrix[i]);
        free(neural_network->denormalization_matrix[i]);
    }
    free(neural_network->normalization_matrix);
    free(neural_network->denormalization_matrix);

    // free last pointer
    free(neural_network->neurons_size);

    // free all sd memory
    int index_sd  = 0;
    for(int i=0; i<neural_network->layer_number; i++){
        if(neural_network->layer_type[i] == 1){
            free(neural_network->sd_neurons_types[index_sd]);
            matrix_delete(neural_network->SD_memory[index_sd]);
            index_sd++;
        }
    }
    free(neural_network->sd_neurons_types);
    free(neural_network->SD_memory);
    free(neural_network->layer_type);

    // delete full structure
    free(neural_network);
}

void clear_neural_network_input(struct NNInput *input){
    free(input->neurons_size);

    // free de_normalization matrixes
    for(int i = 0; i < 2; i++){
        free(input->normalization_matrix[i]);
        free(input->denormalization_matrix[i]);
    }
    free(input->normalization_matrix);
    free(input->denormalization_matrix);

    // free the layer type matrix
    free(input->layer_type);

    // delete full strcture
    free(input);
}

void fill_matrixes_nn(struct NN *neural_network, float *population){
    int global_index = 0;
    
    for(int i=0; i<neural_network->layer_number-1; i++){
        // first the AW matrix is set
        for(int x=0; x<neural_network->AW[i]->sizes[0]; x++){
            for(int y=0; y<neural_network->AW[i]->sizes[1]; y++){
                neural_network->AW[i]->matrix[x][y] = population[global_index];
                global_index++;
            }
        }

        // second the BW is set
        for(int x=0; x<neural_network->BW[i]->sizes[0]; x++){
            for(int y=0; y<neural_network->BW[i]->sizes[1]; y++){
                neural_network->BW[i]->matrix[x][y] = population[global_index];
                global_index++;
            }
        }
    }
}

void de_normalization_process(struct NN *neural_network, struct Matrix *input, int way){
    float r_min, r_max, t_min, t_max;
    for(int i=0; i<neural_network->neurons_size[0]; i++){
        if(way == 0){
            r_min = neural_network->normalization_matrix[1][i];
            r_max = neural_network->normalization_matrix[0][i];
            t_min = -1;
            t_max = 1;
        } else if(way == 1){
            r_min = -1;
            r_max = 1;
            t_min = neural_network->denormalization_matrix[1][i];
            t_max = neural_network->denormalization_matrix[0][i];
        }
        
        input->matrix[i][0] = ((input->matrix[i][0] - r_min)/(r_max - r_min)) * (t_max - t_min) + t_min;
    }
}

void one_calculation(struct NN *neural_network, struct Matrix *input, struct Matrix *output){

    for(int i=0; i<neural_network->layer_number - 1; i++){
        // perform W*input
        matrix_multiply(neural_network->AW[i], input, output);

        // delete the matrix to create new pointer for future loop 
        matrix_delete(input);
        struct Matrix *input = (struct Matrix*)malloc(sizeof(struct Matrix));

        if( i != neural_network->layer_number - 2){
            // perform  - Bias
            int type = 0; // for sub action
            matrix_subst_add(output, neural_network->BW[i], input, type);

            // clear the output
            matrix_delete(output);
            struct Matrix *output = (struct Matrix*)malloc(sizeof(struct Matrix));
            
            // perform activation function, created input for next action
            matrix_all_values_formula(input, neural_network->func_ptr);
        } else {
            // perform activation avoiding Bias for last calculation
            matrix_all_values_formula(output, neural_network->func_ptr);
        }

    }
}