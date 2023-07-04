/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef METRICS_COMMON_REDFISH_H
#define METRICS_COMMON_REDFISH_H

#include <common/types/generic.h>

// How to use Redfish API:
//
// 1) Call redfish_open() passing the host, i.e. https://169.254.95.118. The /redfish/v1 is
//    not included. Also add the user and password if required (or NULL in both arguments).
// 2) Use redfish_get() to read the content of the field. Field formats examples:
//    - /Chassis: the tipical field, it returns a JSON object. A JSON object just can
//                be read by the REDTYPE_JSON.
//    - /Chassis[0]/Power/PowerControl[0]/PowerConsumedWatts: here we are accessing to
//                the index 0 of an array of objects. The previous Chassis JSON is in
//                fact an array of other objects and we are accessing to the first.
//                Later, in PowerControl field we are doing the same. Finally we are
//                accessing to PowerConsumedWatts which is an int value.
//    - /Chassis[0]/Power/PowerControl[MemberId=1]/PowerConsumedWatts: here we are
//                doing the same, but selecting the item in the PowerControl array whose
//                MemberId is 1. You can use this method to select specific indexes by
//                the Name, Id, Description, etc.
//    - /Chassis[%d]/Power/PowerControl[MemberId=%d]/PowerConsumedWatts: also you can
//                use the traditional %d print format to iterate over multiple JSON 
//                array items.
//
//    Accesing to multiple array items through %d could be useful to accumulate their
//    different values. In the redfish_get() function, you can pass an 'accum' variable
//    to return the accumulated number. But remember to match the type_flag with the
//    'accum' variable type. Finally the 'content' could be a single variable or an
//    array, both have to match with the type_flag type. If you are using the '%d' to
//    get multiple items you have to pass an array as 'content'. There is no way to
//    know in advance the number of items to return so provide a wide vector.
//
// Example (is only an example, your Redfish fields can be different):
//      if (state_fail(redfish_open("localhost", "user123", "password123"))) {
//          return 0;
//      }
//      if (state_fail(redfish_read("/Chassis[0]/MaxPowerWatts", (void *) &int_var, NULL, &count, REDTYPE_INT))) {
//          return 0;
//      }
//      if (state_fail(redfish_read("/Chassis[%d]/MaxPowerWatts", (void *) int_array, &int_var, &count, REDTYPE_INT))) {
//          return 0;
//      }
//

// Type flags:
#define REDTYPE_JSON   0 // (char *)
#define REDTYPE_STRING 1 // (char *)
#define REDTYPE_INT    2 // (int)
#define REDTYPE_LLONG  3 // (llong)
#define REDTYPE_DOUBLE 4 // (double)

/* Given a host, a username and password initializes redfish data. */
state_t redfish_open(char *host, char *user, char *pass);

/* accum : When asking for arrays, accum is set to the aggregated value for the array member (out)
 * count : number of elements in the array (out)
 */
state_t redfish_read(char *field, void *content, void *accum, uint *count, int type_flag);

/* Count the array items of a field when the returned field is a JSON array. */
state_t redfish_count_members(char *field, uint *count);

#endif //METRICS_COMMON_REDFISH_H
