/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <common/config.h>
#include <common/output/debug.h>
#include <common/output/error.h>
#include <common/output/verbose.h>
#include <common/system/file.h>
#include <common/system/popen.h>
#include <common/types/generic.h>
#include <common/utils/sched_support.h>

// if enabled, we will support use cases such as
// srun -n X app (where app is not mpi)
// app is supossed to work with 1 master as in mpi
#define APP_MULTIPROCESS_NO_MPI_SUPPORT 1

uint is_job_step_valid(job_id jid, job_id sid)
{
#if SCHED_SLURM
    char command[1024];
    popen_t sched_test;
    snprintf(command, sizeof(command), "/usr/bin/sacct -j %lu.%lu -o State", jid, sid);

    uint no_header = 2;
    // 'one_shot' creates a non-blocking pipe, so we then need
    // to nest to while loops. We could test by setting one_shot
    // to 1, so the popen module itself will block.
    uint one_shot = 0;
    char sep      = ' ';
    char *job_sched_state;

    debug("Executing: %s\n", command);

    pid_t child_pid = fork();
    if (child_pid == 0) {
        struct sigaction sa;
        // Enable SIGALARM
        sigset_t m;
        sigemptyset(&m);
        sigaddset(&m, SIGALRM);
        sigprocmask(SIG_UNBLOCK, &m, NULL);
        // Set default
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGALRM, &sa, NULL);
        // Set time limit
        alarm(DEF_TIMEOUT_COMMANDS);

        if (popen_open(command, no_header, one_shot, &sched_test) != EAR_SUCCESS) {
            debug("popen for scheduler test failedfailed\n");
            return 1;
        }
        uint expected_items = 1;
        uint i              = 0;
        while (i < expected_items) {
            while (popen_read2(&sched_test, sep, "s", &job_sched_state)) {
                debug("Detected state '%s'\n", job_sched_state);
                i++;
            }
        }

        popen_close(&sched_test);

        if (strcmp(job_sched_state, "RUNNING") == 0)
            exit(1);
        if (strcmp(job_sched_state, "COMPLETED") == 0)
            exit(0);
        if (strcmp(job_sched_state, "CANCELLED") == 0)
            exit(0);
        if (strcmp(job_sched_state, "FAILED") == 0)
            exit(0);
        if (strcmp(job_sched_state, "EXITED") == 0)
            exit(0);
        if (strcmp(job_sched_state, "TIMEOUT") == 0)
            exit(0);
    }
    // Parent checks exit status
    if (child_pid > 0) {
        int estatus;
        waitpid(child_pid, &estatus, 0);
        if (WIFEXITED(estatus))
            return (WEXITSTATUS(estatus));
        else
            return 1;
    } else {
        // If we cannot create processes we have lot of problems.
        return 1;
    }
#endif
    /* Should we check more cases ? */
    return 1;
}

/* From SLURM srun doc
 * SLURM_STEP_TASKS_PER_NODE Number of processes per node within the step.
 * SLURM_PROCID The MPI rank (or relative process ID) of the current process.
 * SLURM_LOCALID Node local task ID for the process within a job.
 */

int get_my_sched_node_rank()
{
#if APP_MULTIPROCESS_NO_MPI_SUPPORT
#if SCHED_SLURM
    if (ear_getenv("SLURM_LOCALID") != NULL)
        return atoi(ear_getenv("SLURM_LOCALID"));
#endif
#if SCHED_PBS
    // WARNING: PBS_TASKNUM is the total number of tasks, it is not parallel to SLURM_LOCALID
    // There is currently no way to get something like SLURM_LOCALID.
    // Falling into the default case for now.
    // if (ear_getenv("PBS_TASKNUM") != NULL) return atoi(ear_getenv("PBS_TASKNUM"));
#endif
#endif
    return 0;
}

int get_my_sched_global_rank()
{
#if SCHED_SLURM
#if APP_MULTIPROCESS_NO_MPI_SUPPORT
    if (ear_getenv("SLURM_PROCID") != NULL)
        return atoi(ear_getenv("SLURM_PROCID"));
#endif
#endif
    return 0;
}

