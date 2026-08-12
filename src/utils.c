/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/12 12:18:39 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.h"

long long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL));
}

bool	is_running(t_simulation *sim)
{
	bool	running;

	pthread_mutex_lock(&sim->simulation_mutex);
	running = sim->running;
	pthread_mutex_unlock(&sim->simulation_mutex);
	return (running);
}

void	ft_usleep(int ms, t_simulation *sim)
{
	long long	start;

	start = get_time_ms();
	while (get_time_ms() - start < ms)
	{
		if (!is_running(sim))
			break ;
		usleep(500);
	}
}

void	log_message(t_simulation *sim, int coder_id, const char *msg)
{
	long long	timestamp;

	pthread_mutex_lock(&sim->log_mutex);
	if (is_running(sim))
	{
		timestamp = get_time_ms() - sim->start_time;
		printf("%lld %d %s\n", timestamp, coder_id, msg);
	}
	pthread_mutex_unlock(&sim->log_mutex);
}
