/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_ENERGY_MODEL_H_
#define _EAR_ENERGY_MODEL_H_

#include <common/hardware/architecture.h>
#include <common/states.h>
#include <common/types/types.h>
#include <daemon/shared_configuration.h>

/** The handler of an energy model. It must be initialized by calling one of the load functions,
 * and must be deallocated by calling \ref energy_model_dispose . */
typedef struct energy_model_s *energy_model_t;

/** \brief Loads a CPU energy model.
 * \param[in] sconf Current EAR installation info.
 * \param[in] arch_desc
 *
 * \return A NULL pointer on error.
 */
energy_model_t energy_model_load_cpu_model(settings_conf_t *sconf, architecture_t *arch_desc);

/** \brief Loads a GPU energy model.
 * \param[in] sconf
 * \param[in] arch_desc
 *
 * \return A NULL pointer on error.
 */
energy_model_t energy_model_load_gpu_model(settings_conf_t *sconf, architecture_t *arch_desc);

/** Deallocates the memory used by the energy model handler given as input argument.
 * On error, it fills the global string \ref state_msg with the cause of the error.
 *
 * \return EAR_ERROR The input argument is null.
 * \return EAR_SUCCESS Otherwise. */
state_t energy_model_dispose(energy_model_t energy_model);

/** The starting point function. Used to initialize model internals, e.g., attributes, configuration.
 * Model's implementation dependant.
state_t energy_model_init(energy_model_t energy_model);
*/

/** Project the execution time of an application with the signature given as input argument.
 * Model's implementation dependant.
 * \param[in]  energy_model A loaded energy model.
 * \param[in]  signature    The input signature for the model containing metrics used to project time.
 * \param[in]  from_ps			The P-State index you want the model to project from.
 * \param[in]  to_ps				The target P-State for which you want to predict the execution time.
 * \param[out] proj_time    A pointer that will be filled with the projected execution time. */
state_t energy_model_project_time(energy_model_t energy_model, signature_t *signature, ulong from_ps, ulong to_ps,
                                  double *proj_time);

/** Project the power consumption of an application with the signature given as input argument.
 * Model's implementation dependant.
 * \param[in]  energy_model A loaded energy model.
 * \param[in]  signature    The input signature for the model containing metrics used to project the power.
 * \param[in]  from_ps			The P-State index you want the model to project from.
 * \param[in]  to_ps				The target P-State for which you want to predict the power consumption.
 * \param[out] proj_power   A pointer that will be filled with the projected power consumption. */
state_t energy_model_project_power(energy_model_t energy_model, signature_t *signature, ulong from_ps, ulong to_ps,
                                   double *proj_power);

/** Checks whether the loaded model has a projection from \ref from_ps to \ref to_ps.
 * Model's implementation dependant.
 * \return 0 If the input argument is invalid.
 * \return 0 If the energy model argument does not implement the method. */
uint energy_model_projection_available(energy_model_t energy_model, ulong from_ps, ulong to_ps);

/** Checks whether the loaded model has all projections.
 * Model's implementation dependant. */
uint energy_model_any_projection_available(energy_model_t energy_model);

#endif // _EAR_ENERGY_MODEL_H_
