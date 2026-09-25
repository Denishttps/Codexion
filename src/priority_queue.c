/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   priority_queue.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/09/25 14:07:44 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "priority_queue.h"

int	wait_heap_init(t_wait_heap *heap, int capacity)
{
	heap->items = malloc(sizeof(t_request) * capacity);
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

int	wait_heap_push(t_wait_heap *heap, t_request req, const t_config *config)
{
	if (heap->size >= heap->capacity)
		return (0);
	heap->items[heap->size] = req;
	heap->size++;
	heap_up(heap, heap->size - 1, config);
	return (1);
}

int	wait_heap_remove(t_wait_heap *heap, int coder_id,
	const t_config *config)
{
	int	index;

	index = 0;
	while (index < heap->size && heap->items[index].coder_id != coder_id)
		index++;
	if (index == heap->size)
		return (0);
	heap->size--;
	if (index < heap->size)
	{
		heap->items[index] = heap->items[heap->size];
		heap_up(heap, index, config);
		heap_down(heap, index, config);
	}
	return (1);
}

t_request	wait_heap_pop(t_wait_heap *heap, const t_config *config)
{
	t_request	req;

	req = heap->items[0];
	heap->size--;
	if (heap->size > 0)
	{
		heap->items[0] = heap->items[heap->size];
		heap_down(heap, 0, config);
	}
	return (req);
}
