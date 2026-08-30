/* *************************************************************************
*   File: copyover_update.c			     Addition to CircleMUD *
*  Usage: Pulls the whole mud/ tree from a given branch of the public     *
*         GitHub repo, regenerates the build (autoconf/configure/make),   *
*         for use just before do_copyover.                                *
*                                                                          *
*  Everything shells out via fork()/execvp() with explicit argv arrays -  *
*  no shell is ever invoked, so the admin-supplied branch name can never  *
*  be interpreted as shell syntax regardless of content.                  *
*                                                                          *
*  Only files tracked by git are ever fetched/overwritten. Runtime save   *
*  data (player files, boards, houses, logs) lives under mud/lib and      *
*  mud/log but is gitignored, so it's never part of the fetched tree and  *
*  is never touched - the copy step only overwrites paths that exist in  *
*  the fetched tree, it never deletes anything.                          *
*                                                                          *
*  lib/world *is* tracked in git, and is also where live OLC building     *
*  happens, so before merging anything in we diff the fetched lib/world   *
*  against the live one and require explicit confirmation if they differ -*
*  see COPYOVER_UPDATE_NEEDS_CONFIRM.                                     *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "copyover_update.h"

#define REPO_URL "https://github.com/BadPirate/port4kMUD.git"
#define BUILD_LOG_PATH "/tmp/port4k-copyover-build.log"
#define DIFF_LOG_PATH "/tmp/port4k-copyover-diff.log"
#define TMP_TEMPLATE "/tmp/port4k-copyover-XXXXXX"
#define LOG_TAIL_BYTES 800
#define DIFF_HEAD_BYTES 3000

static void report_status(copyover_status_fn status_cb, void *status_ctx, const char *msg)
{
  if (status_cb)
    status_cb(status_ctx, msg);
}

static int is_valid_branch_name(const char *branch)
{
  size_t len = strlen(branch);
  size_t i;

  if (len == 0 || len > 100)
    return 0;
  if (branch[0] == '-' || branch[0] == '/')
    return 0;
  if (strstr(branch, ".."))
    return 0;

  for (i = 0; i < len; i++) {
    unsigned char c = (unsigned char) branch[i];
    if (!(isalnum(c) || c == '.' || c == '_' || c == '/' || c == '-'))
      return 0;
  }
  return 1;
}

/* Runs argv[0] with the given args, with stdout/stderr appended to
 * log_path. If workdir is non-NULL, the child chdir()s there first
 * (before exec, so no shell is needed to run a command in another
 * directory). Returns the child's exit code, or -1 if it couldn't even
 * be started/waited on. */
static int run_logged_command(char *const argv[], const char *log_path, const char *workdir)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid < 0)
    return -1;

  if (pid == 0) {
    int fd;

    if (workdir && chdir(workdir) < 0)
      _exit(126);

    fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      close(fd);
    }
    execvp(argv[0], argv);
    _exit(127); /* execvp failed */
  }

  if (waitpid(pid, &status, 0) < 0)
    return -1;

  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  return -1;
}

/* Appends the last LOG_TAIL_BYTES of log_path onto whatever's already in
 * errbuf, staying within errbuf_size. */
static void append_log_tail(const char *log_path, char *errbuf, size_t errbuf_size)
{
  FILE *fp;
  long size;
  size_t used = strlen(errbuf);
  size_t remaining;
  size_t to_read;
  char *dest;

  if (used >= errbuf_size - 1)
    return;
  remaining = errbuf_size - used - 1;

  fp = fopen(log_path, "r");
  if (!fp)
    return;

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  to_read = (size > LOG_TAIL_BYTES) ? LOG_TAIL_BYTES : (size_t) size;
  if (to_read > remaining)
    to_read = remaining;

  fseek(fp, -(long) to_read, SEEK_END);
  dest = errbuf + used;
  to_read = fread(dest, 1, to_read, fp);
  dest[to_read] = '\0';

  fclose(fp);
}

/* Appends up to DIFF_HEAD_BYTES from the *start* of log_path onto
 * whatever's already in errbuf, staying within errbuf_size. Used for the
 * world-diff listing, where the interesting part is the first files that
 * differ, not the last ones. */
