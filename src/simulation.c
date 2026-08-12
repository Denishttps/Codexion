/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/12 12:18:34 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "coder.h"
#include "priority_queue.h"
#include "simulation.h"
#include "utils.h"

void	wake_dongles(t_simulation *sim)
{
	int	i;

	i = 0;
	while (i < sim->config.coder_count)
	{
		pthread_mutex_lock(&sim->dongles[i].mutex);
		pthread_cond_broadcast(&sim->dongles[i].cond);
		pthread_mutex_unlock(&sim->dongles[i].mutex);
		i++;
	}
}

void	request_stop(t_simulation *sim)
{
	pthread_mutex_lock(&sim->simulation_mutex);
	sim->running = false;
	pthread_mutex_unlock(&sim->simulation_mutex);
	wake_dongles(sim);
}

void	wait_simulation(t_simulation *sim)
{
	int	i;

	pthread_join(sim->monitor_thread, NULL);
	wake_dongles(sim);
	i = 0;
	while (i < sim->config.coder_count)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
}

void	destroy_simulation(t_simulation *sim)
{
	int	i;

	request_stop(sim);
	i = 0;
	while (i < sim->config.coder_count)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		pthread_cond_destroy(&sim->dongles[i].cond);
		wait_heap_destroy(&sim->dongles[i].wait_heap);
		pthread_mutex_destroy(&sim->coders[i].mutex);
		i++;
	}
	pthread_mutex_destroy(&sim->simulation_mutex);
	pthread_mutex_destroy(&sim->counter_mutex);
	pthread_mutex_destroy(&sim->log_mutex);
	free(sim->coders);
	free(sim->dongles);
}

int	init_simulation(t_simulation *sim, const t_config *config)
{
	memset(sim, 0, sizeof(*sim));
	sim->config = *config;
	sim->coders = malloc(sizeof(t_coder) * config->coder_count);
	sim->dongles = malloc(sizeof(t_dongle) * config->coder_count);
	if (!sim->coders || !sim->dongles)
	{
		free(sim->coders);
		free(sim->dongles);
		return (0);
	}
	init_mutexes(sim);
	init_dongles_coders(sim);
	if (!start_threads(sim))
	{
		request_stop(sim);
		wait_simulation(sim);
		destroy_simulation(sim);
		return (0);
	}
	return (1);
}
