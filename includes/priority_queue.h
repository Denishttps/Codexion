#ifndef PRIORITY_QUEUE_H
# define PRIORITY_QUEUE_H

# include "codexion.h"

int		wait_heap_init(t_wait_heap *heap, int capacity);
void	wait_heap_destroy(t_wait_heap *heap);
int		wait_heap_compare(const t_coder *a, const t_coder *b,
			const t_config *config);
int		wait_heap_push(t_wait_heap *heap, t_coder *coder,
			const t_config *config);
void	wait_heap_remove(t_wait_heap *heap, t_coder *coder,
			const t_config *config);
int		wait_heap_has_higher_for_dongle(t_wait_heap *heap, t_coder *coder,
			int dongle_idx, int coder_count, const t_config *config);

#endif
