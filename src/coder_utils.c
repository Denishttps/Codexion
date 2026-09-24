/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_utils.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/09/24 17:25:25 by dbobrov          ###   ########.fr       */
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

static void	cancel_pair_request(t_coder *coder, t_simulation *sim)
{
	wait_heap_remove(&sim->pair_heap, coder->id, &sim->config);
	pthread_cond_broadcast(&sim->pair_cond);
}

static int	best_grantable_coder(t_coder *coder, t_simulation *sim)
{
	t_request	current;
	t_request	best;
	t_coder		*current_coder;
	int			i;
	int			found;

	i = 0;
	found = 0;
	while (i < sim->pair_heap.size)
	{
		current = sim->pair_heap.items[i];
		current_coder = &sim->coders[current.coder_id - 1];
		if (current_coder->left->available && current_coder->right->available
			&& cooldown_ok(current_coder->left, sim)
			&& cooldown_ok(current_coder->right, sim)
			&& (!found || wait_heap_compare(&current, &best,
					&sim->config) < 0))
		{
			best = current;
			found = 1;
		}
		i++;
	}
	if (!found)
		return (0);
	return (best.coder_id == coder->id);
}

int	take_two_dongles(t_coder *coder, t_simulation *sim)
{
	t_request	req;
	int			ready;

	pthread_mutex_lock(&coder->mutex);
	req.coder_id = coder->id;
	req.deadline = coder->last_compile_start + sim->config.time_to_burnout;
	pthread_mutex_unlock(&coder->mutex);
	pthread_mutex_lock(&sim->counter_mutex);
	req.arrival_order = sim->request_counter++;
	pthread_mutex_unlock(&sim->counter_mutex);
	pthread_mutex_lock(&sim->pair_mutex);
	ready = wait_heap_push(&sim->pair_heap, req, &sim->config);
	if (!ready)
	{
		pthread_mutex_unlock(&sim->pair_mutex);
		return (0);
	}
	while (is_running(sim))
	{
		ready = best_grantable_coder(coder, sim);
		if (ready)
		{
			wait_heap_remove(&sim->pair_heap, coder->id, &sim->config);
			coder->left->available = false;
			coder->right->available = false;
		}
		if (ready)
		{
			pthread_mutex_unlock(&sim->pair_mutex);
			return (1);
		}
		pthread_mutex_unlock(&sim->pair_mutex);
		usleep(1000);
		pthread_mutex_lock(&sim->pair_mutex);
	}
	cancel_pair_request(coder, sim);
	pthread_mutex_unlock(&sim->pair_mutex);
	return (0);
}

void	release_dongle(t_dongle *dongle, t_simulation *sim)
{
	pthread_mutex_lock(&sim->pair_mutex);
	pthread_mutex_lock(&dongle->mutex);
	dongle->available = true;
	dongle->last_release_ms = get_time_ms();
	pthread_cond_broadcast(&dongle->cond);
	pthread_mutex_unlock(&dongle->mutex);
	pthread_cond_broadcast(&sim->pair_cond);
	pthread_mutex_unlock(&sim->pair_mutex);
}
