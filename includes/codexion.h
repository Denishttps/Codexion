#ifndef CODEXION_H
#define CODEXION_H

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>


typedef struct s_simulation t_simulation;

typedef struct s_coder
{
    int				id;
    int				compile_count;
    int				burned_out;
    long long		last_compile_start;
    long long		deadline;
    long long		request_time;
    int				left_dongle_taken;
    int				right_dongle_taken;
    int				waiting;
    pthread_t		thread;
    t_simulation	*sim;
} t_coder;

typedef struct s_dongle
{
    int				id;
    int				available;
    long long		cooldown_until;
    pthread_mutex_t	mutex;
    pthread_cond_t	cond;
} t_dongle;

typedef enum e_scheduler
{
    SCHEDULER_FIFO,
    SCHEDULER_EDF
}   t_scheduler;

typedef struct s_config
{
    int			coder_count;
    int			time_to_burnout;
    int			time_to_compile;
    int			time_to_debug;
    int			time_to_refactor;
    int			compiles_required;
    int			dongle_cooldown;
    t_scheduler	scheduler;
}   t_config;

typedef struct s_simulation
{
    t_coder			*coders;
    t_dongle		*dongles;
	t_config		*config;
	
	pthread_t		monitor_thread;
    pthread_mutex_t	log_mutex;
    pthread_mutex_t	state_mutex;
    pthread_cond_t	state_cond;
	
	long long		start_time;
    int				stop_simulation;
    int				burnout_coder_id;
}   t_simulation;


#endif
