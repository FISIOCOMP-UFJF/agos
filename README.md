# agos
Cellml to code converter

# Compile

`cmake .`

`make`

# Run
`./bin/agos -i cellml/model.cellml -o model -s -l python`

This will generate a single cell solver for the model. To run the solver:

`python model_single_cell_solver.py [simulation_time]`

Ex: 

`python model.py 1000`

will run a simulation of 1000 milliseconds.

It is also possible to generate a C solver that uses CVODE.

`./bin/agos -i cellml/model.cellml -o model -s -l c`

to compile the solver:

`gcc model_single_cell_solver.c -o model -lm -lsundials_cvode`

to run the solver:

`./model [simulation_time]`

# Results

The model state variables will be saved in a file named out.txt

You can use gnuplot to plot the results: 

`gnuplot`

`gnuplot> plot 'out.txt' u 1:2 w lines` 

