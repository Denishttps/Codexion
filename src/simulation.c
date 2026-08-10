/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/10 14:30:09 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/10 16:42:30 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "simulation.h"
#include "coder.h"
#include "utils.h"

void *monitor_thread(void *arg)
{
	t_simulation *sim = arg;
	(void)sim;
	return (NULL);
}

static int init_dongles_coders(t_simulation *sim, const t_config *config)
{
	int i;

	i = 0;
	while (i < config->coder_count)
	{
		sim->dongles[i].id = i + 1;
		sim->dongles[i].available = 1;
		sim->dongles[i].cooldown_until = 0;
		if (pthread_mutex_init(&sim->dongles[i].mutex, NULL) != 0
			|| pthread_cond_init(&sim->dongles[i].cond, NULL) != 0)
			return (0);

		sim->coders[i].id = i + 1;
		sim->coders[i].compile_count = 0;
		sim->coders[i].burned_out = 0;
		sim->coders[i].left_dongle_taken = 0;
		sim->coders[i].right_dongle_taken = 0;
		sim->coders[i].waiting = 0;
		sim->coders[i].last_compile_start = sim->start_time;
		sim->coders[i].deadline = sim->start_time + config->time_to_burnout;
		sim->coders[i].sim = sim;
		i++;
	}
	return (1);
}

static int start_coder_threads(t_simulation *sim)
{
	int i;

	i = 0;
	while (i < sim->config->coder_count)
	{
		if (pthread_create(&sim->coders[i].thread, NULL,
			coder_thread, &sim->coders[i]) != 0)
			return (0);
		i++;
	}
	return (1);
}

void destroy_simulation(t_simulation *sim)
{
	int i;

	pthread_mutex_lock(&sim->state_mutex);
	sim->stop_simulation = 1;
	pthread_cond_broadcast(&sim->state_cond);
	pthread_mutex_unlock(&sim->state_mutex);

	i = 0;
	while (i < sim->config->coder_count)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	pthread_join(sim->monitor_thread, NULL);

	i = 0;
	while (i < sim->config->coder_count)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		pthread_cond_destroy(&sim->dongles[i].cond);
		i++;
	}

	pthread_mutex_destroy(&sim->log_mutex);
	pthread_mutex_destroy(&sim->state_mutex);
	pthread_cond_destroy(&sim->state_cond);

	free(sim->coders);
	free(sim->dongles);
}

int init_simulation(t_simulation *sim, const t_config *config)
{
	sim->stop_simulation = 0;
	sim->burnout_coder_id = 0;
	sim->config = (t_config *)config;
	sim->start_time = get_time_ms();

	sim->coders = malloc(sizeof(t_coder) * config->coder_count);
	sim->dongles = malloc(sizeof(t_dongle) * config->coder_count);
	if (!sim->coders || !sim->dongles)
	{
		free(sim->coders);
		free(sim->dongles);
		return (0);
	}

	if (pthread_mutex_init(&sim->log_mutex, NULL) != 0
		|| pthread_mutex_init(&sim->state_mutex, NULL) != 0
		|| pthread_cond_init(&sim->state_cond, NULL) != 0)
	{
		free(sim->coders);
		free(sim->dongles);
		return (0);
	}

	if (!init_dongles_coders(sim, config))
		return (0);
	if (!start_coder_threads(sim))
		return (0);
	if (pthread_create(&sim->monitor_thread, NULL, monitor_thread, sim) != 0)
		return (0);

	return (1);
}

