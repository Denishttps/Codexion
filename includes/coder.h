/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 22:29:42 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODER_H
# define CODER_H

# include "codexion.h"

void	*coder_thread(void *arg);
void	release_two_dongles(t_coder *coder, t_simulation *sim);
int		can_take(t_dongle *dongle, int coder_id);
int		cooldown_ok(t_dongle *dongle, t_simulation *sim);
void	wait_cooldown(t_dongle *dongle, t_simulation *sim);
void	acquire_dongle(t_coder *coder, t_simulation *sim, t_dongle *dongle);
void	release_dongle(t_dongle *dongle);
void	log_message(t_simulation *sim, int coder_id, const char *msg);
int		take_two_dongles(t_coder *coder, t_simulation *sim);

#endif
