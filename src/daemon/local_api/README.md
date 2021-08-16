## Local communication EARD-EARL

- EARD uses three named pipes to accept connections from EARL. Only one process per node can connect with ERD, the rest of connections are rejected. The name of the pipes are:
- $EAR_TMP/.ear_comm.req_0
- $EAR_TMP/.ear_comm.ack_0.ID where ID is a mix of jobid and stepd
- $EAR_TMP/.ear_comm.ping.ID where ID is a mix of jobid and stepd

Req and Ack are pipes are created by EARD, ping pipe is created by the application. ping pipe is used to detect the application is not responding. The main function in EARD includes a timeout in the select. In case this timeout expires, the EARD checks the application is still alive by sending one byte to the ping pipe. 

- EARD uses one single thread (the main thread) for local requests. If one request blocks this threads, no more connections or requests will be accepted. 
- Current version will not support N threads for requests given metrics are not thread safe
	- It is pending to migrate the functions for average cpu frequency computation 
- The algorithm for connection is the following one:
 * EARD waits in comm.req pipe for new connections (select in main function). Once a new data is received, if accepted, EARD creates the ack pipe opens the ping pipe and send one byte to the ping pipe for synchronization and then it opens the ack pipe. 
* The EARL(through daemon/local_api/eard_api.c) opens the req pipe. Once opened creates and opens the ping pipei (RDWR to avoid blocking here). Then it sends the data connection to the req pipe and waits for the ack in the ping pipe. This ping means the connection has been accepted. Once accepted it opens the ack pipe. 





