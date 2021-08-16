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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/version.h>
//#define SHOW_DEBUGS 1
#include <common/sizes.h>
#include <common/states.h>
#include <common/config.h>
#include <common/output/verbose.h>

// PATHS list
static char *cpufreq_path="/sys/devices/system/cpu/cpu";
static char *cpufreq_driver_folder="cpufreq";
static char *curr_freq_file="scaling_cur_freq";
static char *available_frequencies_file="scaling_available_frequencies";
static char *def_freq_file="scaling_setspeed";
static char *scaling_available_governors_file="scaling_available_governors";
static char *scaling_governor_file="scaling_governor";
static char *scaling_driver_file="scaling_driver";
static char *scaling_max_freq_file="scaling_max_freq";
static char *scaling_min_freq_file="scaling_min_freq";

#define GOVERNOR_MAX_NAME_LEN 128

typedef struct governor{
	char name[GOVERNOR_MAX_NAME_LEN];
	unsigned long max_f,min_f;
}governor_t;

#define FREQ_SIZE 8
#define LF (char)10
#define SPACE (char)32

/* AUXILIARY FUNCTIONS */

unsigned int is_number(char c)
{
	return ((c>='0') && (c<='9')); 
}

unsigned int is_sep(char c)
{
	return ((c==LF) || (c==SPACE));
}

unsigned long read_one_freq(int fd)
{
	unsigned long my_freq;
	int i=0;
	char my_freq_char[FREQ_SIZE],c;
  while((read(fd,&c,sizeof(char))>0) && (is_number(c))){
    my_freq_char[i]=c;
    i++;
    //printf("char %d is %c(%u)\n",i,c,(unsigned int )c);
  }
	//printf("Last %u\n",c);
  my_freq_char[i]='\0';
  my_freq=atoi(my_freq_char);
	return my_freq;

}
unsigned long read_one_word(int fd,char *myword)
{
  int i=0;
  char c;
  while((read(fd,&c,sizeof(char))>0) && (!is_sep(c)))
	{
    myword[i]=c;
    i++;
    //printf("char %d is %c(%u)\n",i,c,(unsigned int )c);
  }
  // printf("Last %u\n",c);
  myword[i]='\0';
  return i;
}

unsigned long write_one_freq(int fd, unsigned long f)
{
	int ret;
	char my_freq_char[FREQ_SIZE];
  sprintf(my_freq_char,"%lu",f);
  my_freq_char[strlen(my_freq_char)]=LF;
  ret=write(fd,my_freq_char,strlen(my_freq_char)+1);
  if (ret<0){
    perror("Error writting file");
    return 0;
  }else if (ret!=(strlen(my_freq_char)+1)){
    debug("Error writting file expected bytes %lu, written %d\n",strlen(my_freq_char)+1,ret);
    return 0;
  }
	return f;

}

/* CPUPOWER API */

unsigned long CPUfreq_get_cpufreq_driver(int cpu,char *src)
{
  int fd;
	unsigned long ret;
  char my_freq_char[FREQ_SIZE];
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,scaling_driver_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_RDONLY);
  if (fd<0){
    perror("Opening file");
    return 0;
  }
	ret=read_one_word(fd,src);
	close(fd);
	return ret;
}

unsigned long CPUfreq_get_cpufreq_governor(int cpu,char *src)
{
  int fd;
  unsigned long ret;
  char my_freq_char[FREQ_SIZE];
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,scaling_governor_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_RDONLY);
  if (fd<0){
    perror("Opening file");
    return 0;
  }
  ret=read_one_word(fd,src);
	close(fd);
  return ret;
}

unsigned long CPUfreq_get_cpufreq_maxf(int cpu)
{
  int fd;
  unsigned long ret;
  char my_freq_char[FREQ_SIZE];
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,scaling_max_freq_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_RDONLY);
  if (fd<0){
    perror("Opening file");
    return 0;
  }
  ret=read_one_freq(fd);
	close(fd);
  return ret;
}

unsigned long CPUfreq_get_cpufreq_minf(int cpu)
{
  int fd;
  unsigned long ret;
  char my_freq_char[FREQ_SIZE];
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,scaling_min_freq_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_RDONLY);
  if (fd<0){
    perror("Opening file");
    return 0;
  }
  ret=read_one_freq(fd);
	close(fd);
  return ret;
}

unsigned long CPUfreq_set_cpufreq_governor(int cpu,char *name)
{
	int fd,size,i,r;
  unsigned long ret;
  char my_freq_char[FREQ_SIZE];
  char curr_freq_path[PATH_MAX];
	char c=LF;
	debug("set governor %s in cpu %d",name,cpu);
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,scaling_governor_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_WRONLY);
  if (fd<0){
    perror("Opening file");
    return 0;
  }
	r=write(fd,name,strlen(name)+1);
	if (r<0){
		perror("Changing governor name");
		return 0;
	}
	close(fd);
	return 1;
}