int get_my_sched_group_node_size()
{
#if SCHED_SLURM
#if APP_MULTIPROCESS_NO_MPI_SUPPORT
    if (ear_getenv("SLURM_STEP_TASKS_PER_NODE") != NULL)
        return atoi(ear_getenv("SLURM_STEP_TASKS_PER_NODE"));
#endif
#endif
    return 1;
}

void sched_local_barrier(char *file, int N)
{
    debug("EARL[%d] waiting in %s for %d processes", getpid(), file, N);
    char lock_file[1024];
    int fd, fd_lock;

    snprintf(lock_file, sizeof(lock_file), "%s.lock", file); // Implements the exclusive acess
    // Create the file and accum 1 element
    while ((fd_lock = ear_file_lock_master(lock_file)) < 0)
        ; // lock
    fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    // Update
    if (fd >= 0) {
        int curr = 0, leido;
        leido    = read(fd, &curr, sizeof(int));
        if (leido == sizeof(int)) { // Already have data
            curr++;
            lseek(fd, 0, SEEK_SET);
        } else { // First process
            curr = 1;
        }
        write(fd, &curr, sizeof(int));
        close(fd);
    } else {
        error("sched_local_barrier timeout");
    }
    ear_file_unlock_master(fd_lock, lock_file);

    debug("Waiting for processes");
    // Wait until the value is equal to N
    int out_tries = 0;
    int max_tries = 1000000;
    int value     = -1;
    do { // Out
        fd = open(file, O_RDONLY);
        // Read
        if (fd >= 0) {
            read(fd, &value, sizeof(int));
            close(fd);
        } else {
            error("That should not happen! (%s)", strerror(errno));
        }
        // Compare
        if (value == N)
            return;
        out_tries++;
        usleep(100000);
    } while (out_tries < max_tries);
    error("sched_local_barrier timeout");
    return;
}

void remove_local_barrier(char *file)
{
    unlink(file);
}

/* Functions to parse and to process SLURM-like nodelists */

/*
 * This function returns an array of (potentially) linked lists.
 * Each
 *
 * This uses a simple state machine to parse the node information.
 * the states are:
 *   in_range (explicit)  -> parsing inside a []. ex: the "45,46" in cmp25[45,46]
 *   out_range (implicit) -> parsing outside a [] ex: the "cmp25" in cmp25[45,46]
 * encountering a '[' will change the state from out_range to in_range.
 * encountering a ']' will change the state from in_range to out_range.
 *
 * encountering a '[' while in_range or a ']' while out_range is not supported and the character will be ignored.
 * encountering a ',' while out_range create a new range in the list, resetting the state and setting the previous
 * next_is_new to 1;
 *
 *
 *   in_range has a second state:
 *     filling_first (implicit) -> parsing the "45" in cmp25[45-48] or parsing the "45" and "48" in cmp25[45,48]
 *     filling_second (explicit) -> parsing the "48" in cmp25[45-48] or parsing NOTHING in cmp25[45,48]
 *   encountering a '-' while in_range will change the state from filling_first to filling_second.
 *   encountering a ',' while in_range will create a new pair, and reset the state to filling_first
 *   encountering a ']' while in_range will move the current_range forward, resetting the state and setting the previous
 *
 */
