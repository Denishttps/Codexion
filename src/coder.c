/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/09/24 17:20:50 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "coder.h"
#include "priority_queue.h"
#include "utils.h"

void	release_two_dongles(t_coder *coder, t_simulation *sim)
{
	(void)sim;
	release_dongle(coder->left, sim);
	release_dongle(coder->right, sim);
}

static int	compile_cycle(t_coder *coder, t_simulation *sim)
{
	if (!take_two_dongles(coder, sim))
		return (0);
	log_message(sim, coder->id, "has taken a dongle");
	log_message(sim, coder->id, "has taken a dongle");
	pthread_mutex_lock(&coder->mutex);
	coder->last_compile_start = get_time_ms();
	pthread_mutex_unlock(&coder->mutex);
	log_message(sim, coder->id, "is compiling");
	ft_usleep(sim->config.time_to_compile, sim);
	release_two_dongles(coder, sim);
	pthread_mutex_lock(&coder->mutex);
	coder->compile_count++;
	pthread_mutex_unlock(&coder->mutex);
	return (1);
}

static void	handle_single_coder(t_coder *coder, t_simulation *sim)
{
	acquire_dongle(coder, sim, coder->left);
	if (!is_running(sim))
		return ;
	log_message(sim, coder->id, "has taken a dongle");
	while (is_running(sim))
		ft_usleep(1000, sim);
	release_dongle(coder->left, sim);
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
	while (is_running(sim)
		&& coder->compile_count < sim->config.compiles_required)
	{
		if (!compile_cycle(coder, sim))
			break ;
		if (!is_running(sim))
			break ;
		log_message(sim, coder->id, "is debugging");
		ft_usleep(sim->config.time_to_debug, sim);
		log_message(sim, coder->id, "is refactoring");
		ft_usleep(sim->config.time_to_refactor, sim);
	}
	return (NULL);
}
