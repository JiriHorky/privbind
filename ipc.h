#ifndef IPC_H
#define IPC_H

struct ipc_msg_req {
    enum { MSG_REQ_NONE, MSG_REQ_BIND } type;
    union {
        /* Each request type has one member here */	
        /* This is a bit tricky. We define all supported types + the generic one,
         * so the receiver can use the generic handle name, and we can be sure
	 * that the uniun will be big enough */
        struct { struct sockaddr addr; /* Address to bind */ } bind;
        struct { struct sockaddr_in addr; /* Address to bind */ } bind4;
        struct { struct sockaddr_in6 addr; /* Address to bind */ } bind6;
    } data;
};

struct ipc_msg_reply {
    enum { MSG_REP_NONE, MSG_REP_STAT } type;
    union {
	struct { int retval; int error; } stat;
    } data;
};

#endif /* IPC_H */
