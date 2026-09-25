/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_utils.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 14:50:31 by dbobrov           #+#    #+#             */
/*   Updated: 2026/09/25 15:08:47 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "coder.h"
#include "priority_queue.h"
#include "utils.h"

static int	dongles_grantable(t_coder *coder, t_simulation *sim)
{
	return (coder->left->available && coder->right->available
		&& cooldown_ok(coder->left, sim)
		&& cooldown_ok(coder->right, sim));
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
		if (dongles_grantable(current_coder, sim)
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

static int	wait_for_pair(t_coder *coder, t_simulation *sim)
{
	while (is_running(sim))
	{
		if (best_grantable_coder(coder, sim))
		{
			wait_heap_remove(&sim->pair_heap, coder->id,
				&sim->config);
			coder->left->available = false;
			coder->right->available = false;
			return (1);
		}
		pthread_mutex_unlock(&sim->pair_mutex);
		usleep(1000);
		pthread_mutex_lock(&sim->pair_mutex);
	}
	wait_heap_remove(&sim->pair_heap, coder->id, &sim->config);
	pthread_cond_broadcast(&sim->pair_cond);
	return (0);
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
	if (ready)
		ready = wait_for_pair(coder, sim);
	pthread_mutex_unlock(&sim->pair_mutex);
	return (ready);
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
