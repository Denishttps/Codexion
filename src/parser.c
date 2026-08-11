/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbobrov <dbobrov@student.42wolfsburg.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/10 14:30:34 by dbobrov           #+#    #+#             */
/*   Updated: 2026/08/10 14:30:34 by dbobrov          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser.h"


int parse_args(int argc, char **argv, t_config *config)
{
	if (argc != 9)
		return (0);

	config->coder_count = get_argument(argv[1]);
	config->time_to_burnout = get_argument(argv[2]);
	config->time_to_compile = get_argument(argv[3]);
	config->time_to_debug = get_argument(argv[4]);
	config->time_to_refactor = get_argument(argv[5]);
	config->compiles_required = get_argument(argv[6]);
	config->dongle_cooldown = get_argument(argv[7]);

	if (config->coder_count < 0
        || config->time_to_burnout < 0
        || config->time_to_compile < 0
        || config->time_to_debug < 0
        || config->time_to_refactor < 0
        || config->compiles_required < 0
        || config->dongle_cooldown < 0)
        return (0);

	if (strcmp(argv[8], "fifo") == 0)
		config->scheduler = SCHEDULER_FIFO;
	else if (strcmp(argv[8], "edf") == 0)
		config->scheduler = SCHEDULER_EDF;
	else
		return (0);
	return (1);
}


int get_argument(const char *arg)
{
	int data;
	int i;
	int digit;

	if (!arg || arg[0] == '\0')
		return (-1);
	i = 0;
	data = 0;
	while (arg[i])
	{
		if (arg[i] < '0' || arg[i] > '9')
			return (-1);
		digit = arg[i] - '0';
		if (data > (INT_MAX - digit) / 10)
			return (-1);
		data = data * 10 + digit;
		i++;
	}
	if (data <= 0)
		return (-1);

	return (data);
}
