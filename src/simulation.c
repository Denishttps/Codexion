/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/10 14:30:09 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 18:11:49 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "simulation.h"
#include "coder.h"
#include "priority_queue.h"
#include "utils.h"

static void	request_stop(t_simulation *sim)
{
	if (!sim->state_mutex_initialized)
		return ;
	pthread_mutex_lock(&sim->state_mutex);
	sim->stop_simulation = 1;
	if (sim->state_cond_initialized)
		pthread_cond_broadcast(&sim->state_cond);
	pthread_mutex_unlock(&sim->state_mutex);
}

static int	init_coder(t_simulation *sim, int i)
{
	t_coder	*coder;

	coder = &sim->coders[i];
	coder->id = i + 1;
	coder->compile_count = 0;
	coder->burned_out = 0;
	coder->left_dongle_taken = 0;
	coder->right_dongle_taken = 0;
	coder->waiting = 0;
	coder->heap_index = -1;
	coder->request_time = 0;
	coder->last_compile_start = sim->start_time;
	coder->deadline = sim->start_time + sim->config.time_to_burnout;
	coder->sim = sim;
	return (1);
}

static int	init_dongles_coders(t_simulation *sim)
{
	int			i;
	t_dongle	*dongle;

	i = 0;
	while (i < sim->config.coder_count)
	{
		dongle = &sim->dongles[i];
		dongle->id = i + 1;
		dongle->available = 1;
		dongle->cooldown_until = 0;
		if (pthread_mutex_init(&dongle->mutex, NULL) != 0)
			return (0);
		if (pthread_cond_init(&dongle->cond, NULL) != 0)
		{
			pthread_mutex_destroy(&dongle->mutex);
			return (0);
		}
		sim->dongles_initialized++;
		init_coder(sim, i);
		i++;
	}
	return (1);
}

static int	start_coder_threads(t_simulation *sim)
{
	int	i;

	i = 0;
	while (i < sim->config.coder_count)
	{
		if (pthread_create(&sim->coders[i].thread, NULL,
				coder_thread, &sim->coders[i]) != 0)
			return (0);
		sim->coder_threads_started++;
		i++;
	}
	return (1);
}

static int	init_mutexes(t_simulation *sim)
{
	if (pthread_mutex_init(&sim->log_mutex, NULL) != 0)
		return (0);
	sim->log_mutex_initialized = 1;
	if (pthread_mutex_init(&sim->state_mutex, NULL) != 0)
		return (0);
	sim->state_mutex_initialized = 1;
	if (pthread_cond_init(&sim->state_cond, NULL) != 0)
		return (0);
	sim->state_cond_initialized = 1;
	return (1);
}

void	wait_simulation(t_simulation *sim)
{
	int	i;

	if (sim->threads_joined)
		return ;
	i = 0;
	while (i < sim->coder_threads_started)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	request_stop(sim);
	if (sim->monitor_thread_started)
		pthread_join(sim->monitor_thread, NULL);
	sim->threads_joined = 1;
}

void	destroy_simulation(t_simulation *sim)
{
	int	i;

	if (!sim->threads_joined)
	{
		request_stop(sim);
		wait_simulation(sim);
	}
	i = 0;
	while (i < sim->dongles_initialized)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		pthread_cond_destroy(&sim->dongles[i].cond);
		i++;
	}
	if (sim->heap_initialized)
		wait_heap_destroy(&sim->wait_heap);
	if (sim->state_cond_initialized)
		pthread_cond_destroy(&sim->state_cond);
	if (sim->state_mutex_initialized)
		pthread_mutex_destroy(&sim->state_mutex);
	if (sim->log_mutex_initialized)
		pthread_mutex_destroy(&sim->log_mutex);
	free(sim->coders);
	free(sim->dongles);
}

int	init_simulation(t_simulation *sim, const t_config *config)
{
	memset(sim, 0, sizeof(*sim));
	sim->config = *config;
	sim->start_time = get_time_ms();
	sim->coders = malloc(sizeof(t_coder) * config->coder_count);
	sim->dongles = malloc(sizeof(t_dongle) * config->coder_count);
	if (!sim->coders || !sim->dongles)
	{
		destroy_simulation(sim);
		return (0);
	}
	if (!init_mutexes(sim))
	{
		destroy_simulation(sim);
		return (0);
	}
	if (!wait_heap_init(&sim->wait_heap, config->coder_count))
	{
		destroy_simulation(sim);
		return (0);
	}
	sim->heap_initialized = 1;
	if (!init_dongles_coders(sim) || !start_coder_threads(sim))
	{
		destroy_simulation(sim);
		return (0);
	}
	if (pthread_create(&sim->monitor_thread, NULL, monitor_thread, sim) != 0)
	{
		destroy_simulation(sim);
		return (0);
	}
	sim->monitor_thread_started = 1;
	return (1);
}
