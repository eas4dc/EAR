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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
}
int execute(char *cmd)
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

