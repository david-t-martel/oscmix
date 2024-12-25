% Define the source C++ file
sourceFile = 'osc_rme.cpp';

% Define the include directories for liblo and oscmix
includeDirs = {
    '/path/to/liblo/include', % Replace with the actual path to your liblo include directory
    '/path/to/oscmix'         % Replace with the actual path to your oscmix directory
};

% Define the libraries to link against
libraries = {
    '-llo' % Link against the liblo library
};

% Construct the mex command with include directories and libraries
mexCommand = sprintf('mex %s -I"%s" %s', sourceFile, strjoin(includeDirs, '" -I"'), strjoin(libraries));

% Execute the mex command
eval(mexCommand);

% Display a success message
disp('MEX function compiled successfully!');