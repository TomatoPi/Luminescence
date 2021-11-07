/*
  Pads : NoteOn 0x90, NoteOff 0x80
    channel : colonne : 0 - 8
    d1 : ligne : 0x35 - 0x39
                 0x34 ligne du bas

    dernière colonne : 0x52 - 0x56
                       0x51 bas droite

    momentanné

  Selection track : CC 0xb0
    channel : colonne
    d1 : 0x17
    8 messages d'affilé.

    stationnaire

  Activator :
    NoteOn, NoteOff
    channel : colonne
    note :
      0x32 : Activator
      0x31 : Solo
      0x30 : Arm

  Faders :
    CC, channel : colonne, 0x07, value

  Master : 
    0xb0 0x0e value

  Encodeurs :
    0xb0 0x30-0x33 value
    0xb0 0x34-0x37 value

    0xb0 0x10-0x13 value
    0xb0 0x14-0x17 value

  Play, Stop, Rec :
    NoteOn,NoteOff
    0x5b,c,d

  TapTempo :
    Note, 0x63
  
  Nudge +, - :
    Note, 0x65 0x64

  PAN,SendA - C :
    Note, 0x57 - 0x5a 
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#include <string>

const char* path_from_apc = NULL;
const char* path_to_apc = NULL;


volatile int is_running = 1;

void register_mappings()
{

}

std::string midistr_to_command(const char* midistr)
{
	uint8_t msg[512];
  ssize_t len = 0, nread = strlen(midistr), index = 0;
  while(index < nread)
  {
    int tmp;
    int n = sscanf(midistr+index, "%02x", &tmp);
    if (n == -1)
    {
      break;
    }			
    if (n == 0)
      break;
    msg[len] = tmp;
    fprintf(stderr, "%zd : %u : reamaining : %s\n", len, msg[len], midistr+index);
    
    len += 1;
    index += 3;
  }
  // convert raw midi in 'msg' to command str
  if (len != 3)
  {
    fprintf(stderr, "Unsupported midimsg : %s\n", midistr);
    return std::string();
  }

  uint16_t key = ((uint16_t)msg[0]) << 8 | msg[1];
  uint8_t value = msg[2];
  // search for key in mappings
  if (1) // key not found
  {
    fprintf(stderr, "Unbounded midi msg %s\n", midistr);
    return std::string();
  }

  // generate message from key and value
  char command[512];
  sprintf(command, "%04u %u", key, value);
  return std::string(command);
}

// Search for mapping object in maping table
// convert command args to raw midi str
// return message length or -1 on error
int command_to_midistr(const char* command, char* midistr)
{
  if (0) // if command not found
  {
    fprintf(stderr, "Unbound command : %s\n", command);
    return 0;
  }

  uint16_t key = 0; // get key from command
  uint8_t value = 0;// convert args to value
  sprintf(midistr, "%02x %02x %02x", key >> 8, key & 0xFF, value);
  return 3;
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

  if (argc != 3)
  {
    fprintf(stderr, "Usage : %s <from-apc40> <to-apc40>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  path_from_apc = argv[1];
  path_to_apc = argv[2];

  register_mappings();

  pid_t cpid = fork();
  if (-1 == cpid)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (0 == cpid) //  Child : FromAPC -> stdout
  {
    char buffer[512];
    FILE* apc40 = fopen(path_from_apc, "r");
    if (!apc40)
    {
      perror("fopen from apc");
      exit(EXIT_FAILURE);
    }
    while (is_running && fgets(buffer, 512, apc40))
    {
      std::string command = midistr_to_command(buffer);
      if (command.length())
      {
        fprintf(stdout, "%s\n", command.c_str());
      }
    }
  }
  else // 0 != cpid : Parent : stdin -> toAPC
  {
    char command_buffer[512], midibuffer[512];
    FILE* apc40 = fopen(path_to_apc, "w");
    if (!apc40)
    {
      perror("fopen to apc");
      exit(EXIT_FAILURE);
    }
    while (is_running && fgets(command_buffer, 512, stdin))
    {
      int msglen = command_to_midistr(command_buffer, midibuffer);
      if (-1 == msglen)
      {
        fprintf(stderr, "Error parsing command : %s", command_buffer);
        exit(EXIT_FAILURE);
      }
      else if (0 < msglen)
      {
        fprintf(apc40, "%s\n", midibuffer);
      }
    }
    kill(cpid, SIGTERM);
  }

  return 0;
}