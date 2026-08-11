/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/10 15:05:54 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/10 15:05:54 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "coder.h"
#include "priority_queue.h"
#include "utils.h"

static int	get_left_dongle(const t_coder *coder, int count)
{
	(void)count;
	return (coder->id - 1);
}

static int	get_right_dongle(const t_coder *coder, int count)
{
	return (coder->id % count);
}

static void	lock_dongle_pair(t_simulation *sim, int left, int right)
{
	if (left == right)
	{
		pthread_mutex_lock(&sim->dongles[left].mutex);
		return ;
	}
	if (left < right)
	{
		pthread_mutex_lock(&sim->dongles[left].mutex);
		pthread_mutex_lock(&sim->dongles[right].mutex);
	}
	else
	{
		pthread_mutex_lock(&sim->dongles[right].mutex);
		pthread_mutex_lock(&sim->dongles[left].mutex);
	}
}

static void	unlock_dongle_pair(t_simulation *sim, int left, int right)
{
	if (left == right)
	{
		pthread_mutex_unlock(&sim->dongles[left].mutex);
		return ;
	}
	pthread_mutex_unlock(&sim->dongles[right].mutex);
	pthread_mutex_unlock(&sim->dongles[left].mutex);
}

static void	set_abs_time_ms(struct timespec *time, long long ms)
{
	time->tv_sec = ms / 1000;
	time->tv_nsec = (ms % 1000) * 1000000;
}

static int	simulation_stopped(t_simulation *sim)
{
	int	stopped;

	pthread_mutex_lock(&sim->state_mutex);
	stopped = sim->stop_simulation;
	pthread_mutex_unlock(&sim->state_mutex);
	return (stopped);
}

static void	sleep_ms_interruptible(t_simulation *sim, int ms)
{
	long long	end;
	long long	now;
	int			chunk;

	end = get_time_ms() + ms;
	while (!simulation_stopped(sim))
	{
		now = get_time_ms();
		if (now >= end)
			break ;
		chunk = (int)(end - now);
		if (chunk > 1)
			chunk = 1;
		usleep(chunk * 1000);
	}
}

static int	dongles_ready(t_simulation *sim, int left, int right, long long now)
{
	int	ready;

	lock_dongle_pair(sim, left, right);
	ready = (sim->dongles[left].available
			&& sim->dongles[left].cooldown_until <= now
			&& sim->dongles[right].available
			&& sim->dongles[right].cooldown_until <= now);
	unlock_dongle_pair(sim, left, right);
	return (ready);
}

static long long	next_cooldown_wakeup(t_simulation *sim, int left, int right,
	long long now)
{
	long long	wakeup;

	wakeup = 0;
	lock_dongle_pair(sim, left, right);
	if (sim->dongles[left].available && sim->dongles[left].cooldown_until > now)
		wakeup = sim->dongles[left].cooldown_until;
	if (sim->dongles[right].available
		&& sim->dongles[right].cooldown_until > now
		&& sim->dongles[right].cooldown_until > wakeup)
		wakeup = sim->dongles[right].cooldown_until;
	unlock_dongle_pair(sim, left, right);
	return (wakeup);
}

static int	coder_pair_ready(t_simulation *sim, t_coder *coder, long long now)
{
	int	left;
	int	right;

	left = get_left_dongle(coder, sim->config.coder_count);
	right = get_right_dongle(coder, sim->config.coder_count);
	return (dongles_ready(sim, left, right, now));
}

static int	coder_pairs_conflict(t_simulation *sim, t_coder *a, t_coder *b)
{
	int	a_left;
	int	a_right;
	int	b_left;
	int	b_right;

	a_left = get_left_dongle(a, sim->config.coder_count);
	a_right = get_right_dongle(a, sim->config.coder_count);
	b_left = get_left_dongle(b, sim->config.coder_count);
	b_right = get_right_dongle(b, sim->config.coder_count);
	return (a_left == b_left || a_left == b_right
		|| a_right == b_left || a_right == b_right);
}

static int	higher_priority_can_run(t_coder *coder, t_simulation *sim,
	long long now)
{
	int		i;
	t_coder	*other;

	i = 0;
	while (i < sim->wait_heap.size)
	{
		other = sim->wait_heap.items[i];
		if (other != coder && other->waiting
			&& coder_pairs_conflict(sim, other, coder)
			&& wait_heap_compare(other, coder, &sim->config) < 0
			&& coder_pair_ready(sim, other, now))
			return (1);
		i++;
	}
	return (0);
}

int	take_two_dongles(t_coder *coder, t_simulation *sim)
{
	int				left;
	int				right;
	long long		now;
	long long		wakeup;
	struct timespec	time;

	left = get_left_dongle(coder, sim->config.coder_count);
	right = get_right_dongle(coder, sim->config.coder_count);
	pthread_mutex_lock(&sim->state_mutex);
	coder->waiting = 1;
	coder->request_time = get_time_ms();
	if (!wait_heap_push(&sim->wait_heap, coder, &sim->config))
		sim->stop_simulation = 1;
	while (!sim->stop_simulation)
	{
		now = get_time_ms();
		if (dongles_ready(sim, left, right, now)
			&& !higher_priority_can_run(coder, sim, now))
		{
			lock_dongle_pair(sim, left, right);
			sim->dongles[left].available = 0;
			sim->dongles[right].available = 0;
			unlock_dongle_pair(sim, left, right);
			wait_heap_remove(&sim->wait_heap, coder, &sim->config);
			coder->left_dongle_taken = 1;
			coder->right_dongle_taken = 1;
			coder->waiting = 0;
			pthread_cond_broadcast(&sim->state_cond);
			pthread_mutex_unlock(&sim->state_mutex);
			return (1);
		}
		wakeup = next_cooldown_wakeup(sim, left, right, now);
		if (wakeup > now)
		{
			set_abs_time_ms(&time, wakeup);
			pthread_cond_timedwait(&sim->state_cond, &sim->state_mutex,
				&time);
		}
		else
			pthread_cond_wait(&sim->state_cond, &sim->state_mutex);
	}
	wait_heap_remove(&sim->wait_heap, coder, &sim->config);
	coder->waiting = 0;
	pthread_mutex_unlock(&sim->state_mutex);
	return (0);
}

