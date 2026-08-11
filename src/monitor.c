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


void *monitor_thread(void *arg)
{
	t_simulation *sim;

	sim = (t_simulation *)arg;

	while (!sim->stop_simulation)
	{
		pthread_mutex_lock(&sim->state);
	}
}