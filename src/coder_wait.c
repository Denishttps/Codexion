/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_wait.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 14:09:59 by dbobrov           #+#    #+#             */
/*   Updated: 2026/09/25 14:10:01 by dbobrov          ###   ########.fr       */
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
	if (top.coder_id != coder_id || !dongle->available)
		return (0);
	return (1);
}

int	cooldown_ok(t_dongle *dongle, t_simulation *sim)
{
	if (dongle->last_release_ms == 0)
		return (1);
	return (get_time_ms() - dongle->last_release_ms
		>= sim->config.dongle_cooldown);
}

void	wait_cooldown(t_dongle *dongle, t_simulation *sim)
{
	struct timespec	ts;
	long long		elapsed;
	long long		remaining;

	elapsed = get_time_ms() - dongle->last_release_ms;
	if (elapsed >= sim->config.dongle_cooldown)
		return ;
	remaining = sim->config.dongle_cooldown - elapsed;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += remaining / 1000;
	ts.tv_nsec += (remaining % 1000) * 1000000;
	if (ts.tv_nsec >= 1000000000L)
	{
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}
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
