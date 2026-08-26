/* *************************************************************************
*   File: copyover_update.h			     Addition to CircleMUD *
*  Usage: Pulls the whole mud/ tree from a given branch of the public     *
*         GitHub repo and rebuilds bin/circle in place, for use just      *
*         before do_copyover.                                             *
************************************************************************* */

/* Returns 0 on success. On failure, returns nonzero and writes a short
 * diagnostic (including git/build output) into errbuf. Never touches the
 * currently-running binary - a failure here leaves the live game untouched. */
int copyover_update_source(const char *branch, char *errbuf, size_t errbuf_size);
