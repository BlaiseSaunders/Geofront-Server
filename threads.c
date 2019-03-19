#include "theheader.h"
#include "proto.h"

#define BUF_SIZE 1024


void sigchld_handler()
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}


void *get_in_addr(struct sockaddr *sa)
{
	return &(((struct sockaddr_in *)sa)->sin_addr);
}

struct pthread_list
{
	pthread_t thread;
	struct pthread_list *next;
};

struct handle_client_s
{
	int new_fd;
	struct thread_params_s *params;
};

bool params_in_use;




void *handle_client(void *params)
{
	int sent, new_fd;

	struct thread_params_s *args;

	struct handle_client_s *args1 = params;

	union player_net_dat player_net;

	new_fd = args1->new_fd;
	params_in_use = false;

	args = args1->params;

	printf("Spawned child.\n");
	while (1)
	{
		printf("Packing player info.\n");
		sent = 0;

		player_net.p.x = args->players->x;
		player_net.p.y = args->players->y;
		player_net.p.thrust = args->players->thrust;

		while (sent < (int)sizeof player_net)
			if  ((sent += send(new_fd, &player_net.s+sent, (sizeof player_net)-sent, 0)) == -1)
				perror("send");
		printf("Sent off %d bytes\n", sent);
		fflush(stdout);
		usleep(100000);
	}
	close(new_fd);
	return NULL;
}


void *update_players(void *params)
{
	int sock, new_fd, rv, yes = 1;
	int backlog = 10;
	char port[] = "2424";
	char s[INET6_ADDRSTRLEN];
	struct thread_params_s *args = params;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	struct sigaction sa;
	socklen_t sin_size;



	struct pthread_list *threads;
	struct pthread_list *c_thread;


	struct handle_client_s cli_params;

	threads = (struct pthread_list *)malloc(sizeof (struct pthread_list));

	c_thread = threads;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
	{
		printf("getaddrinfo: %s\n", gai_strerror(rv));
		return NULL;
	}

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sock = socket(p->ai_family, p->ai_socktype,
		                   p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes,
		               sizeof (int)) == -1)
		{
			perror("setsockopt");
			return NULL;
		}
		if (bind(sock, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sock);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		printf("failed to bind\n");
		return NULL;
	}

	freeaddrinfo(servinfo);


	if (listen(sock, backlog) == -1)
	{
		perror("listen");
		return NULL;
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		return NULL;
	}


	printf("Awaiting incoming connections...\n");


	while (1) /* Nii-chan can wait on us Nyoro~n */
	{

		sin_size = sizeof their_addr;
		new_fd = accept(sock, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1)
		{
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
		          get_in_addr((struct sockaddr *)&their_addr),
		          s, sizeof s);
		printf("%s connected.\n", s);



		cli_params.new_fd = new_fd;
		cli_params.params = args;
		pthread_create(&c_thread->thread, NULL, handle_client,
		               &cli_params);

		c_thread->next = (struct pthread_list *)malloc(sizeof
		                 (struct pthread_list));
		c_thread = c_thread->next;

		while (params_in_use)
			sleep(1);

		/*memset(buf, 0, BUF_SIZE);
		got = 0;
		while ((got += recv(sock, buf, BUF_SIZE-1, 0)) > 0)
			printf("got: %d bytes\n", got);
		printf("Message: %s\n", buf);

		sleep(5);*/
	}

}