static void append_log_head(const char *log_path, char *errbuf, size_t errbuf_size)
{
  FILE *fp;
  size_t used = strlen(errbuf);
  size_t remaining;
  size_t to_read;
  size_t got;

  if (used >= errbuf_size - 1)
    return;
  remaining = errbuf_size - used - 1;
  to_read = (remaining < DIFF_HEAD_BYTES) ? remaining : DIFF_HEAD_BYTES;

  fp = fopen(log_path, "r");
  if (!fp)
    return;

  got = fread(errbuf + used, 1, to_read, fp);
  errbuf[used + got] = '\0';

  fclose(fp);
}

int copyover_update_source(const char *branch, int confirmed,
                            copyover_status_fn status_cb, void *status_ctx,
                            char *errbuf, size_t errbuf_size)
{
  char tmpdir[64];
  char fetched_mud[PATH_MAX];
  char fetched_world[PATH_MAX];
  char cp_src[PATH_MAX];
  char msg[256];
  FILE *logfile;

  errbuf[0] = '\0';

  if (!branch || !*branch) {
    snprintf(errbuf, errbuf_size, "No branch specified.");
    return COPYOVER_UPDATE_ERROR;
  }
  if (!is_valid_branch_name(branch)) {
    snprintf(errbuf, errbuf_size, "Invalid branch name '%s'.", branch);
    return COPYOVER_UPDATE_ERROR;
  }

  /* Fresh log for this attempt. */
  logfile = fopen(BUILD_LOG_PATH, "w");
  if (logfile)
    fclose(logfile);

  strcpy(tmpdir, TMP_TEMPLATE);
  if (!mkdtemp(tmpdir)) {
    snprintf(errbuf, errbuf_size, "Failed to create temp directory: %s", strerror(errno));
    return COPYOVER_UPDATE_ERROR;
  }

  snprintf(msg, sizeof(msg), "Cloning branch '%s' from the public repo...\r\n", branch);
  report_status(status_cb, status_ctx, msg);

  {
    char *argv[] = {
      "git", "clone", "--depth", "1", "--branch", (char *) branch,
      "--single-branch", "--filter=blob:none", "--sparse",
      REPO_URL, tmpdir, NULL
    };
    if (run_logged_command(argv, BUILD_LOG_PATH, NULL) != 0) {
      snprintf(errbuf, errbuf_size, "git clone failed (branch '%s' not found?):\n", branch);
      append_log_tail(BUILD_LOG_PATH, errbuf, errbuf_size);
      goto fail_cleanup;
    }
  }

  {
    char *argv[] = { "git", "-C", tmpdir, "sparse-checkout", "set", "mud", NULL };
    if (run_logged_command(argv, BUILD_LOG_PATH, NULL) != 0) {
      snprintf(errbuf, errbuf_size, "git sparse-checkout failed:\n");
      append_log_tail(BUILD_LOG_PATH, errbuf, errbuf_size);
      goto fail_cleanup;
    }
  }

  snprintf(fetched_mud, sizeof(fetched_mud), "%s/mud", tmpdir);
  if (access(fetched_mud, F_OK) != 0) {
    snprintf(errbuf, errbuf_size, "Branch '%s' has no mud/ directory.", branch);
    goto fail_cleanup;
  }

  /* lib/world is tracked in git *and* is where live OLC building happens,
   * so it's the one place a copyover can silently clobber work nobody's
   * committed yet. Diff it before merging anything in, and refuse to
   * proceed without explicit confirmation if it differs. */
  report_status(status_cb, status_ctx, "Comparing world files against the live tree...\r\n");

  snprintf(fetched_world, sizeof(fetched_world), "%s/lib/world", fetched_mud);
  {
    FILE *dl = fopen(DIFF_LOG_PATH, "w");
    if (dl)
      fclose(dl);
  }
  {
    char *argv[] = { "diff", "-rq", fetched_world, "world", NULL };
    int diff_rc = run_logged_command(argv, DIFF_LOG_PATH, NULL);

    if (diff_rc != 0) {
      if (!confirmed) {
        char *cleanup_argv[] = { "rm", "-rf", tmpdir, NULL };

        snprintf(errbuf, errbuf_size,
                 "World files differ between the live game and branch '%s':\n\n", branch);
        append_log_head(DIFF_LOG_PATH, errbuf, errbuf_size);
        snprintf(errbuf + strlen(errbuf), errbuf_size - strlen(errbuf),
                 "\nRe-run as 'copyover %s confirm' to overwrite the world files above.\n", branch);

        run_logged_command(cleanup_argv, BUILD_LOG_PATH, NULL);
        return COPYOVER_UPDATE_NEEDS_CONFIRM;
      }
      report_status(status_cb, status_ctx, "World files differ - overwriting because confirmed.\r\n");
    } else {
      report_status(status_cb, status_ctx, "World files unchanged.\r\n");
    }
  }

  /* Merge the fetched (tracked-files-only) tree over the live mud/ tree.
   * cp only adds/overwrites paths present in the source - it never deletes,
   * so anything gitignored (player saves, boards, houses, logs, mud/bin,
   * mud/lib-dist) that isn't part of the fetched tree is left untouched. */
  report_status(status_cb, status_ctx, "Merging fetched tree into the live mud/ tree...\r\n");

  snprintf(cp_src, sizeof(cp_src), "%s/.", fetched_mud);
  {
    char *argv[] = { "cp", "-a", cp_src, "../", NULL };
    if (run_logged_command(argv, BUILD_LOG_PATH, NULL) != 0) {
      snprintf(errbuf, errbuf_size, "Failed to copy fetched source into mud/:\n");
      append_log_tail(BUILD_LOG_PATH, errbuf, errbuf_size);
      goto fail_cleanup;
    }
  }

  {
    char *argv[] = { "rm", "-rf", tmpdir, NULL };
    run_logged_command(argv, BUILD_LOG_PATH, NULL); /* best-effort, not fatal */
  }

  /* Regenerate configure/Makefile/conf.h from the (possibly updated)
   * configure.in/Makefile.in, exactly as the Dockerfile does at image-build
   * time - don't just trust whatever Makefile/conf.h happened to be
   * committed. Both run with cwd = mud/ (we're in mud/lib). */
  report_status(status_cb, status_ctx, "Running autoconf...\r\n");
  {
    char *argv[] = { "autoconf", NULL };
    if (run_logged_command(argv, BUILD_LOG_PATH, "..") != 0) {
      snprintf(errbuf, errbuf_size, "autoconf failed after updating from branch '%s':\n", branch);
      append_log_tail(BUILD_LOG_PATH, errbuf, errbuf_size);
      return COPYOVER_UPDATE_ERROR;
    }
  }

  report_status(status_cb, status_ctx, "Running ./configure...\r\n");
  {
    char *argv[] = { "./configure", NULL };
    if (run_logged_command(argv, BUILD_LOG_PATH, "..") != 0) {
      snprintf(errbuf, errbuf_size, "./configure failed after updating from branch '%s':\n", branch);
      append_log_tail(BUILD_LOG_PATH, errbuf, errbuf_size);
      return COPYOVER_UPDATE_ERROR;
    }
  }

  report_status(status_cb, status_ctx, "Building (make)...\r\n");
  {
    char *argv[] = { "make", "-C", "../src", NULL };
    if (run_logged_command(argv, BUILD_LOG_PATH, NULL) != 0) {
      snprintf(errbuf, errbuf_size,
               "Build failed after updating from branch '%s' - the running server is untouched:\n",
               branch);
      append_log_tail(BUILD_LOG_PATH, errbuf, errbuf_size);
      return COPYOVER_UPDATE_ERROR;
    }
  }

  report_status(status_cb, status_ctx, "Build succeeded.\r\n");

  return COPYOVER_UPDATE_OK;

fail_cleanup:
  {
    char *argv[] = { "rm", "-rf", tmpdir, NULL };
    run_logged_command(argv, BUILD_LOG_PATH, NULL);
  }
  return COPYOVER_UPDATE_ERROR;
}
