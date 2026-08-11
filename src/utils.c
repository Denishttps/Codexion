/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/10 14:59:11 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/10 14:59:11 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.h"

long long get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);
}

void log_message(t_simulation *sim, int coder_id, const char *msg)
{
    long long timestamp;

    timestamp = get_time_ms() - sim->start_time;
    pthread_mutex_lock(&sim->log_mutex);
    printf("%lld %d %s\n", timestamp, coder_id, msg);
    pthread_mutex_unlock(&sim->log_mutex);
}