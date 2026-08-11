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

int wait_heap_compare(const t_request *a, const t_request *b,
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
	if (a->arrival_order != b->arrival_order)
	{
		if (a->arrival_order < b->arrival_order)
			return (-1);
		return (1);
	}
	return (0);
}

static void swap_nodes(t_wait_heap *heap, int a, int b)
{
	t_request tmp;

	tmp = heap->items[a];
	heap->items[a] = heap->items[b];
	heap->items[b] = tmp;
}

static void heap_up(t_wait_heap *heap, int index, const t_config *config)
{
	int parent;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (wait_heap_compare(&heap->items[index], &heap->items[parent],
				config) >= 0)
			break;
		swap_nodes(heap, index, parent);
		index = parent;
	}
}

static void heap_down(t_wait_heap *heap, int index, const t_config *config)
{
	int left;
	int right;
	int best;

	while (1)
	{
		left = index * 2 + 1;
		right = index * 2 + 2;
		best = index;
		if (left < heap->size && wait_heap_compare(&heap->items[left],
				&heap->items[best], config) < 0)
			best = left;
		if (right < heap->size && wait_heap_compare(&heap->items[right],
				&heap->items[best], config) < 0)
			best = right;
		if (best == index)
			break;
		swap_nodes(heap, index, best);
		index = best;
	}
}

int wait_heap_init(t_wait_heap *heap, int capacity)
{
	heap->items = malloc(sizeof(t_request) * capacity);
	if (!heap->items)
		return (0);
	heap->size = 0;
	heap->capacity = capacity;
	return (1);
}

void wait_heap_destroy(t_wait_heap *heap)
{
	free(heap->items);
	heap->items = NULL;
	heap->size = 0;
	heap->capacity = 0;
}

int wait_heap_push(t_wait_heap *heap, t_request req, const t_config *config)
{
	if (heap->size >= heap->capacity)
		return (0);
	heap->items[heap->size] = req;
	heap->size++;
	heap_up(heap, heap->size - 1, config);
	return (1);
}

t_request wait_heap_pop(t_wait_heap *heap, const t_config *config)
{
	t_request req;

	req = heap->items[0];
	heap->size--;
	if (heap->size > 0)
	{
		heap->items[0] = heap->items[heap->size];
		heap_down(heap, 0, config);
	}
	return (req);
}

t_request wait_heap_peek(t_wait_heap *heap)
{
	return (heap->items[0]);
}
