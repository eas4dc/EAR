
# EARD Remote API

The remote API contains three sections: the server portion (eard_server_api/dynamic_configuration), the client portion (eard_rapi) and the internal functions (eard_rapi_internals).

## Basics
We define a client as something which sends a request (be it just the request or expecting data afterwards) to an external source. Conversely, a server is defined by something which receives a message with or without an expectation from the sender to receive data in return.

The global structure goes as follows:
  - *Commands and EARGMD* are always considered to be clients, since they never receive communication without sending a request for it first. Instead they always initiate the communication by sending requests (internally request_t) to EARDs.
  - *EARDs* are both servers and clients. They act as servers when they receive and process a request, but also as clients when they propagate said request and, if necessary, aggregate the returning data.

A non-data global request (like restore-conf, for example) goes through the following path:
  - Either a command or EARGMD sends the message to NUM_PROPS (defined at compile-time) nodes of each range. Then, these nodes propagate the message up to NUM_PROPS nodes, and so on and so forth until it reaches a node that does not have to propagate any further.

A global data request (like status) follows the same structure, but the propagating nodes receive data from those whom they communicate with, which in turn they aggregate and return to whoever send them the request. 

There is a thorough explanation of the propagation method later.

### Specific node requests
This are requests that are to be sent only to a specified number of nodes. They go through a similar flow as global messages, but differ in their propagation method. More on that later




## Internals
### Files: eard_rapi_internals.c/h

This section contains all functionality related to: sending, receiving and processing data, connecting and disconnecting to a single server (an eard) as well as correcting propagation errors. Propagation themselves are not found in these files since it is a functionality exclusive to servers, and not clients. It also includes a lower-level function to send data to all nodes of a cluster_conf, which uses the correction function in case of failure at any point. 

All requests are sent as data, with the corresponding header indicating they are requests. Additionally, not a fully request_t is used, but instead an internal_request_t that contains only the necessary data is sent.


Relevant functions:
  - eards_remote_connect/disconnect: connects/disconnects to a specified node.
  - send_data: sends data through a currently open socket.
  - send_command: sends a request_t through a currently open socket. Internally this uses send_data.
  - send_command_all: sends a request_t to all nodes in a cluster_conf, using propagation. Internally uses send_command and eard_remote_connect/disconnect.
  - receive_data: reads_data from a socket. It first read a header that indicates the type and lenght of data to be read
  - process_data: aggregates in the appropiate form the data received (concatenates status, but add up the values of power_status, for example)
  - data_all_nodes: same as send_command_all, but expects data to be returned from the nodes. It uses send_command, receive_data  and process_data internally. 
  - internal_send_command_nodes: same as send_command_all, but instead of sending to the whole cluster it sends only to a number of specified nodes.
  - send_command_nodes: higher level function that prepares a request_t to be sent with internal_send_command_nodes.

## Remote API
### Files: eard_rapi.c/h

This file contains high-level functions that send requests (either globally or to a specific node) using the internal functions.

Some examples would be:
  - eards_set_max_freq: Sets the max frequency of a single node. Uses send_command internally.
  - set_max_freq_all_nodes: Sets the max frequency of all nodes. Uses send_command_all internally.
  - eards_get_status: Requests and reads the status of a single node. Uses send_command and receive_data internally.
  - status_all_nodes: Requests and reads the status of all nodes. Uses data_all_nodes internally.

## Server API
### Files: dynamic_configuration.c/h, eard_server_api.c/h

eard_server_api contains the functions to create the server socket as well as wait for a client request, read said request, send ack back to the client, and propagate received requests to other nodes.

