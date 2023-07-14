/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: exam <marvin@42.fr>                        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 1970/01/01 00:00:00 by exam              #+#    #+#             */
/*   Updated: 1970/01/01 19:44:11 by exam             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "microshell.h"

int	err(char *str)
{
	while (*str)
		write(2, str++, 1);
	return (1);
}

int	cd(char **argv, int i)
{
	if (i != 2)
		return (err("error: cd: bad arguments\n"));
	else if (chdir(argv[1]) == -1)
		return (err("error: cd: cannot change directory to "), err(argv[1]), err("\n"));
	return (0);
}

int	exec(char **argv, char **envp, int i)
{
	int	fd[2];
	int	pid;
	int	status;
	int	flag_pipe;

	flag_pipe = 0;
	if (argv[i] && !strcmp(argv[i], "|"))
		flag_pipe = 1;
	if (*argv && (!strcmp(*argv, "|") || !strcmp(*argv, ";")))
		return (0);
	if (flag_pipe && pipe(fd) == -1)
		return (err("error: fatal\n"));
	pid = fork();
	if (!pid) 
	{
		argv[i] = 0;
		if (flag_pipe && (dup2(fd[1], 1) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1))
			return (err("error: fatal\n"));
		execve(*argv, argv, envp);
		return (err("error: cannot execute "), err(*argv), err("\n"));
	}
	waitpid(pid, &status, 0);
	if (flag_pipe && (dup2(fd[0], 0) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1))
		return (err("error: fatal\n"));
	return (WIFEXITED(status) && WEXITSTATUS(status));
}

int	main(int argc, char **argv, char **envp)
{
	int	i;
	int	status;

	i = 0;
	status = 0;
	if (argc == 1)
		return (status); 
	while (argv[i] && argv[++i]) 
	{
		argv += i;
		i = 0;
		while (argv[i] && strcmp(argv[i], "|") && strcmp(argv[i], ";"))
			i++;
		if (!strcmp(*argv, "cd"))
			status = cd(argv, i);
		else if (i)
			status = exec(argv, envp, i);
	}
	return (status);
}
