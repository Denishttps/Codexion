#ifndef PRIORITY_QUEUE_H
# define PRIORITY_QUEUE_H

# include "codexion.h"

int		wait_heap_init(t_wait_heap *heap, int capacity);
void	wait_heap_destroy(t_wait_heap *heap);
int		wait_heap_compare(const t_request *a, const t_request *b,
			const t_config *config);
int		wait_heap_push(t_wait_heap *heap, t_request req,
			const t_config *config);
t_request wait_heap_pop(t_wait_heap *heap, const t_config *config);
t_request wait_heap_peek(t_wait_heap *heap);

#endif