int32_t parse_range_list(const char *source, range_def_t **range_list)
{
    // if there's nothing on the string, return 0 total parsed
    if (source == NULL || source[0] == '\0')
        return 0;

    int32_t num_parsed         = 1;
    range_def_t *first_range   = calloc(num_parsed, sizeof(range_def_t));
    range_def_t *current_range = first_range;

    int32_t offset           = 0;
    bool in_range            = false;
    bool filling_second      = false;
    char first_num_buff[16]  = {0};
    char second_num_buff[16] = {0};
    size_t buff_offset       = 0;

    while (*source) {
        /* Inside the [] brackets (the "45-48" in cmp25[45-48])*/
        if (in_range) {
            if (*source == ',') {
                // bookkeeping for 0s to the left
                for (int32_t i = 0; i < buff_offset; i++) {
                    if ((first_num_buff[i] == second_num_buff[i]) && (first_num_buff[i] == '0'))
                        current_range->numbers[current_range->numbers_count - 1].leading_zeroes++;
                    else
                        break;
                }

                // reset state control variables
                buff_offset    = 0;
                filling_second = false;
                memset(first_num_buff, 0, sizeof(first_num_buff));
                memset(second_num_buff, 0, sizeof(second_num_buff));

                current_range->numbers =
                    realloc(current_range->numbers, sizeof(pair) * (current_range->numbers_count + 1));
                memset(&current_range->numbers[current_range->numbers_count], 0, sizeof(pair));
                current_range->numbers_count++;

                // go to next char
                goto next;
            }

            // this is only here for safety
            if (*source == '[') {
                // go to next char
                goto next;
            }

            if (*source == ']') {
                // bookkeeping for 0s to the left
                for (int32_t i = 0; i < buff_offset; i++) {
                    if ((first_num_buff[i] == second_num_buff[i]) && (first_num_buff[i] == '0'))
                        current_range->numbers[current_range->numbers_count - 1].leading_zeroes++;
                    else
                        break;
                }

                // reset state control variables
                in_range       = false;
                filling_second = false;
                buff_offset    = 0;
                offset         = 0;
                memset(first_num_buff, 0, sizeof(first_num_buff));
                memset(second_num_buff, 0, sizeof(second_num_buff));

                /* only allocate if there's something AFTER the range ex:
                 * cmp25[45,46]test
                 * cmp25[45,46]rack[25]
                 * The following case is NOT contemplated here:
                 *    cmp25[45],cmp2547
                 * This will be taken care of when we parse the ',' in the next iteration
                 */
                if (*(source + 1)) {
                    if (*(source + 1) != ',') {
                        range_def_t *next_range = calloc(1, sizeof(range_def_t));
                        current_range->next     = next_range;
                        current_range           = next_range;
                    }
                }

                // go to next char
                goto next;
            }

            if (*source == '-') {
                filling_second = true;
                buff_offset    = 0;

                // go to next char
                goto next;
            }

            /* General case, we are reading numbers from left to right.
             * Simply multiply by 10 what we currently have and add the new value */

            // auxiliary value to shorten it. We are filling the pair of the last allocated "numbers" in the
            // current_range. if the first has been already filled (marked by the '-' char) then we use the second.
            int32_t *to_fill = filling_second ? &current_range->numbers[current_range->numbers_count - 1].second
                                              : &current_range->numbers[current_range->numbers_count - 1].first;

            *to_fill *= 10;
            *to_fill += *source - '0';

            // For bookkeeping 0s on the left
            if (!filling_second)
                first_num_buff[buff_offset] = *source;
            else
                second_num_buff[buff_offset] = *source;

            buff_offset++;

            // go to next char
            goto next;
        }

        /* Outside the [] brackets (the "cmp25" in cmp25[45-48])*/

        if (*source == '[') {
            in_range    = true; // mark that we are within [] bounds
            buff_offset = 0;

            current_range->numbers       = calloc(1, sizeof(pair));
            current_range->numbers_count = 1;

            // go to next char
            goto next;
        }

        // this is only here for safety
        if (*source == ']') {
            // go to next char
            goto next;
        }

        if (*source == ',') {
            // bookkeeping for 0s to the left
            for (int32_t i = 0; i < buff_offset; i++) {
                if ((first_num_buff[i] == second_num_buff[i]) && (first_num_buff[i] == '0'))
                    current_range->numbers[current_range->numbers_count - 1].leading_zeroes++;
                else
                    break;
            }
            // reset state control variables
            in_range       = false;
            filling_second = false;
            offset         = 0;
            buff_offset    = 0;
            memset(first_num_buff, 0, sizeof(first_num_buff));
            memset(second_num_buff, 0, sizeof(second_num_buff));

            // allocation of next one
            first_range   = realloc(first_range, sizeof(range_def_t) * (num_parsed + 1));
            current_range = &first_range[num_parsed];
            memset(current_range, 0, sizeof(range_def_t));
            num_parsed++;

            // go to next char
            goto next;
        }

        current_range->prefix[offset] = *source;
        offset++;

    next:
        source++;
    }

    *range_list = first_range;
    return num_parsed;
}

