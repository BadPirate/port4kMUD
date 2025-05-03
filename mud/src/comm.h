/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define NUM_RESERVED_DESCS	8

#define COPYOVER_FILE "copyover.dat"

/* comm.c */
void	send_to_all(char *messg);
void	send_to_char(char *messg, struct char_data *ch);
void	send_to_room(char *messg, int room);
void	send_to_outdoor(char *messg);
void	perform_to_all(char *messg, struct char_data *ch);
void	close_socket(struct descriptor_data *d);

void	perform_act(char *orig, struct char_data *ch, struct obj_data *obj,
		    void *vict_obj, struct char_data *to);

void	act(char *str, int hide_invisible, struct char_data *ch,
struct obj_data *obj, void *vict_obj, int type);

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define TO_WORLD    	5
#define TO_SLEEP	128	/* to char, even if sleeping */

int	write_to_descriptor(socket_t desc, char *txt);
void	write_to_q(char *txt, struct txt_q *queue, int aliased);
void	write_to_output(const char *txt, struct descriptor_data *d);
void	page_string(struct descriptor_data *d, char *str, int keep_internal);

#define SEND_TO_Q(messg, desc)  write_to_output((messg), desc)

#define USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  (!USING_SMALL(d))

typedef RETSIGTYPE sigfunc(int);


/* Function prototypes for brain-dead OS's */

#if defined(__COMM_C__) && defined(__GNUC__)

#ifndef accept
   int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
#endif

#ifndef bind
   int bind(int s, const struct sockaddr *addr, socklen_t addrlen);
#endif

#ifndef chdir
   int chdir(const char *path);
#endif

#ifndef close
   int close(int fd);
#endif

// #ifndef fcntl
//    int fcntl();
// #endif

#ifndef getpeername
   int getpeername(int s, struct sockaddr *name, socklen_t *namelen);
#endif

#ifndef getrlimit
   int getrlimit(int resource, struct rlimit *rlp);
#endif

#ifndef getsockname
   int getsockname(int s, struct sockaddr *name, socklen_t *namelen);
#endif

#ifndef htonl
   u_long htonl(u_long hostlong);
#endif

#ifndef htons
   u_short htons(u_short hostshort);
#endif

#ifndef listen
   int listen(int s, int backlog);
#endif

#ifndef ntohl
   u_long ntohl(u_long netlong);
#endif

// #ifndef read
//    int read();
// #endif

#ifndef select
   int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
#endif

#ifndef setitimer
   int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue);
#endif

#ifndef setrlimit
   int setrlimit(int resource, const struct rlimit *rlp);
#endif

#ifndef setsockopt
   int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
#endif

#ifndef socket
   int socket(int domain, int type, int protocol);
#endif

// #ifndef write
//    int write();
// #endif

#endif /* __COMM_C__ */
