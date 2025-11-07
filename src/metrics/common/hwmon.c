/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <metrics/common/file.h>
#include <metrics/common/hwmon.h>

static int file_read(char *path, int *fd, int close_on_read, char *buffer, size_t buffer_length)
{
    int i = 0;
    int r = 0;
    // No file descriptor and no path
    if (*fd < 0 && path[0] == 'f' && path[1] == 'd') {
        return 0;
    }
    if (*fd < 0) {
        if ((*fd = open(path, O_RDONLY)) < 0) {
            debug("Opening '%s' failed: %s", path, strerror(errno));
            return 0;
        }
    }
    lseek(*fd, 0, SEEK_SET);
    while (r < buffer_length) {
        if ((r = read(*fd, &buffer[r], buffer_length)) <= 0) {
            break;
        }
        buffer_length -= r;
        i += r;
    }
    buffer[i] = '\0';
    if (buffer[strlen(buffer) - 1] == '\n') {
        buffer[strlen(buffer) - 1] = '\0';
    }
    debug("HWMON read from '%s': %s (fd%d)", path, buffer, *fd);
    if (close_on_read) {
        close(*fd);
        *fd = -2; // We are using the -1 to count unopened items
    }
    return 1;
}

static int read_device_item(char *folder_path, char *type_number, char *item_name, ullong address, int close)
{
    char path[SZ_PATH];
    int *item_toint;
    int *item_fd;
    char *item;

    item_fd    = (int  *) (address);
    item       = (char *) (address + sizeof(int));
    item_toint = (int  *) (address + sizeof(int) + 32);
    // Getting full item path
    if (folder_path != NULL) {
        sprintf(path, "%s/%s_%s", folder_path, type_number, item_name);
        debug("Opening device_item %s", path);
        if (!filemagic_exists(path)) {
            sprintf(path, "%s/device/%s_%s", folder_path, type_number, item_name);
            debug("Opening device_item %s", path);
            if (!filemagic_exists(path)) {
                return 0;
            }
        }
    } else {
        sprintf(path, "fd %d", *item_fd);
    }
    // Reading file
    if (!file_read(path, item_fd, close, item, 32)) {
        return 0;
    }
    *item_toint = atoi(item);
    return 1;
}

static void open_devices(char *folder_path, char *devs_name, char *label, hwmon_dev_t **devs, uint *devs_count)
{
    char type_number[SZ_PATH];
    int completed = 0;
    int missing   = 0;
    int labeled   = 0;
    int d         = 0;
    int i         = 0;

    *devs       = NULL;
    *devs_count = 0;
    *devs       = realloc(*devs, sizeof(hwmon_dev_t));
    memset(*devs, 0, sizeof(hwmon_dev_t));
    do {
        completed = 0;
        sprintf(type_number, "%s%d", devs_name, d + 1);
        // Cleaning
        memset(&(*devs)[i], 0, sizeof(hwmon_dev_t));
        sprintf((*devs)[i].label, "no-label");
        (*devs)[i].number              = d + 1;
        (*devs)[i].input_fd            = -1;
        (*devs)[i].label_fd            = -1;
        (*devs)[i].max_fd              = -1;
        (*devs)[i].min_fd              = -1;
        (*devs)[i].average_fd          = -1;
        (*devs)[i].average_interval_fd = -1;
        //
        #define offset(var) (ullong) & ((*devs)[i].var)
        completed += read_device_item(folder_path, type_number, "input"   , offset(input_fd)  , 0);
        completed += read_device_item(folder_path, type_number, "oem_info", offset(label_fd)  , 1); // Secondary label
        completed += read_device_item(folder_path, type_number, "label"   , offset(label_fd)  , 1);
        completed += read_device_item(folder_path, type_number, "max"     , offset(max_fd)    , 1);
        completed += read_device_item(folder_path, type_number, "average" , offset(average_fd), 0);
        completed += read_device_item(folder_path, type_number, "average_interval", offset(average_interval_fd), 1);
        #undef offset
        // Label control
        labeled = (label == NULL) || (strcasestr((*devs)[i].label, label) != NULL);
        // If have read at least one thing
        if (completed) {
            if (labeled) {
                // Allocating more space for the next device and its items (+2 because it NULL terminated)
                *devs = realloc(*devs, sizeof(hwmon_dev_t) * ((*devs_count) + 2));
                memset(&(*devs)[*devs_count+1], 0, sizeof(hwmon_dev_t));
                (*devs_count)++;
                i++;
            }
            missing = 0;
        } else {
            // 4 device holes is too much
            if (++missing == 4) {
                break;
            }
        }
        d++;
    } while (1);
    // Null-terminated list of devices
    (*devs)[i].is_null = 1;
}