/*
 * -base: the previously-processed ranges. ex: island[0-9]rack[0-9]node[1-10] -> when processing the range containing
 *      "node", base will be "islandNrackM".
 * -cr: the current range being processed
 * -max_size: maximum buffer size
 * -current_size: an updating counter for the current string length stored in buffer. This allows us to not have to
 *      repeatedly call strlen which gets increasingly slower as the string grows
 * -buffer: where the final string is stored
 *
 *  Returns 0 if successful, if it fails it returns the number of nodes that could not be fit into buffer.
 */
int32_t concat_range(char *base, range_def_t *cr, size_t max_size, size_t *current_size, char buffer[max_size])
{

    int32_t cut = 0; // the amount of nodes cut from the final list due to space
    if (cr == NULL) {
        // printf("%s\n", base);
        if ((*current_size + strlen(base)) >= max_size)
            return 1; // we cannot fit the new node, so we cut it
        if (*current_size) {
            strcat(buffer, ",");
            *current_size += 1;
        }
        strcat(buffer, base);
        *current_size += strlen(base);
        return 0; // the node was successfully copied
    }
    char buff[1024]        = {0};
    char second_buff[1024] = {0};
    if (base != NULL)
        strcpy(buff, base);
    if (cr->numbers_count == 0) { // special case
        if ((*current_size + strlen(base)) >= max_size)
            return 1; // we cannot fit the new node, so we cut it
        if (*current_size) {
            strcat(buffer, ",");
            *current_size += 1;
        }
        strcat(buffer, cr->prefix);
        *current_size += strlen(cr->prefix);
        return 0; // the node was successfully copied
    }
    for (int32_t k = 0; k < cr->numbers_count; k++) {
        memset(second_buff, 0, sizeof(second_buff));
        memset(buff, 0, sizeof(buff));
        if (base != NULL)
            strcpy(buff, base);
        pair *aux = &cr->numbers[k];
        /*   1)0-0 if NO numbers had to be set (ideally shouldn't happen and the pointer would be null with no range).
         * cmp2546[] (useless []) 2)0-N or N-M if BOTH numbers have been set. ex: cmp25[45-47] 3)N-0 if only ONE number
         * has been set. ex: cmp25[45]
         */
        if (aux->first == 0 && aux->second == 0) {
            sprintf(second_buff, "%s", cr->prefix);
            strcat(buff, second_buff);
            cut += concat_range(buff, cr->next, max_size, current_size, buffer);
        } else if (aux->second == 0) {
            sprintf(second_buff, "%s", cr->prefix);
            for (int32_t i = 0; i < aux->leading_zeroes; i++)
                strcat(second_buff, "0");
            sprintf(&second_buff[strlen(second_buff)], "%d", aux->first);
            strcat(buff, second_buff);
            cut += concat_range(buff, cr->next, max_size, current_size, buffer);
        } else {
            for (int32_t i = 0; i <= (aux->second - aux->first); i++) {
                memset(second_buff, 0, sizeof(second_buff));
                memset(buff, 0, sizeof(second_buff));
                if (base != NULL)
                    strcpy(buff, base);

                sprintf(second_buff, "%s", cr->prefix);
                int32_t zeroes = aux->leading_zeroes + difference_in_units(aux->first + i, aux->second);
                for (int32_t j = 0; j < zeroes; j++)
                    strcat(second_buff, "0");
                sprintf(&second_buff[strlen(second_buff)], "%d", aux->first + i);
                strcat(buff, second_buff);
                cut += concat_range(buff, cr->next, max_size, current_size, buffer);
            }
        }
    }
    return cut;
}

void free_range_list(int32_t num_ranges, range_def_t list[num_ranges])
{
    for (int32_t i = 0; i < num_ranges; i++) {
        range_def_t *cr   = &list[i];
        range_def_t *prev = NULL;
        while (cr->next) {
            prev = cr;
            cr   = cr->next;
        }
        if (cr->numbers_count > 0) {
            free(cr->numbers);
        }
        if (prev) { // if there's a previous, it's not part of the top-most array
            free(cr);
            prev->next = NULL;
            i--; // we go back to the head of the linked list (the current one in the array)
            continue;
        }
    }
    free(list);
}

