#include <jack/jack.h>
#include <jack/midiport.h>

#include <unistd.h>
#include <stdio.h>
#include <signal.h>

const char* pgm_name = "Jack-Proxy-Out";
jack_client_t* client = NULL;
jack_port_t* midi_in = NULL;

volatile int is_running = 1;

int jack_callback(jack_nframes_t nframes, void* args)
{
  (void) args;

  jack_nframes_t events_count;
  jack_midi_event_t event;

  void* in_buffer = jack_port_get_buffer(midi_in, nframes);
  events_count = jack_midi_get_event_count(in_buffer);

  for (jack_nframes_t i = 0 ; i < events_count ; ++i)
  {
    if (0 != jack_midi_event_get(&event, in_buffer, i))
      continue;

    for (size_t i = 0 ; i < event.size ; ++i)
    {
      fprintf(stdout, "%02x ", event.buffer[i]);
    }
    fprintf(stdout, "\n");
  }

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
	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);	
	
	jack_status_t jack_status;
	client = jack_client_open(pgm_name, JackNullOption, &jack_status);
	if (NULL == client)
		{ perror(""); return __LINE__; }
	
	midi_in = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (NULL == midi_in)
		{ perror(""); return __LINE__; }
	
	if (0 != jack_set_process_callback(client, jack_callback, NULL))
		{ perror(""); return __LINE__; }
	
	if (0 != jack_activate(client))
		{ perror(""); return __LINE__; }
		
	while(is_running){ usleep(1000); }
	
	jack_deactivate(client);
	jack_client_close(client);
	
	return 0;
}