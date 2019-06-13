#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <sys/types.h>
#include <fcntl.h>

#include "middle.h"

struct trace_state {
    // TODO: Actually track file descriptors. This field just exists
    // currently so that we don't pass 0 to malloc.
    int _placeholder;
};

struct trace_state* trace_init() {
    return (struct trace_state*) malloc(sizeof(struct trace_state));
}

void trace_add_dependency(struct trace_state* state, pid_t thread_id, struct file_reference file, enum dependency_type type) {
    fprintf(stderr, "[%d] Dep: ", thread_id);
    switch (type) {
    case DEP_READ:
        fprintf(stderr, "read");
        break;
    case DEP_MODIFY:
        fprintf(stderr, "modify");
        break;
    case DEP_CREATE:
        fprintf(stderr, "create");
        break;
    case DEP_REMOVE:
        fprintf(stderr, "remove");
        break;
    }
    if (!file.follow_links) {
        fprintf(stderr, " (nofollow)");
    }
    if (file.path == NULL) {
        fprintf(stderr, " FD %d\n", file.fd);
    } else if (file.fd == AT_FDCWD) {
        fprintf(stderr, " %s\n", file.path);
    } else {
        fprintf(stderr, " {%d/}%s\n", file.fd, file.path);
    }
}

void trace_add_change_cwd(struct trace_state* state, pid_t thread_id, struct file_reference file) {
    fprintf(stderr, "[%d] Change working directory to ", thread_id);
    if (file.fd == AT_FDCWD) {
        fprintf(stderr, "%s\n", file.path);
    } else {
        fprintf(stderr, "{%d/}%s\n", file.fd, file.path);
    }
}

void trace_add_change_root(struct trace_state* state, pid_t thread_id, struct file_reference file) {
    fprintf(stderr, "[%d] Change root to ", thread_id);
    if (file.fd == AT_FDCWD) {
        fprintf(stderr, "%s\n", file.path);
    } else {
        fprintf(stderr, "{%d/}%s\n", file.fd, file.path);
    }
}

void trace_add_open(struct trace_state* state, pid_t thread_id, int fd, struct file_reference file, int access_mode, bool is_rewrite) {
    fprintf(stderr, "[%d] Open %d -> ", thread_id, fd);
    if (file.fd == AT_FDCWD) {
        fprintf(stderr, "%s\n", file.path);
    } else {
        fprintf(stderr, "{%d/}%s\n", file.fd, file.path);
    }
}

void trace_add_pipe(struct trace_state* state, pid_t thread_id, int fds[2]) {
    fprintf(stderr, "[%d] Pipe %d, %d\n", thread_id, fds[0], fds[1]);
}

void trace_add_dup(struct trace_state* state, pid_t thread_id, int duped_fd, int new_fd) {
    fprintf(stderr, "[%d] Dup %d <- %d\n", thread_id, duped_fd, new_fd);
}

void trace_add_mmap(struct trace_state* state, pid_t thread_id, int fd) {
    // TODO: look up the permissions that the file was opened with
    fprintf(stderr, "[%d] Mmap %d\n", thread_id, fd);
}

// TODO: Do we need this?
void trace_add_close(struct trace_state* state, pid_t thread_id, int fd) {
    fprintf(stderr, "[%d] Close %d\n", thread_id, fd);
}

void trace_add_fork(struct trace_state* state, pid_t parent_thread_id, pid_t child_process_id) {
    fprintf(stderr, "[%d] Fork %d\n", parent_thread_id, child_process_id);
}

void trace_add_exec(struct trace_state* state, pid_t process_id, char* exe_path) {
    fprintf(stderr, "[%d] Inside exec: %s\n", process_id, exe_path);
    free(exe_path);
}

void trace_add_exec_argument(struct trace_state* state, pid_t process_id, char* argument, int index) {
    fprintf(stderr, "[%d]     Arg %d: %s\n", process_id, index, argument);
    free(argument);
}

void trace_add_exit(struct trace_state* state, pid_t thread_id) {
    fprintf(stderr, "[%d] Exit\n", thread_id);
}

void trace_complete(struct trace_state* state) {
    free(state);
}