unsigned long CPUfreq_set_cpufreq_minf(int cpu,unsigned long minf)
{
  int fd;
  unsigned long ret;
  char my_freq_char[FREQ_SIZE];
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,scaling_min_freq_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_WRONLY);
  if (fd<0){
    perror("Opening file");
    return 0;
  }
	ret= write_one_freq(fd,minf);	
	close(fd);
	return ret;
}
unsigned long CPUfreq_set_cpufreq_maxf(int cpu,unsigned long maxf)
{
  int fd;
  unsigned long ret;
  char my_freq_char[FREQ_SIZE];
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,scaling_max_freq_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_WRONLY);
  if (fd<0){
    perror("Opening file");
    return 0;
  }
  ret= write_one_freq(fd,maxf);
	close(fd);
  return ret;

}




unsigned long CPUfreq_set_frequency(unsigned int cpu, unsigned long f)
{
  int fd,ret;
  char my_freq_char[FREQ_SIZE];
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,def_freq_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_WRONLY);
  if (fd<0){
    perror("Opening file");
    return 0;
  }
	f=write_one_freq(fd,f);
	close(fd);
  return f;
}

void CPUfreq_put_available_frequencies(unsigned long *f)
{
  free(f);
}


unsigned long *CPUfreq_get_available_frequencies(int cpu,unsigned long *num_freqs)
{
	int fd;
  unsigned long *my_freq=NULL,f,num_f=0;
  char c,*freq_char;
  int i;
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,available_frequencies_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_RDONLY);
  if (fd<0){
    perror("Error opening file");
    return 0;
  }
	do{
		f=read_one_freq(fd);	
		if (f>0){
			num_f++;
			debug("Allocting memory for frequency %lu",num_f);
			my_freq=realloc(my_freq,num_f*sizeof(unsigned long));
			if (my_freq==NULL){
				debug("Error allocating memory for frequency list");
				return 0;
			}
			my_freq[num_f-1]=f;
		}
	}while(f>0);
	*num_freqs=num_f;
	close(fd);
	return my_freq;

}

char **CPUfreq_get_available_governors(int cpu,unsigned long *num_governors)
{
  int fd,gov_len;
  char c;
	char **governor,*c_governor;
  int i,num_gov=0;
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,scaling_available_governors_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_RDONLY);
  if (fd<0){
    perror("Error opening file");
    return 0;
  }
	c_governor=malloc(GOVERNOR_MAX_NAME_LEN*sizeof(char));
	debug("Malloc de %lu byes\n",sizeof(char*));
	governor=malloc(sizeof(char *));
  do{
    gov_len=read_one_word(fd,c_governor);
		debug("Governor %d = %s (%d)\n",num_gov,c_governor,gov_len);
    if (gov_len>0){
			governor[num_gov]=malloc(gov_len+1);
			strcpy(governor[num_gov],c_governor);
			num_gov++;
			debug("reaalloc de %lu byes\n",sizeof(char*)*num_gov);
			governor=realloc(governor,sizeof(char *)*(num_gov+1));
    }
  }while(gov_len>0);
	governor[num_gov]=NULL;
	debug("Totall governors found %d\n",num_gov);
  *num_governors=num_gov;
	close(fd);
  return governor;
}

void CPUfreq_put_available_governors(char **glist)
{
	int i=0;
	while(glist[i]!=NULL){
		free(glist[i]);
		i++;
	}
	free(glist);
}

void CPUfreq_get_policy(int cpu,governor_t *g)
{
	CPUfreq_get_cpufreq_governor(cpu,g->name);
	g->max_f=CPUfreq_get_cpufreq_maxf(cpu);
	g->min_f=CPUfreq_get_cpufreq_minf(cpu);
	debug("Savig policy %s maxf %lu minf %lu",g->name,g->max_f,g->min_f);
}


int CPUfreq_set_policy(int cpu,governor_t *g)
{
	unsigned long ret;
	debug("Restoring policy %s maxf %lu minf %lu",g->name,g->max_f,g->min_f);
  if ((ret=CPUfreq_set_cpufreq_governor(cpu,g->name))==0) return EAR_ERROR; 
  if ((ret=CPUfreq_set_cpufreq_maxf(cpu,g->max_f))==0) return EAR_ERROR;
  if ((ret=CPUfreq_set_cpufreq_minf(cpu,g->min_f))==0) return EAR_ERROR;
	return EAR_SUCCESS;
}

unsigned long CPUfreq_get_num_pstates(int cpu)
{
  int fd;
  unsigned long f,num_f=0;
  int i;
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu,cpufreq_driver_folder,available_frequencies_file);
  debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_RDONLY);
  if (fd<0){
    perror("Error opening file");
    return 0;
  }
  do{
    f=read_one_freq(fd);
		if (f>0) num_f++;
  }while(f>0);
	close(fd);
  return num_f;

}

unsigned long CPUfreq_get(int cpu_num)
{
  int fd;
  unsigned long my_freq;
  char curr_freq_path[PATH_MAX];
  sprintf(curr_freq_path,"%s%d/%s/%s",cpufreq_path,cpu_num,cpufreq_driver_folder,curr_freq_file);
	debug("Opening %s\n",curr_freq_path);
  fd=open(curr_freq_path,O_RDONLY);
  if (fd<0){
		perror("Error opening file");
    return 0;
  }
	my_freq=read_one_freq(fd);
	close(fd);
  return my_freq;

}


