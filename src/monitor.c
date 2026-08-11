/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 18:07:06 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 18:07:06 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "simulation.h"
#include "utils.h"


int check_coders(t_simulation *sim)
{
	int			i;
	long long	now;
	t_coder		*coder;

	i = 0;
	now = get_time_ms();
	while (i < sim->config->coder_count)
	{
		coder = &sim->coders[i];
		if (now >= coder->deadline && !(coder->left_dongle_taken && coder->right_dongle_taken))
		{
			log_message(sim, coder->id, "burned out");
			sim->stop_simulation = 1;
			sim->burnout_coder_id = coder->id;
			return (coder->id);
		}
		i++;
	}
	return (-1);
}


void *monitor_thread(void *arg)
{
    t_simulation *sim = (t_simulation *)arg;
    int id;

    while (!sim->stop_simulation)
    {
        pthread_mutex_lock(&sim->state_mutex);
        id = check_coders(sim);
        if (id != -1)
        {
            pthread_cond_broadcast(&sim->state_cond);
            pthread_mutex_unlock(&sim->state_mutex);
            return (NULL);
        }
        pthread_mutex_unlock(&sim->state_mutex);
        usleep(500);
    }
    return (NULL);
}