static int test_chip_folder(char *path)
{
    return filemagic_exists(path);
}

static int test_chip_name(char *chip_mem, char *folder_path, char *name_file)
{
    char path[SZ_PATH];
    char data[128];
    int fd = -1;

    sprintf(path, "%s/%s", folder_path, name_file);
    if (!file_read(path, &fd, 1, data, 128)) {
        return 0;
    }
    if (strcmp(chip_mem, data) != 0) {
        return -1;
    }
    return 1;
}

int test_chip_labels(char *folder_path, char *devs_name, char *label)
{
    struct aux_s {
        hwmon_add(aux);
    } aux = {0};
    char type_number[SZ_PATH];
    int  completed = 0;
    int  missing = 0;
    int  d = -1;

    if (label == NULL) {
        return 1;
    }
    do {
        ++d;
        aux.aux_fd = -1;
        sprintf(type_number, "%s%d", devs_name, d + 1);
        if (!(completed = read_device_item(folder_path, type_number, "label", (ullong) &aux.aux_fd, 1))) {
            if (!(completed = read_device_item(folder_path, type_number, "oem_info", (ullong) &aux.aux_fd, 1))) {
                if (++missing == 4) {
                    break;
                }
                continue;
            }
        }
        missing = 0;
        if (strcasestr(aux.aux, label)) {
            return 1;
        }
    } while(1);
    return 0;
}

state_t hwmon_open(char *chip_name, char *dev_type, char *label, hwmon_t *chips[], uint *chips_count)
{
    char folder_path[SZ_PATH];
    int i = 0;
    int test;

    *chips       = NULL;
    *chips_count = 0;
    do {
        sprintf(folder_path, "/sys/class/hwmon/hwmon%d", i);
        // debug("HWMON testing chip %s", folder_path);
        //  If folder does not exists, there are no more hwmon folders
        if (!test_chip_folder(folder_path)) {
            break;
        }
        // If file name matches
        if ((test = test_chip_name(chip_name, folder_path, "name"       )) != 0 ||
            (test = test_chip_name(chip_name, folder_path, "device/name")) != 0 ||
            (test = test_chip_name(chip_name, folder_path, "device/hid" )) != 0) { // Rare case
            // The is a chip but it has different name
            if (test == -1) {
                goto next;
            }
            // Testing if at least one device label is present in this chip
            if (!test_chip_labels(folder_path, dev_type, label)) {
                debug("HWMON chip does not contain any device type with label %s", label);
                goto next;
            }
            debug("HWMON new chip found! We found %d already", (*chips_count) + 1);
            // Allocating more space for the devices of the newly found chip (+2 because it NULL terminated)
            *chips = realloc(*chips, sizeof(hwmon_t) * ((*chips_count) + 2));
            memset(&(*chips)[*chips_count], 0, sizeof(hwmon_t) * 2);
            // Opening the devices of this chip
            open_devices(folder_path, dev_type, label, &(*chips)[*chips_count].devs, &(*chips)[*chips_count].devs_count);
            // Increasing the number of devices found
            (*chips_count)++;
        }
    next:
        ++i;
    } while (1);
    if (*chips_count == 0) {
        return_print(EAR_ERROR, "HWMON failed to find %s chip name", chip_name);
    }
    // Null-terminating
    (*chips)[*chips_count].is_null = 1;
    // Extreme debugging
    #if 1
    int j;
    debug("List of opened files:")
    for (i = 0; i < *chips_count+1; ++i)
    {
        debug(" - CHIP%d '%s' (is_null %d)", i, chip_name, (*chips)[i].is_null);
        for (j = 0; j < (*chips)[i].devs_count+1 && !(*chips)[i].is_null; ++j) {
            debug("   - DEV%03d fd%2d %s%d_ (%s) (is_null %d)", j, (*chips)[i].devs[j].input_fd, dev_type,
                  (*chips)[i].devs[j].number, (*chips)[i].devs[j].label, (*chips)[i].devs[j].is_null);
        }
    }
    #endif
    return EAR_SUCCESS;
}

