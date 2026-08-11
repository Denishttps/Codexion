/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation_init.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 12:00:00 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "coder.h"
#include "priority_queue.h"
#include "simulation.h"
#include "utils.h"

int	init_coder(t_simulation *sim, int i)
{
	t_coder	*coder;

	coder = &sim->coders[i];
	coder->id = i + 1;
	coder->compile_count = 0;
	coder->burned_out = 0;
	coder->left_idx = i;
	coder->right_idx = (i + 1) % sim->config.coder_count;
	coder->left = &sim->dongles[coder->left_idx];
	coder->right = &sim->dongles[coder->right_idx];
	coder->last_compile_start = sim->start_time;
	coder->sim = sim;
	pthread_mutex_init(&coder->mutex, NULL);
	return (1);
}

int	init_dongles_coders(t_simulation *sim)
{
	int			i;
	t_dongle	*dongle;

	i = 0;
	while (i < sim->config.coder_count)
	{
		dongle = &sim->dongles[i];
		dongle->id = i + 1;
		dongle->available = true;
		dongle->last_release_ms = 0;
		wait_heap_init(&dongle->wait_heap, sim->config.coder_count);
		pthread_mutex_init(&dongle->mutex, NULL);
		pthread_cond_init(&dongle->cond, NULL);
		i++;
	}
	i = 0;
	while (i < sim->config.coder_count)
	{
		init_coder(sim, i);
		i++;
	}
	return (1);
}

int	init_mutexes(t_simulation *sim)
{
	pthread_mutex_init(&sim->log_mutex, NULL);
	pthread_mutex_init(&sim->simulation_mutex, NULL);
	pthread_mutex_init(&sim->counter_mutex, NULL);
	sim->request_counter = 0;
	sim->running = false;
	return (1);
}

int	start_threads(t_simulation *sim)
{
	int	i;

	sim->start_time = get_time_ms();
	i = 0;
	while (i < sim->config.coder_count)
	{
		sim->coders[i].last_compile_start = sim->start_time;
		i++;
	}
	sim->running = true;
	if (pthread_create(&sim->monitor_thread, NULL, monitor_thread, sim) != 0)
		return (0);
	i = 0;
	while (i < sim->config.coder_count)
	{
		if (pthread_create(&sim->coders[i].thread, NULL, coder_thread,
				&sim->coders[i]) != 0)
			return (0);
		i++;
	}
	return (1);
}
