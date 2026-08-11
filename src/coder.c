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
#include "utils.h"

static int get_left_dongle(const t_coder *coder, int count)
{
    (void)count;
    return (coder->id - 1);
}

static int get_right_dongle(const t_coder *coder, int count)
{
    return (coder->id % count);
}

static int compare_priority(const t_coder *a, const t_coder *b,
    const t_config *config)
{
    if (config->scheduler == SCHEDULER_EDF)
    {
        if (a->deadline != b->deadline)
            return (a->deadline < b->deadline ? -1 : 1);
    }
    if (a->request_time != b->request_time)
        return (a->request_time < b->request_time ? -1 : 1);
    if (a->id != b->id)
        return (a->id < b->id ? -1 : 1);
    return (0);
}

static int wants_dongle(const t_coder *coder, int dongle_idx, int count)
{
    int left = get_left_dongle(coder, count);
    int right = get_right_dongle(coder, count);

    return (dongle_idx == left || dongle_idx == right);
}

static int is_best_for_dongle(t_simulation *sim, t_coder *coder, int dongle_idx)
{
    int i;
    int count;
    t_coder *other;

    count = sim->config->coder_count;
    i = 0;
    while (i < count)
    {
        other = &sim->coders[i];
        if (other != coder && other->waiting && wants_dongle(other, dongle_idx, count))
        {
            if (compare_priority(other, coder, sim->config) < 0)
                return (0);
        }
        i++;
    }
    return (1);
}

static const t_coder *get_global_best_waiting(t_simulation *sim, int left, int right)
{
    int i;
    int count;
    const t_coder *best = NULL;
    const t_coder *current;

    count = sim->config->coder_count;
    i = 0;
    while (i < count)
    {
        current = &sim->coders[i];
        if (current->waiting && wants_dongle(current, left, count)
            && wants_dongle(current, right, count))
        {
            if (!best || compare_priority(current, best, sim->config) < 0)
                best = current;
        }
        i++;
    }
    return (best);
}

static void lock_dongle_pair(t_simulation *sim, int left, int right)
{
    if (left == right)
    {
        pthread_mutex_lock(&sim->dongles[left].mutex);
        return;
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

static void unlock_dongle_pair(t_simulation *sim, int left, int right)
{
    if (left == right)
    {
        pthread_mutex_unlock(&sim->dongles[left].mutex);
        return;
    }
    pthread_mutex_unlock(&sim->dongles[right].mutex);
    pthread_mutex_unlock(&sim->dongles[left].mutex);
}

int take_two_dongles(t_coder *coder, t_simulation *sim)
{
    int left;
    int right;
    long long now;
    const t_coder *best;

    left = get_left_dongle(coder, sim->config->coder_count);
    right = get_right_dongle(coder, sim->config->coder_count);
    pthread_mutex_lock(&sim->state_mutex);
    coder->waiting = 1;
    coder->request_time = get_time_ms();
    while (!sim->stop_simulation)
    {
        now = get_time_ms();
        if (sim->dongles[left].available
            && sim->dongles[left].cooldown_until <= now
            && sim->dongles[right].available
            && sim->dongles[right].cooldown_until <= now)
        {
            if (is_best_for_dongle(sim, coder, left)
                && is_best_for_dongle(sim, coder, right))
            {
                lock_dongle_pair(sim, left, right);
                sim->dongles[left].available = 0;
                sim->dongles[right].available = 0;
                unlock_dongle_pair(sim, left, right);
                coder->left_dongle_taken = 1;
                coder->right_dongle_taken = 1;
                coder->waiting = 0;
                pthread_mutex_unlock(&sim->state_mutex);
                return (1);
            }
            best = get_global_best_waiting(sim, left, right);
            if (best == coder)
            {
                lock_dongle_pair(sim, left, right);
                sim->dongles[left].available = 0;
                sim->dongles[right].available = 0;
                unlock_dongle_pair(sim, left, right);
                coder->left_dongle_taken = 1;
                coder->right_dongle_taken = 1;
                coder->waiting = 0;
                pthread_mutex_unlock(&sim->state_mutex);
                return (1);
            }
        }
        pthread_cond_wait(&sim->state_cond, &sim->state_mutex);
    }
    coder->waiting = 0;
    pthread_mutex_unlock(&sim->state_mutex);
    return (0);
}

void release_two_dongles(t_coder *coder, t_simulation *sim)
{
    int left;
    int right;
    long long now;

    left = get_left_dongle(coder, sim->config->coder_count);
    right = get_right_dongle(coder, sim->config->coder_count);
    now = get_time_ms();

    pthread_mutex_lock(&sim->state_mutex);
    lock_dongle_pair(sim, left, right);
    sim->dongles[left].available = 1;
    sim->dongles[left].cooldown_until = now + sim->config->dongle_cooldown;
    sim->dongles[right].available = 1;
    sim->dongles[right].cooldown_until = now + sim->config->dongle_cooldown;
    unlock_dongle_pair(sim, left, right);
    coder->left_dongle_taken = 0;
    coder->right_dongle_taken = 0;
    pthread_cond_broadcast(&sim->state_cond);
    pthread_mutex_unlock(&sim->state_mutex);
}

void *coder_thread(void *arg)
{
    t_coder *coder = arg;
    t_simulation *sim = coder->sim;
    const t_config *config = sim->config;

    while (!sim->stop_simulation && coder->compile_count < config->compiles_required)
    {
        if (!take_two_dongles(coder, sim))
            break;

        coder->last_compile_start = get_time_ms();
        coder->deadline = coder->last_compile_start + config->time_to_burnout;

        log_message(sim, coder->id, "has taken a dongle");
        log_message(sim, coder->id, "has taken a dongle");
        log_message(sim, coder->id, "is compiling");

        usleep(config->time_to_compile * 1000);
        coder->compile_count++;

        release_two_dongles(coder, sim);

        if (coder->compile_count >= config->compiles_required)
            break;

        log_message(sim, coder->id, "is debugging");
        usleep(config->time_to_debug * 1000);

        log_message(sim, coder->id, "is refactoring");
        usleep(config->time_to_refactor * 1000);
    }
    return (NULL);
}