void hwmon_close(hwmon_t **chips)
{
    int c = 0; // c of chip
    if ( chips == NULL) { return; }
    if (*chips == NULL) { return; }
    hwmon_close_labels(*chips, NULL);
    while ((*chips)[c].devs != NULL && !(*chips)[c].is_null) {
        free((*chips)[c].devs);
        ++c;
    }
    free(*chips);
    *chips = NULL;
}

int hwmon_count_items(hwmon_t chips[], char *item_name, char *label)
{
    int counter = 0;
    int c       = 0; // c of chip
    int d       = 0; // d of device

    int cmp1 = strncmp(item_name, "input"           ,  5) == 0;
    int cmp2 = strncmp(item_name, "min"             ,  3) == 0;
    int cmp3 = strncmp(item_name, "max"             ,  3) == 0;
    int cmp4 = strncmp(item_name, "average"         ,  7) == 0;
    int cmp5 = strncmp(item_name, "average_interval", 16) == 0;

    if (chips == NULL) {
        return 0;
    }
    while (!chips[c].is_null) {
        d = 0;
        while (!chips[c].devs[d].is_null) {
            if (label == NULL || strcasestr(chips[c].devs[d].label, label) != NULL) {
                if (cmp1) counter += chips[c].devs[d].input_fd   != -1;
                if (cmp2) counter += chips[c].devs[d].min_fd     != -1;
                if (cmp3) counter += chips[c].devs[d].max_fd     != -1;
                if (cmp4) counter += chips[c].devs[d].average_fd != -1;
                if (cmp5) counter += chips[c].devs[d].average_interval_fd != -1;
            }
            ++d;
        }
        ++c;
    }
    debug("counted %d items '%s'", counter, label);
    return counter;
}

void hwmon_read(hwmon_t *chips)
{
    int c = 0; // c of chip
    int d = 0; // d of device
    while (!chips[c].is_null) {
        d = 0;
        while (!chips[c].devs[d].is_null) {
            #define offset(var) (ullong) & chips[c].devs[d].var
            read_device_item(NULL, NULL, NULL, offset(input_fd), 0);
            read_device_item(NULL, NULL, NULL, offset(average_fd), 0);
            ++d;
        }
        ++c;
    }
}

void hwmon_calc_average(hwmon_t *chips, char *label)
{
    int c = 0; // c of chip
    int d = 0; // d of device
    int m = 0; // m of matches

    debug("List of averages:");
    while (!chips[c].is_null) {
        d = 0;
        m = 0;
        // Cleaning
        memset(&chips[c].devs_avg, 0, sizeof(hwmon_dev_t));
        while (!chips[c].devs[d].is_null) {
            debug("- CHIP%d DEV%d comparing labels '%s' == '%s'", c, d, chips[c].devs[d].label, label);
            if (label == NULL || strcasestr(chips[c].devs[d].label, label) != NULL) {
                debug("   - input  : acc + %u = %u", chips[c].devs_avg.input_toint  , chips[c].devs[d].input_toint  );
                debug("   - average: acc + %u = %u", chips[c].devs_avg.average_toint, chips[c].devs[d].average_toint);
                chips[c].devs_avg.input_toint   += chips[c].devs[d].input_toint;
                chips[c].devs_avg.max_toint     += chips[c].devs[d].max_toint;
                chips[c].devs_avg.min_toint     += chips[c].devs[d].min_toint;
                chips[c].devs_avg.average_toint += chips[c].devs[d].average_toint;
                chips[c].devs_avg.average_interval_toint += chips[c].devs[d].average_interval_toint;
                ++m;
            }
            ++d;
        }
        debug("   - input  : %5u / %u = ??", chips[c].devs_avg.input_toint  , m);
        debug("   - average: %5u / %u = ??", chips[c].devs_avg.average_toint, m);
        chips[c].devs_avg.input_toint   /= m;
        chips[c].devs_avg.max_toint     /= m;
        chips[c].devs_avg.min_toint     /= m;
        chips[c].devs_avg.average_toint /= m;
        chips[c].devs_avg.average_interval_toint /= m;
        debug("   - input  : ?? = %u", chips[c].devs_avg.input_toint  );
        debug("   - average: ?? = %u", chips[c].devs_avg.average_toint);
        ++c;
    }
}

