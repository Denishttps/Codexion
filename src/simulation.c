/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/10 14:30:09 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 18:11:49 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "simulation.h"
#include "coder.h"
#include "priority_queue.h"
#include "utils.h"

void wake_dongles(t_simulation *sim)
{
    int i;

    i = 0;
    while (i < sim->config.coder_count)
    {
        pthread_mutex_lock(&sim->dongles[i].mutex);
        pthread_cond_broadcast(&sim->dongles[i].cond);
        pthread_mutex_unlock(&sim->dongles[i].mutex);
        i++;
    }
}

static void request_stop(t_simulation *sim)
{
    pthread_mutex_lock(&sim->simulation_mutex);
    sim->running = false;
    pthread_mutex_unlock(&sim->simulation_mutex);
    wake_dongles(sim);
}

static int init_coder(t_simulation *sim, int i)
{
    t_coder *coder;

    coder = &sim->coders[i];
    coder->id = i + 1;
    coder->compile_count = 0;
    coder->burned_out = 0;
    coder->left_idx = i;
    coder->right_idx = (i + 1) % sim->config.coder_count;
    coder->left = &sim->dongles[coder->left_idx];
    coder->right = &sim->dongles[coder->right_idx];
    coder->last_compile_start = sim->start_time;
    coder->sim = sim;
    pthread_mutex_init(&coder->mutex, NULL);
    return (1);
}

static int init_dongles_coders(t_simulation *sim)
{
    int         i;
    t_dongle    *dongle;

    i = 0;
    while (i < sim->config.coder_count)
    {
        dongle = &sim->dongles[i];
        dongle->id = i + 1;
        dongle->available = true;
        dongle->last_release_ms = 0;
        wait_heap_init(&dongle->wait_heap, sim->config.coder_count);
        pthread_mutex_init(&dongle->mutex, NULL);
        pthread_cond_init(&dongle->cond, NULL);
        i++;
    }
    i = 0;
    while (i < sim->config.coder_count)
    {
        init_coder(sim, i);
        i++;
    }
    return (1);
}

static int start_threads(t_simulation *sim)
{
    int i;

    sim->start_time = get_time_ms();
    i = 0;
    while (i < sim->config.coder_count)
    {
        sim->coders[i].last_compile_start = sim->start_time;
        i++;
    }
    sim->running = true;
    if (pthread_create(&sim->monitor_thread, NULL, monitor_thread, sim) != 0)
        return (0);
    i = 0;
    while (i < sim->config.coder_count)
    {
        if (pthread_create(&sim->coders[i].thread, NULL, coder_thread, &sim->coders[i]) != 0)
            return (0);
        i++;
    }
    return (1);
}

static int init_mutexes(t_simulation *sim)
{
    pthread_mutex_init(&sim->log_mutex, NULL);
    pthread_mutex_init(&sim->simulation_mutex, NULL);
    pthread_mutex_init(&sim->counter_mutex, NULL);
    sim->request_counter = 0;
    sim->running = false;
    return (1);
}

void wait_simulation(t_simulation *sim)
{
    int i;

    pthread_join(sim->monitor_thread, NULL);
    wake_dongles(sim);
    i = 0;
    while (i < sim->config.coder_count)
    {
        pthread_join(sim->coders[i].thread, NULL);
        i++;
    }
}

void destroy_simulation(t_simulation *sim)
{
    int i;

    request_stop(sim);
    i = 0;
    while (i < sim->config.coder_count)
    {
        pthread_mutex_destroy(&sim->dongles[i].mutex);
        pthread_cond_destroy(&sim->dongles[i].cond);
        wait_heap_destroy(&sim->dongles[i].wait_heap);
        pthread_mutex_destroy(&sim->coders[i].mutex);
        i++;
    }
    pthread_mutex_destroy(&sim->simulation_mutex);
    pthread_mutex_destroy(&sim->counter_mutex);
    pthread_mutex_destroy(&sim->log_mutex);
    free(sim->coders);
    free(sim->dongles);
}

int init_simulation(t_simulation *sim, const t_config *config)
{
    memset(sim, 0, sizeof(*sim));
    sim->config = *config;
    sim->coders = malloc(sizeof(t_coder) * config->coder_count);
    sim->dongles = malloc(sizeof(t_dongle) * config->coder_count);
    if (!sim->coders || !sim->dongles)
    {
        free(sim->coders);
        free(sim->dongles);
        return (0);
    }
    init_mutexes(sim);
    init_dongles_coders(sim);
    if (!start_threads(sim))
    {
        request_stop(sim);
        wait_simulation(sim);
        destroy_simulation(sim);
        return (0);
    }
    return (1);
}
