// osc_rme.cpp

#include "mex.h"
#include "lo/lo.h"
#include "oscmix.h"

#define OSC_BUFFER_SIZE 1024
#define OSC_SEND_PORT 50000 // Standard OSC send port
#define OSC_RECV_PORT 50001 // Standard OSC receive port

// Function to open an OSC connection (updated for localhost)
void osc_open(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Check for proper number of arguments
    if (nrhs != 0)
    {
        mexErrMsgIdAndTxt("osc:open:nrhs", "No inputs required.");
    }
    if (nlhs != 2)
    {
        mexErrMsgIdAndTxt("osc:open:nlhs", "Two outputs required.");
    }

    // Use localhost IP address
    const char *ip_address = "127.0.0.1";

    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        mexErrMsgIdAndTxt("osc:open:socket", "Error opening socket.");
    }

    // Set up the server address structure (for sending)
    sockaddr_in send_addr;
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(OSC_SEND_PORT);
    send_addr.sin_addr.s_addr = inet_addr(ip_address);

    // Bind the socket to the receive port
    sockaddr_in recv_addr;
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(OSC_RECV_PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0)
    {
        mexErrMsgIdAndTxt("osc:open:bind", "Error binding socket.");
    }

    // Output the socket file descriptor and send address
    plhs[0] = mxCreateDoubleScalar(sockfd);
    plhs[1] = mxCreateNumericMatrix(1, sizeof(send_addr), mxUINT8_CLASS, mxREAL);
    memcpy(mxGetData(plhs[1]), &send_addr, sizeof(send_addr));
}

// Function to send an OSC message (updated to use send_addr)
void osc_send_message(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Check for proper number of arguments
    if (nrhs < 4)
    {
        mexErrMsgIdAndTxt("osc:send_message:nrhs", "At least four inputs required.");
    }
    if (nlhs != 0)
    {
        mexErrMsgIdAndTxt("osc:send_message:nlhs", "No output required.");
    }

    // Get input arguments (socket file descriptor, send address, OSC address, type tags, and data)
    int sockfd = (int)mxGetScalar(prhs[0]);
    sockaddr_in *send_addr = (sockaddr_in *)mxGetData(prhs[1]); // Get send address
    char *address = mxArrayToString(prhs[2]);
    char *type_tags = mxArrayToString(prhs[3]);

    // Create an OSC message
    lo_message msg = lo_message_new();

    // Pack data into the message based on type tags
    int data_index = 4; // Start from the fourth input argument
    for (int i = 0; type_tags[i] != '\0'; i++)
    {
        switch (type_tags[i])
        {
        case 'i':
        {
            int value = (int)mxGetScalar(prhs[data_index++]);
            lo_message_add(msg, "i", value);
            break;
        }
        case 'f':
        {
            float value = (float)mxGetScalar(prhs[data_index++]);
            lo_message_add(msg, "f", value);
            break;
        }
        // Add more cases for other data types as needed (s, b, etc.)
        default:
            mexErrMsgIdAndTxt("osc:send_message:type_tag", "Unsupported type tag.");
        }
    }

    // Send the OSC message using sendto()
    int result = sendto(sockfd, lo_message_serialise(msg), lo_message_length(msg), 0,
                        (struct sockaddr *)send_addr, sizeof(*send_addr));
    if (result == -1)
    {
        mexErrMsgIdAndTxt("osc:send_message:send", "Error sending OSC message.");
    }

    // Clean up
    lo_message_free(msg);
    mxFree(address);
    mxFree(type_tags);
}

// Function to read a parameter value (experimental)
void osc_read_param(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Check for proper number of arguments
    if (nrhs != 2)
    {
        mexErrMsgIdAndTxt("osc:read_param:nrhs", "Two inputs required.");
    }
    if (nlhs != 1)
    {
        mexErrMsgIdAndTxt("osc:read_param:nlhs", "One output required.");
    }

    // Get input arguments (socket file descriptor and OSC address)
    int sockfd = (int)mxGetScalar(prhs[0]);
    char *address = mxArrayToString(prhs[1]);

    // Create an OSC address
    lo_address osc_address = lo_address_new_from_sockfd(sockfd);

    // Send an OSC message to request the parameter value (experimental)
    // (This might need to be adjusted based on the RME's response behavior)
    int result = lo_send_message(osc_address, address, NULL);
    if (result == -1)
    {
        mexErrMsgIdAndTxt("osc:read_param:send", "Error sending OSC message.");
    }

    // Receive the OSC response (experimental)
    char buffer[OSC_BUFFER_SIZE];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    result = lo_recv_from(sockfd, buffer, OSC_BUFFER_SIZE, (struct sockaddr *)&from, &fromlen);
    if (result < 0)
    {
        mexErrMsgIdAndTxt("osc:read_param:recv", "Error receiving OSC message.");
    }

    // Parse the OSC response and extract the value (experimental)
    // This is where you need to analyze the actual OSC response from the RME device
    // and implement the correct parsing logic.
    // For now, assume the response is a single float value:
    lo_message msg = lo_message_deserialise(buffer, result, &from);
    if (msg == NULL)
    {
        mexErrMsgIdAndTxt("osc:read_param:deserialize", "Error deserializing OSC message.");
    }

    float value = 0.0f;
    if (lo_message_get_arg(msg, 0, "f", &value) == -1)
    {
        mexErrMsgIdAndTxt("osc:read_param:get_arg", "Error getting float argument from OSC message.");
    }

    // Output the value
    plhs[0] = mxCreateDoubleScalar(value);

    // Clean up
    lo_message_free(msg);
    lo_address_free(osc_address);
    mxFree(address);
}

// Function to close the OSC connection
void osc_close(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Check for proper number of arguments
    if (nrhs != 1)
    {
        mexErrMsgIdAndTxt("osc:close:nrhs", "One input required.");
    }
    if (nlhs != 0)
    {
        mexErrMsgIdAndTxt("osc:close:nlhs", "No output required.");
    }

    // Get input argument (socket file descriptor)
    int sockfd = (int)mxGetScalar(prhs[0]);

    // Close the socket
    close(sockfd);
}

// Gateway function for MEX
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // Get the command string
    if (nrhs < 1)
    {
        mexErrMsgIdAndTxt("osc:nrhs", "At least one input required.");
    }
    char *cmd = mxArrayToString(prhs[0]);

    // Call the appropriate function based on the command
    if (strcmp(cmd, "open") == 0)
    {
        osc_open(nlhs, plhs, nrhs - 1, prhs + 1);
    }
    else if (strcmp(cmd, "send_message") == 0)
    {
        osc_send_message(nlhs, plhs, nrhs - 1, prhs + 1);
    }
    else if (strcmp(cmd, "read_param") == 0)
    {
        osc_read_param(nlhs, plhs, nrhs - 1, prhs + 1);
    }
    else if (strcmp(cmd, "close") == 0)
    {
        osc_close(nlhs, plhs, nrhs - 1, prhs + 1);
    }
    else
    {
        mexErrMsgIdAndTxt("osc:cmd", "Unknown command.");
    }

    mxFree(cmd);
}