Relevant functions:
  - create_server_socket: self explanatory.
  - close_server_socket: self explanatory.
  - wait_for_client: sets the socket to accept any connection and waits for it. Returns when a connection is made.
  - read_command: reads from the incoming connection a request and returns its code. Uses receive_data internally.
  - send_answer: sends ack back to the client if the command was read correctly.
  - init_ips: initializes a list of IPs to which this node will propagate the nodes to.
  - propagate_request: receives a command and propagates it, if necessary, to the pre-set nodes. Uses send_command internally.
  - propagate_data: similar to propagate_request, but expects data to be returned from the connection. Uses send_command, receive_data and process_data internally.
  - propagate\_\*: all other propagate functions are command-specific functions that use propagate_data internally.

dynamic_configuration uses eard_server_api functions and processes the requests returned from read_command.


## Propagation

There are two types of propagation: global (to all nodes in a cluster) and specific (to a list of nodes)

### Global
Global propagation starts with the client (command or EARGMD) sending the request to the first NUM_PROPS nodes of each range (as defined in ear.conf). Every node has the list of IPs of all the nodes in its range. This could easily be changed to be Island-wide instead of range-wide, which would be faster in cases where the ranges are small.
In turn, the nodes that have received the request will propagate it to NUM_PROPS nodes each, for a total of NUM_PROPS^2 of nodes being contacted in the second wave. Then, with a total of 15 nodes and NUM_PROPS = 3, the nodes will be contacted in this order:

```
N00 N01 N02 N03 N04 N05 N06 N07 N08 N09 N10 N11 N12 N13 N14
 1   1   1   2   2   2   2   2   2   2   2   2   3   3   3
```

That said, the nodes calculate to whom they propagate to following: 

    current_distance = id - id%NUM_PROPS      where id is the position of the node within the list, with the first starting at 0 (EG: N04 has distance 3, but position 4)

    offset = id%NUM_PROPS                     

    current_distance*NUM_PROPS + i*NUM_PROPS + offset     where i goes from 1 to NUM_PROPS

With this and NUM_PROPS = 3, 

    N00 has distance 0 and offset 0, therefore it will propagate to N03, N06 and N09 (1, 2 and 3 * NUM_PROPS, respectively),

    N01 has distance 0 and offset 1, therefore it will propagate to N04, N07 and N10 (the same as N00 but +1 because of offset),

    N02 has distance 0 and offset 1, therefore it will propagate to N05, N08 and N11 (the same as N00 but +2 because of offset),

With the previous messages we have covered the entirety of N03-N011 on the second wave of messages.

    N03 has distance 3 and offset 0, therefore it will propagate to N12, N15 and N18 (3*NUM_PROPS + (1, 2, 3) * NUM_PROPS)

    N04 is the same but +1 because of offset, so it will contact N13, N16 and N19

    N05 is the same but +2 because of offset, so it will contact N14, N17 and N20

    N06 has distance 6 and offset 0, which makes it jump another point and contacts N21, N24 and N27

This system covers all nodes and propagates messages in an exponential way. This makes large ranges more appealing for non-data requests, but less so for ones with data response since the accumulated data slows down the process.

## Specific propagation

Specific propagation differs in that the nodes that one propagates to are not preset, but instead are received with the message. The receiving node then divides the list in NUM_PROPS sublists, so nodes will never receive the same message twice. The client might send more than NUM_PROPS nodes, depending on the number of nodes on the list and the number of propagations each node will do. This is calculated in the get_max_prop_group function, found in eard_rapi_internals.c 

An example from a node with NUM_PROPS = 3 would be:

   Node N00 receives the list with 15 nodes N00-N14. It first removes themselves from the list, then divides the list in NUM_PROPS and sends to the starting node of each sublist the message. 

   The lists look like: N01-N04, N05-N08, N09-N14
   
   N01 receives the message and propagates it to N02, N03 and N04, who receive a list only containing themselves. N05 is a similar case.

   N09 subdivides the list again and propagates similarly to N00.

   The current subdivision (using only NUM_PROPS is mildly inefficient since it does not distribute evenly (the first two list contain 4 elements each, while the last one contains 6), but this is the worst case scenario where the last list will contain up to NUM_PROPS-1 nodes more than the others.


This covers all the basics of EARDs remote API and its internal functionalities.




    




