#ifndef IPC_H
#define IPC_H

struct ipc_msg_req {
    enum { MSG_REQ_NONE, MSG_REQ_BIND } type;
    union {
	/* Each request type has one member here */
	struct { struct sockaddr_in addr; /* Address to bind */ } bind;
    } data;
};

struct ipc_msg_reply {
    enum { MSG_REP_NONE, MSG_REP_STAT } type;
    union {
	struct { int retval; int error; int calls_left; } stat;
    } data;
};

#endif /* IPC_H */
