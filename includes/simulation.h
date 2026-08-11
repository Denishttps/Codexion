/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 12:00:00 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#ifndef SIMULATION_H
# define SIMULATION_H

# include "codexion.h"

int		init_simulation(t_simulation *sim, const t_config *config);
void	wait_simulation(t_simulation *sim);
void	wake_dongles(t_simulation *sim);
void	*monitor_thread(void *arg);
void	destroy_simulation(t_simulation *sim);
int		init_coder(t_simulation *sim, int i);
int		init_dongles_coders(t_simulation *sim);
int		init_mutexes(t_simulation *sim);
int		start_threads(t_simulation *sim);
void	request_stop(t_simulation *sim);

#endif
