#define _GNU_SOURCE

#include <jack/jack.h>
#include <jack/midiport.h>

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

const char* pgm_name = "Jack-Proxy-In";
jack_client_t* client;
jack_port_t* midi_out;

int pipefd[2];

volatile int is_running = 1;

int jack_callback(jack_nframes_t nframes, void* args)
{
	(void) args;
	
	void* out_buffer = jack_port_get_buffer(midi_out, nframes);
	jack_midi_clear_buffer(out_buffer);
	
	char buff[512];
	ssize_t nread;
	do
	{
		nread = read(pipefd[0], buff, 512);
		if (-1 == nread)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else
			{
				perror("read");
				exit(EXIT_FAILURE);
			}
		}
		else if (0 < nread)
		{
	        	void* raw = jack_midi_event_reserve(out_buffer, 0, nread);
		        memcpy(raw, buff, nread);
		        for(int i=0;i<nread;++i)
		        	fprintf(stderr,"%02X ", (uint8_t)buff[i]);
		        fprintf(stderr,"\n");
		}
	} while(0 < nread);
	
  	return 0;
}

void sighandler(int sig)
{
	switch(sig)
	{
	case SIGTERM:
	case SIGINT:
		is_running = 0;
		break;
	}
}

int main(int argc, char* const argv[])
{
	if (pipe2(pipefd, O_NONBLOCK | O_DIRECT))
	{
		perror("pipe2");
		exit(EXIT_FAILURE);
	}
	
	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);	
	
	jack_status_t jack_status;
	client = jack_client_open(pgm_name, JackNullOption, &jack_status);
	if (NULL == client)
		{ perror(""); return __LINE__; }
	
	midi_out = jack_port_register(client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
	if (NULL == midi_out)
		{ perror(""); return __LINE__; }
	
	if (0 != jack_set_process_callback(client, jack_callback, NULL))
		{ perror(""); return __LINE__; }
	
	if (0 != jack_activate(client))
		{ perror(""); return __LINE__; }
	
	char buff[512];
	uint8_t msg[512];
	while (is_running && fgets(buff, 511, stdin))
	{
		ssize_t len = 0, nread=strlen(buff);
		fprintf(stderr, "Read %zd bytes from stdin : %s\n", nread, buff);
		ssize_t index = 0;
		while(index < 10)
		{
			int tmp;
			int n = sscanf(buff+index, "%02x", &tmp);
			if (n == -1)
			{
				break;
			}			
			if (n == 0)
				break;
			msg[len] = tmp;
			fprintf(stderr, "%zd : %u : reamaining : %s\n", len, msg[len], buff+index);
			
			len += 1;
			index += 3;
		}
		if (write(pipefd[1], msg, len) != len)
		{
			perror("write");
			exit(EXIT_FAILURE);
		}
	}
	
	jack_deactivate(client);
	jack_client_close(client);
	
	return 0;
}
