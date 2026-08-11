/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 12:00:00 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/11 23:57:47 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser.h"
#include "simulation.h"

static int	init_config(int argc, char **argv, t_config *config)
{
	memset(config, 0, sizeof(*config));
	if (!parse_args(argc, argv, config))
	{
		fprintf(stderr, "Invalid arguments\n");
		return (0);
	}
	return (1);
}

int	main(int argc, char **argv)
{
	t_config		config;
	t_simulation	sim;

	if (!init_config(argc, argv, &config))
		return (1);
	if (!init_simulation(&sim, &config))
	{
		fprintf(stderr, "Failed to initialize simulation\n");
		return (1);
	}
	wait_simulation(&sim);
	destroy_simulation(&sim);
	return (0);
}