static void set_univisited(hwmon_t chips[], hwmon_dev_t devs[])
{
    int e = 0; // of not c nor d
    while (chips != NULL && !chips[e].is_null) {
        chips[e].is_visited = 0;
        ++e;
    }
    while (devs != NULL && !devs[e].is_null) {
        devs[e].is_visited = 0;
        ++e;
    }
}

hwmon_t *hwmon_iter_chips(hwmon_t chips[])
{
    int c = 0; // c of chip
    while (1) {
        if (chips[c].is_null) {
            set_univisited(chips, NULL);
            return NULL;
        }
        if (!chips[c].is_visited) {
            chips[c].is_visited = 1;
            return &chips[c];
        }
        ++c;
    }
    return NULL;
}

hwmon_dev_t *hwmon_iter_devs(hwmon_t *chip, char *label)
{
    int d = 0; // d of device
    while (1) {
        if (chip->devs[d].is_null) {
            set_univisited(NULL, chip->devs);
            return NULL;
        }
        if (!chip->devs[d].is_visited && (label == NULL || strcasestr(chip->devs[d].label, label))) {
            chip->devs[d].is_visited = 1;
            return &chip->devs[d];
        }
        ++d;
    }
    return NULL;
}

void hwmon_close_labels(hwmon_t chips[], char *label)
{
    int negative = 0;
    int match;
    int d = 0; // d of device
    int c = 0; // c of chip

    #define close_dev(dev_fd) \
    if (dev_fd >= 0) {        \
        debug("closing " #dev_fd " %d", dev_fd); \
        close(dev_fd);        \
        dev_fd = -1;          \
    }
    if (label != NULL) {
        negative = (label[0] == '!');
        if (negative) {
            label = &label[1];
        }
    }
    while (!chips[c].is_null) {
        while (!chips[c].devs[d].is_null) {
            match = (label == NULL) ? 1 : (strcasestr(chips[c].devs[d].label, label) != NULL);
            if ((!negative && match) || (negative && !match)) {
                debug("closing '%s' file descriptors", chips[c].devs[d].label);
                close_dev(chips[c].devs[d].input_fd);
                close_dev(chips[c].devs[d].label_fd);
                close_dev(chips[c].devs[d].max_fd);
                close_dev(chips[c].devs[d].min_fd);
                close_dev(chips[c].devs[d].average_fd);
                close_dev(chips[c].devs[d].average_interval_fd);
            }
            ++d;
        }
        ++c;
    }
}

#if TEST1
int main(int argc, char *argv[])
{
    uint chips_count = 0;
    uint cores_count = 0;
    hwmon_t *chips   = NULL;

     if (state_fail(hwmon_open("coretemp", "temp", &chips, &chips_count))) {
    //if (state_fail(hwmon_open("power_meter", "power", &chips, &chips_count))) {
        debug("Failed: %s", state_msg);
        return 0;
    }
    if (!(cores_count = hwmon_count_items(chips, "input", "Core"))) {
    //if (!(cores_count = hwmon_count_items(chips, "average", "Grace"))) {
        hwmon_close(&chips);
        return 0;
    }
    printf("Detected %d Core inputs\n", cores_count);
    //printf("Detected %d averages\n", cores_count);
    hwmon_read(chips);
     hwmon_calc_average(chips, "Core");
    //hwmon_calc_average(chips, "Grace");
    printf("The average Core temperature of Socket 0 is %dº\n", chips[0].devs_avg.input_toint / 1000);
    //printf("The average power is %d xW\n", chips[1].devs_avg.average_toint);
    // 1 by 1 iteration
    hwmon_dev_t *dev = NULL;
    hwmon_t *chip    = NULL;
    while ((chip = hwmon_iter_chips(chips)) != NULL) {
        while ((dev = hwmon_iter_devs(chip, NULL)) != NULL) {
            printf("temp%d_input %d (label %s)\n", dev->number, dev->input_toint, dev->label);
            //printf("power%d_average %d (label %s)\n", dev->number, dev->average_toint, dev->label);
        }
    }
    hwmon_close(&chips);
    return 0;
}
#endif
