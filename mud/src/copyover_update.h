/* *************************************************************************
*   File: copyover_update.h			     Addition to CircleMUD *
*  Usage: Pulls the whole mud/ tree from a given branch of the public     *
*         GitHub repo and rebuilds bin/circle in place, for use just      *
*         before do_copyover.                                             *
************************************************************************* */

/* Progress callback: invoked with a human-readable, \r\n-terminated
 * message as each step of the update starts/finishes (clone, world diff,
 * merge, autoconf, configure, make). May be NULL if the caller doesn't
 * want progress updates. */
typedef void (*copyover_status_fn)(void *ctx, const char *msg);

#define COPYOVER_UPDATE_OK             0
#define COPYOVER_UPDATE_ERROR         -1
/* lib/world differs between the live tree and the fetched branch and
 * 'confirmed' was false - nothing was touched or built. errbuf holds a
 * summary of what differs and how to re-run to confirm. */
#define COPYOVER_UPDATE_NEEDS_CONFIRM  1

/* Returns COPYOVER_UPDATE_OK on success. On failure, returns
 * COPYOVER_UPDATE_ERROR and writes a short diagnostic (including
 * git/build output) into errbuf. Returns COPYOVER_UPDATE_NEEDS_CONFIRM,
 * without merging or building anything, if the fetched branch's
 * lib/world differs from the live lib/world and 'confirmed' is false -
 * pass confirmed=1 (e.g. the admin re-running with a "confirm" argument)
 * to proceed and overwrite it. Never touches the currently-running
 * binary - anything short of COPYOVER_UPDATE_OK leaves the live game
 * untouched. */
int copyover_update_source(const char *branch, int confirmed,
                            copyover_status_fn status_cb, void *status_ctx,
                            char *errbuf, size_t errbuf_size);
