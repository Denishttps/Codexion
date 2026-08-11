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

static int can_take(t_dongle *dongle, int coder_id)
{
    t_request top;

    if (dongle->wait_heap.size == 0)
        return (0);
    top = wait_heap_peek(&dongle->wait_heap);
    if (top.coder_id != coder_id)
        return (0);
    if (!dongle->available)
        return (0);
    return (1);
}

static int cooldown_ok(t_dongle *dongle, t_simulation *sim)
{
    if (dongle->last_release_ms == 0)
        return (1);
    if (get_time_ms() - dongle->last_release_ms >= sim->config.dongle_cooldown)
        return (1);
    return (0);
}

static void wait_cooldown(t_dongle *dongle, t_simulation *sim)
{
    struct timeval  tv;
    struct timespec ts;
    long long       elapsed;
    long long       remaining;

    elapsed = get_time_ms() - dongle->last_release_ms;
    if (elapsed >= sim->config.dongle_cooldown)
        return;
    remaining = sim->config.dongle_cooldown - elapsed;
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + (tv.tv_usec / 1000 + remaining) / 1000;
    ts.tv_nsec = ((tv.tv_usec / 1000 + remaining) % 1000) * 1000000;
    pthread_cond_timedwait(&dongle->cond, &dongle->mutex, &ts);
}

static void acquire_dongle(t_coder *coder, t_simulation *sim, t_dongle *dongle)
{
    t_request req;

    req.coder_id = coder->id;
    pthread_mutex_lock(&coder->mutex);
    req.deadline = coder->last_compile_start + sim->config.time_to_burnout;
    pthread_mutex_unlock(&coder->mutex);

    pthread_mutex_lock(&sim->counter_mutex);
    req.arrival_order = sim->request_counter++;
    pthread_mutex_unlock(&sim->counter_mutex);

    pthread_mutex_lock(&dongle->mutex);
    wait_heap_push(&dongle->wait_heap, req, &sim->config);
    while (is_running(sim) && (!can_take(dongle, coder->id) || !cooldown_ok(dongle, sim)))
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

static void get_dongle_order(t_coder *coder, t_dongle **first, t_dongle **second)
{
    if (coder->left_idx <= coder->right_idx)
    {
        *first = coder->left;
        *second = coder->right;
    }
    else
    {
        *first = coder->right;
        *second = coder->left;
    }
}

static void release_dongle(t_dongle *dongle)
{
    pthread_mutex_lock(&dongle->mutex);
    dongle->available = true;
    dongle->last_release_ms = get_time_ms();
    pthread_cond_broadcast(&dongle->cond);
    pthread_mutex_unlock(&dongle->mutex);
}

void release_two_dongles(t_coder *coder, t_simulation *sim)
{
    (void)sim;
    release_dongle(coder->left);
    release_dongle(coder->right);
}

static int compile_cycle(t_coder *coder, t_simulation *sim)
{
    t_dongle *first;
    t_dongle *second;

    get_dongle_order(coder, &first, &second);
    acquire_dongle(coder, sim, first);
    if (!is_running(sim))
        return (0);
    log_message(sim, coder->id, "has taken a dongle");
    acquire_dongle(coder, sim, second);
    if (!is_running(sim))
        return (0);
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

static void handle_single_coder(t_coder *coder, t_simulation *sim)
{
    acquire_dongle(coder, sim, coder->left);
    if (!is_running(sim))
        return;
    log_message(sim, coder->id, "has taken a dongle");
    while (is_running(sim))
        ft_usleep(1000, sim);
    release_dongle(coder->left);
}

void *coder_thread(void *arg)
{
    t_coder         *coder;
    t_simulation    *sim;

    coder = arg;
    sim = coder->sim;
    if (sim->config.coder_count == 1)
    {
        handle_single_coder(coder, sim);
        return (NULL);
    }
    while (is_running(sim) && coder->compile_count < sim->config.compiles_required)
    {
        if (!compile_cycle(coder, sim))
            break;
        if (!is_running(sim))
            break;
        log_message(sim, coder->id, "is debugging");
        ft_usleep(sim->config.time_to_debug, sim);
        log_message(sim, coder->id, "is refactoring");
        ft_usleep(sim->config.time_to_refactor, sim);
    }
    return (NULL);
}
