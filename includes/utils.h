#ifndef UTILS_H
# define UTILS_H

# include "codexion.h"

long long	get_time_ms(void);
void		log_message(t_simulation *sim, int coder_id, const char *msg);
bool		is_running(t_simulation *sim);
void		ft_usleep(int ms, t_simulation *sim);

#endif
