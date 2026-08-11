/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   priority_queue.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 19:45:33 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 19:45:34 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "priority_queue.h"

int	wait_heap_compare(const t_coder *a, const t_coder *b,
	const t_config *config)
{
	if (config->scheduler == SCHEDULER_EDF)
	{
		if (a->deadline != b->deadline)
		{
			if (a->deadline < b->deadline)
				return (-1);
			return (1);
		}
	}
	if (a->request_time != b->request_time)
	{
		if (a->request_time < b->request_time)
			return (-1);
		return (1);
	}
	if (a->id != b->id)
	{
		if (a->id < b->id)
			return (-1);
		return (1);
	}
	return (0);
}

static int	wants_dongle(const t_coder *coder, int dongle_idx, int count)
{
	int	left;
	int	right;

	left = coder->id - 1;
	right = coder->id % count;
	return (dongle_idx == left || dongle_idx == right);
}

static void	swap_nodes(t_wait_heap *heap, int a, int b)
{
	t_coder	*tmp;

	tmp = heap->items[a];
	heap->items[a] = heap->items[b];
	heap->items[b] = tmp;
	heap->items[a]->heap_index = a;
	heap->items[b]->heap_index = b;
}

static void	heap_up(t_wait_heap *heap, int index, const t_config *config)
{
	int	parent;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (wait_heap_compare(heap->items[index], heap->items[parent],
				config) >= 0)
			break ;
		swap_nodes(heap, index, parent);
		index = parent;
	}
}

static void	heap_down(t_wait_heap *heap, int index, const t_config *config)
{
	int	left;
	int	right;
	int	best;

	while (1)
	{
		left = index * 2 + 1;
		right = index * 2 + 2;
		best = index;
		if (left < heap->size && wait_heap_compare(heap->items[left],
				heap->items[best], config) < 0)
			best = left;
		if (right < heap->size && wait_heap_compare(heap->items[right],
				heap->items[best], config) < 0)
			best = right;
		if (best == index)
			break ;
		swap_nodes(heap, index, best);
		index = best;
	}
}

int	wait_heap_init(t_wait_heap *heap, int capacity)
{
	heap->items = malloc(sizeof(t_coder *) * capacity);
	if (!heap->items)
		return (0);
	heap->size = 0;
	heap->capacity = capacity;
	return (1);
}

void	wait_heap_destroy(t_wait_heap *heap)
{
	free(heap->items);
	heap->items = NULL;
	heap->size = 0;
	heap->capacity = 0;
}

int	wait_heap_push(t_wait_heap *heap, t_coder *coder, const t_config *config)
{
	if (coder->heap_index != -1)
		return (1);
	if (heap->size >= heap->capacity)
		return (0);
	heap->items[heap->size] = coder;
	coder->heap_index = heap->size;
	heap->size++;
	heap_up(heap, coder->heap_index, config);
	return (1);
}

void	wait_heap_remove(t_wait_heap *heap, t_coder *coder,
	const t_config *config)
{
	int	index;

	index = coder->heap_index;
	if (index < 0 || index >= heap->size)
		return ;
	coder->heap_index = -1;
	heap->size--;
	if (index == heap->size)
		return ;
	heap->items[index] = heap->items[heap->size];
	heap->items[index]->heap_index = index;
	heap_up(heap, index, config);
	heap_down(heap, heap->items[index]->heap_index, config);
}

int	wait_heap_has_higher_for_dongle(t_wait_heap *heap, t_coder *coder,
	int dongle_idx, int coder_count, const t_config *config)
{
	int		i;
	t_coder	*other;

	i = 0;
	while (i < heap->size)
	{
		other = heap->items[i];
		if (other != coder && other->waiting
			&& wants_dongle(other, dongle_idx, coder_count)
			&& wait_heap_compare(other, coder, config) < 0)
			return (1);
		i++;
	}
	return (0);
}
