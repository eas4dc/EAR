/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/system/folder.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

    folder_t folder;
    char buffer[512];
    char *file;
    state_t s;

    strcpy(buffer, argv[1]);
    // Initilizing folder scanning
    s = folder_open(&folder, buffer);

    if (state_fail(s)) {
        printf("Error opening folder %s\n", buffer);
    }

    while ((file = folder_getnext_type(&folder, "", "", DT_DIR))) {
        printf("Sub-folder/File detected %s\n", file);
    }

    printf("Done\n");
}
