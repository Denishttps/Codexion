/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_utils.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/12 12:17:58 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "coder.h"
#include "priority_queue.h"
#include "utils.h"

int	can_take(t_dongle *dongle, int coder_id)
{
	t_request	top;

	if (dongle->wait_heap.size == 0)
		return (0);
	top = wait_heap_peek(&dongle->wait_heap);
	if (top.coder_id != coder_id)
		return (0);
	if (!dongle->available)
		return (0);
	return (1);
}

int	cooldown_ok(t_dongle *dongle, t_simulation *sim)
{
	if (dongle->last_release_ms == 0)
		return (1);
	if (get_time_ms() - dongle->last_release_ms >= sim->config.dongle_cooldown)
		return (1);
	return (0);
}

void	wait_cooldown(t_dongle *dongle, t_simulation *sim)
{
	struct timeval	tv;
	struct timespec	ts;
	long long		elapsed;
	long long		remaining;

	elapsed = get_time_ms() - dongle->last_release_ms;
	if (elapsed >= sim->config.dongle_cooldown)
		return ;
	remaining = sim->config.dongle_cooldown - elapsed;
	gettimeofday(&tv, NULL);
	ts.tv_sec = tv.tv_sec + (tv.tv_usec / 1000 + remaining) / 1000;
	ts.tv_nsec = ((tv.tv_usec / 1000 + remaining) % 1000) * 1000000;
	pthread_cond_timedwait(&dongle->cond, &dongle->mutex, &ts);
}

void	acquire_dongle(t_coder *coder, t_simulation *sim, t_dongle *dongle)
{
	t_request	req;

	req.coder_id = coder->id;
	pthread_mutex_lock(&coder->mutex);
	req.deadline = coder->last_compile_start + sim->config.time_to_burnout;
	pthread_mutex_unlock(&coder->mutex);
	pthread_mutex_lock(&sim->counter_mutex);
	req.arrival_order = sim->request_counter++;
	pthread_mutex_unlock(&sim->counter_mutex);
	pthread_mutex_lock(&dongle->mutex);
	wait_heap_push(&dongle->wait_heap, req, &sim->config);
	while (is_running(sim) && (!can_take(dongle, coder->id)
			|| !cooldown_ok(dongle, sim)))
	{
		if (can_take(dongle, coder->id))
			wait_cooldown(dongle, sim);
		else
			pthread_cond_wait(&dongle->cond, &dongle->mutex);
	}
	if (is_running(sim))
	{
		wait_heap_pop(&dongle->wait_heap, &sim->config);
		dongle->available = false;
	}
	pthread_mutex_unlock(&dongle->mutex);
}

void	release_dongle(t_dongle *dongle)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->available = true;
	dongle->last_release_ms = get_time_ms();
	pthread_cond_broadcast(&dongle->cond);
	pthread_mutex_unlock(&dongle->mutex);
}
