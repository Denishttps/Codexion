/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   priority_queue_utils.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/12 12:18:23 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "priority_queue.h"

int	wait_heap_compare(const t_request *a, const t_request *b,
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

void	swap_nodes(t_wait_heap *heap, int a, int b)
{
	t_request	tmp;

	tmp = heap->items[a];
	heap->items[a] = heap->items[b];
	heap->items[b] = tmp;
}

void	heap_up(t_wait_heap *heap, int index, const t_config *config)
{
	int	parent;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (wait_heap_compare(&heap->items[index], &heap->items[parent],
				config) >= 0)
			break ;
		swap_nodes(heap, index, parent);
		index = parent;
	}
}

void	heap_down(t_wait_heap *heap, int index, const t_config *config)
{
	int	left;
	int	right;
	int	best;

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
			break ;
		swap_nodes(heap, index, best);
		index = best;
	}
}