void	release_two_dongles(t_coder *coder, t_simulation *sim)
{
	int			left;
	int			right;
	long long	now;

	left = get_left_dongle(coder, sim->config.coder_count);
	right = get_right_dongle(coder, sim->config.coder_count);
	now = get_time_ms();
	pthread_mutex_lock(&sim->state_mutex);
	lock_dongle_pair(sim, left, right);
	sim->dongles[left].available = 1;
	sim->dongles[left].cooldown_until = now + sim->config.dongle_cooldown;
	sim->dongles[right].available = 1;
	sim->dongles[right].cooldown_until = now + sim->config.dongle_cooldown;
	unlock_dongle_pair(sim, left, right);
	coder->left_dongle_taken = 0;
	coder->right_dongle_taken = 0;
	pthread_cond_broadcast(&sim->state_cond);
	pthread_mutex_unlock(&sim->state_mutex);
}

static void	start_compile(t_coder *coder, t_simulation *sim)
{
	pthread_mutex_lock(&sim->state_mutex);
	coder->last_compile_start = get_time_ms();
	coder->deadline = coder->last_compile_start + sim->config.time_to_burnout;
	pthread_mutex_unlock(&sim->state_mutex);
}

static int	finish_compile(t_coder *coder, t_simulation *sim)
{
	int	keep_working;

	pthread_mutex_lock(&sim->state_mutex);
	coder->compile_count++;
	if (coder->compile_count == sim->config.compiles_required)
	{
		sim->completed_coders++;
		if (sim->completed_coders == sim->config.coder_count)
		{
			sim->stop_simulation = 1;
			pthread_cond_broadcast(&sim->state_cond);
		}
	}
	keep_working = (!sim->stop_simulation
			&& coder->compile_count < sim->config.compiles_required);
	pthread_mutex_unlock(&sim->state_mutex);
	return (keep_working);
}

static int	coder_should_continue(t_coder *coder, t_simulation *sim)
{
	int	keep_working;

	pthread_mutex_lock(&sim->state_mutex);
	keep_working = (!sim->stop_simulation
			&& coder->compile_count < sim->config.compiles_required);
	pthread_mutex_unlock(&sim->state_mutex);
	return (keep_working);
}

static int	take_only_dongle(t_coder *coder, t_simulation *sim)
{
	int	left;

	left = get_left_dongle(coder, sim->config.coder_count);
	pthread_mutex_lock(&sim->state_mutex);
	while (!sim->stop_simulation)
	{
		if (dongles_ready(sim, left, left, get_time_ms()))
		{
			lock_dongle_pair(sim, left, left);
			sim->dongles[left].available = 0;
			unlock_dongle_pair(sim, left, left);
			coder->left_dongle_taken = 1;
			coder->right_dongle_taken = 0;
			pthread_cond_broadcast(&sim->state_cond);
			pthread_mutex_unlock(&sim->state_mutex);
			return (1);
		}
		pthread_cond_wait(&sim->state_cond, &sim->state_mutex);
	}
	pthread_mutex_unlock(&sim->state_mutex);
	return (0);
}

static void	handle_single_coder(t_coder *coder, t_simulation *sim)
{
	if (!take_only_dongle(coder, sim))
		return ;
	log_message(sim, coder->id, "has taken a dongle");
	while (!simulation_stopped(sim))
		usleep(1000);
	release_two_dongles(coder, sim);
}

void	*coder_thread(void *arg)
{
	t_coder			*coder;
	t_simulation	*sim;

	coder = arg;
	sim = coder->sim;
	if (sim->config.coder_count == 1)
	{
		handle_single_coder(coder, sim);
		return (NULL);
	}
	while (coder_should_continue(coder, sim))
	{
		if (!take_two_dongles(coder, sim))
			break ;
		start_compile(coder, sim);
		log_message(sim, coder->id, "has taken a dongle");
		log_message(sim, coder->id, "has taken a dongle");
		log_message(sim, coder->id, "is compiling");
		sleep_ms_interruptible(sim, sim->config.time_to_compile);
		if (simulation_stopped(sim))
		{
			release_two_dongles(coder, sim);
			break ;
		}
		if (!finish_compile(coder, sim))
		{
			release_two_dongles(coder, sim);
			break ;
		}
		release_two_dongles(coder, sim);
		if (simulation_stopped(sim))
			break ;
		log_message(sim, coder->id, "is debugging");
		sleep_ms_interruptible(sim, sim->config.time_to_debug);
		if (simulation_stopped(sim))
			break ;
		log_message(sim, coder->id, "is refactoring");
		sleep_ms_interruptible(sim, sim->config.time_to_refactor);
	}
	return (NULL);
}
