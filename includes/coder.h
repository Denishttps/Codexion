#ifndef CODER_H
#define CODER_H

#include "codexion.h"

int	take_two_dongles(t_coder *coder, t_simulation *sim);
void	release_two_dongles(t_coder *coder, t_simulation *sim);
void	log_message(t_simulation *sim, int coder_id, const char *msg);
void	*coder_thread(void *arg);

#endif
