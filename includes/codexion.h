/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 12:00:00 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#ifndef CODEXION_H
# define CODEXION_H

# include <limits.h>
# include <pthread.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/time.h>
# include <time.h>
# include <unistd.h>

typedef struct s_simulation	t_simulation;

typedef enum e_scheduler
{
	SCHEDULER_FIFO,
	SCHEDULER_EDF
}							t_scheduler;

typedef struct s_config
{
	int						coder_count;
	int						time_to_burnout;
	int						time_to_compile;
	int						time_to_debug;
	int						time_to_refactor;
	int						compiles_required;
	int						dongle_cooldown;
	t_scheduler				scheduler;
}							t_config;

typedef struct s_request
{
	int						coder_id;
	long long				deadline;
	long long				arrival_order;
}							t_request;

typedef struct s_wait_heap
{
	t_request				*items;
	int						size;
	int						capacity;
}							t_wait_heap;

typedef struct s_dongle
{
	int						id;
	bool					available;
	long long				last_release_ms;
	t_wait_heap				wait_heap;
	pthread_mutex_t			mutex;
	pthread_cond_t			cond;
}							t_dongle;

typedef struct s_coder
{
	int						id;
	int						compile_count;
	int						burned_out;
	long long				last_compile_start;

	t_dongle				*left;
	t_dongle				*right;
	int						left_idx;
	int						right_idx;

	pthread_t				thread;
	pthread_mutex_t			mutex;
	t_simulation			*sim;
}							t_coder;

typedef struct s_simulation
{
	t_coder					*coders;
	t_dongle				*dongles;
	t_config				config;

	pthread_t				monitor_thread;
	pthread_mutex_t			log_mutex;

	pthread_mutex_t			simulation_mutex;
	bool					running;
	long long				start_time;

	long long				request_counter;
	pthread_mutex_t			counter_mutex;

	int						burnout_coder_id;
}							t_simulation;

#endif
