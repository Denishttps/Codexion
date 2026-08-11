#ifndef SIMULATION_H
# define SIMULATION_H

# include "codexion.h"

int     init_simulation(t_simulation *sim, const t_config *config);
void	wait_simulation(t_simulation *sim);
void	wake_dongles(t_simulation *sim);
void	*monitor_thread(void *arg);
void    destroy_simulation(t_simulation *sim);

#endif