int32_t expand_list(const char *source, size_t target_length, char target[target_length])
{
    range_def_t *list  = NULL;
    int32_t num_ranges = parse_range_list(source, &list);

    int32_t cut_nodes = 0;
    size_t final_size = 0;
    for (int32_t i = 0; i < num_ranges; i++) {
        cut_nodes += concat_range("", &list[i], target_length, &final_size, target);
    }

    free_range_list(num_ranges, list);

    return cut_nodes;
}

int32_t expand_list_alloc(const char *source, char **target)
{
    range_def_t *list  = NULL;
    int32_t num_ranges = parse_range_list(source, &list);

    size_t buff_length = 1024;

    int32_t cut_nodes = 0;
    size_t alloc_size = 1; // start at 1 for the emtpy space at the end
    size_t prev_alloc = 0;

    char *buff         = calloc(buff_length, sizeof(char));
    char *final_buffer = NULL;

    for (int32_t i = 0; i < num_ranges; i++) {
        size_t current_size = 0;
        memset(buff, 0, buff_length);
        cut_nodes = concat_range("", &list[i], buff_length, &current_size, buff);
        // if the buffer cannot fit the new ranges, increase it
        if (cut_nodes) {
            buff_length *= 2;
            buff = realloc(buff, sizeof(char) * buff_length);
            i--; // re-do the previous one
            continue;
        }

        bool add_comma = false;

        if (alloc_size > 1) {
            add_comma = true;
            alloc_size++; // new comma
        }
        alloc_size += current_size;
        final_buffer = realloc(final_buffer, sizeof(char) * alloc_size);
        memset(&final_buffer[prev_alloc], 0, sizeof(char) * alloc_size - prev_alloc);
        prev_alloc = alloc_size;
        if (add_comma)
            strcat(final_buffer, ",");
        strcat(final_buffer, buff);
    }

    debug("Final list %s\n", final_buffer);
    *target = final_buffer;

    free(buff);

    free_range_list(num_ranges, list);

    return 0;
}
#if 0

// These functions were used by EARD , not longer used
static int is_running(char *status_text)
{
  if (strlen(status_text) == 0) return 0;
  if (strstr(status_text, "COMPLETED") != NULL) return 0;
  if (strstr(status_text, "FAILED")    != NULL) return 0;
  if (strstr(status_text, "EXITED")    != NULL) return 0;
  if (strstr(status_text, "TIMEOUT")   != NULL) return 0;
  if (strstr(status_text, "CANCELLED") != NULL) return 0;
  return 1;
}


static int read_word(int fd, char * buff, int limit)
{
  int t = 0, r = 0;
  while(((r = read(fd, &buff[t], (limit-t))) > 0) && (t < limit)){
    t += r;
  }
  return t;
}

static int is_job_running(char *tmp, int jobid)
{
  /* sacct -j 1512221.4294967291 -o state -n*/
  char path_file[GENERIC_NAME], pid_str[128], status[128];
  int fd, size;

  sprintf(path_file,"%s/jobid_status_file", tmp);
  fd = open(path_file, O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR);
  pid_t status_pid = fork();
  if (status_pid == 0 ){
    dup2(fd,1);
    sprintf(pid_str,"%d.batch", jobid);
    debug("job id: %d", jobid);
    if (execlp("sacct", "sacct", "-j", pid_str,"-o","state", "-n", NULL) < 0) error( "Error execlp (%s)", strerror(errno)); // perror ("error execlp");
    exit(1);
  }
  if (waitpid(status_pid,  NULL, 0) < 0 ) error("Error waitpid (%s)", strerror(errno));
  if (lseek(fd, 0 , SEEK_SET) < 0 ) error("Error lseek (%s)", strerror(errno));
  size = read_word(fd, status, 128);
  status[size] = '\0';
  return is_running(status);
}

#endif
