#ifndef ACTIVATION_FNC_H
#define ACTIVATION_FNC_H

// function to select activation function for pointer
void select_activation_function(float (**func_ptr)(float));

// for tests
void select_tang_activation_function(float (**func_ptr)(float));
void select_sigm_activation_function(float (**func_ptr)(float));
#endif