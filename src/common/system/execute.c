/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*   
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*   
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING  
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
  }else if (WIFEXITED(ret) && WEXITSTATUS(ret)){
        error("Command %s terminates with exit status  %d",cmd,WEXITSTATUS(ret));
				return EAR_ERROR;
  }
	return EAR_SUCCESS;
}

