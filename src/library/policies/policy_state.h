/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_POLICIES_STATES_H
#define EAR_POLICIES_STATES_H

#define EAR_POLICY_READY 	    0
#define EAR_POLICY_CONTINUE     1 /*!< Frequency has not been changed and next state must be the same. */
#define EAR_POLICY_GLOBAL_EV    2 /*!< Next state must be a global evaluation. */
#define EAR_POLICY_GLOBAL_READY 3 /*!< Global evaluation ready. */
#define EAR_POLICY_LOCAL_EV		4
#define EAR_POLICY_TRY_AGAIN    5 /*!< Frquency has been changed but next state must be the same. */

#define EAR_POLICY_NO_STATE     1000
#endif // EAR_POLICIES_H
