/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/12 12:18:17 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "simulation.h"
#include "utils.h"

static void	handle_burnout(t_simulation *sim, int idx, long long now)
{
	pthread_mutex_lock(&sim->log_mutex);
	printf("%lld %d burned out\n", now - sim->start_time, sim->coders[idx].id);
	pthread_mutex_lock(&sim->simulation_mutex);
	sim->running = false;
	pthread_mutex_unlock(&sim->simulation_mutex);
	pthread_mutex_unlock(&sim->log_mutex);
	wake_dongles(sim);
}

static void	check_burnout(t_simulation *sim)
{
	int			i;
	long long	now;

	i = 0;
	while (i < sim->config.coder_count)
	{
		pthread_mutex_lock(&sim->coders[i].mutex);
		if (sim->coders[i].compile_count >= sim->config.compiles_required)
		{
			pthread_mutex_unlock(&sim->coders[i].mutex);
			i++;
			continue ;
		}
		now = get_time_ms();
		if (now
			- sim->coders[i].last_compile_start >= sim->config.time_to_burnout)
		{
			pthread_mutex_unlock(&sim->coders[i].mutex);
			handle_burnout(sim, i, now);
			return ;
		}
		pthread_mutex_unlock(&sim->coders[i].mutex);
		i++;
	}
}

static void	check_all_done(t_simulation *sim)
{
	int	i;

	i = 0;
	while (i < sim->config.coder_count)
	{
		pthread_mutex_lock(&sim->coders[i].mutex);
		if (sim->coders[i].compile_count < sim->config.compiles_required)
		{
			pthread_mutex_unlock(&sim->coders[i].mutex);
			return ;
		}
		pthread_mutex_unlock(&sim->coders[i].mutex);
		i++;
	}
	pthread_mutex_lock(&sim->simulation_mutex);
	sim->running = false;
	pthread_mutex_unlock(&sim->simulation_mutex);
	wake_dongles(sim);
}

void	*monitor_thread(void *arg)
{
	t_simulation	*sim;

	sim = (t_simulation *)arg;
	while (is_running(sim))
	{
		check_burnout(sim);
		if (!is_running(sim))
			break ;
		check_all_done(sim);
		usleep(1000);
	}
	return (NULL);
}
