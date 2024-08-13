/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <execinfo.h>

#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>

int execute_with_fork(char *cmd)
{
  int ret;
   ret=fork();
   if (ret==0){
      debug("Executing %s",cmd);
      ret=system(cmd);
      if (WIFSIGNALED(ret)){
        error("Command %s terminates with signal %d",cmd,WTERMSIG(ret));
      }else if (WIFEXITED(ret) && WEXITSTATUS(ret)){
        error("Command %s terminates with exit status  %d",cmd,WEXITSTATUS(ret));
      }
			exit(0);
    }else if (ret<0){
      return EAR_ERROR;
    }else return EAR_SUCCESS;
    return EAR_SUCCESS;
}
__attribute__ ((used))int execute(char *cmd)
{
  int ret;
  ret=system(cmd);
  if (WIFSIGNALED(ret)){
        error("Command %s terminates with signal %d",cmd,WTERMSIG(ret));
				return EAR_ERROR;
  }
	else if (WIFEXITED(ret) && WEXITSTATUS(ret)){
        error("Command %s terminates with exit status  %d",cmd,WEXITSTATUS(ret));
				return EAR_ERROR;
  }
	return EAR_SUCCESS;
}


void print_stack(int fd)
{
  void *array[10];
  char **strings;
  int size, i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  if (strings != NULL)
  {

    dprintf(fd, "Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++)
      dprintf (fd, "%s - from backtrace %p\n", strings[i], array[i]);
  }

  free (strings);

}

void *get_stack(int lv)
{
  void *array[10];
  int size = backtrace (array, ((lv+1 > 10) ? 10 : lv+1));
  char **strings;
  strings = backtrace_symbols (array, size);
  free (strings);
  return array[lv];
}

