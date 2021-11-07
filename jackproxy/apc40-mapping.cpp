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
#include <vector>
#include <functional>
#include <unordered_map>

const char* path_from_apc = NULL;
const char* path_to_apc = NULL;

volatile int is_running = 1;

struct binding_t
{
  uint8_t        midikey[2];
  const char*   command;
  std::function<std::string(uint8_t)> midival_to_str;
  std::function<uint8_t(const char*)> str_to_midival;

  uint16_t key() const { return ((uint16_t)midikey[0]) << 8 | midikey[1]; }
};

std::string uint8_to_str(uint8_t val) { return std::to_string(val); }
uint8_t str_to_uint8(const char* str) { int res; sscanf(str, "%d", &res); return res; }

std::vector<binding_t>                                  bindings_list;
std::unordered_multimap<int16_t, const binding_t*>      midi_to_command_map;
std::unordered_multimap<std::string, const binding_t*>  command_to_midi_map;

void register_mappings()
{
  // Generate bindings
  bindings_list = {
    { {0xb0, 0x07}, "intensity:0", uint8_to_str, str_to_uint8 },
    { {0x90, 0x35}, "colormod:0", uint8_to_str, str_to_uint8 },
  };

  // Generate tables
  for (const auto& binding : bindings_list)
  {
    midi_to_command_map.emplace(binding.key(), &binding);
    command_to_midi_map.emplace(binding.command, &binding);
  }
}

std::vector<std::string> midistr_to_command(const char* midistr)
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
    return {};
  }

  const uint16_t key = ((uint16_t)msg[0]) << 8 | msg[1];
  // search for key in mappings
  auto [begin, end] = midi_to_command_map.equal_range(key);
  if (begin == end) // key not found
  {
    fprintf(stderr, "Unbounded midi msg %04x\n", key);
    return {};
  }

  const uint8_t value = msg[2];
  std::vector<std::string> result;
  for (auto itr = begin; itr != end; ++itr)
  {
    auto& [_, binding] = *itr;
    char tmp[512];
    sprintf(tmp, "%s %s", binding->command, binding->midival_to_str(value).c_str());
    result.emplace_back(tmp);
  }

  return result;
}

// Search for mapping object in maping table
// convert command args to raw midi str
// return message length or -1 on error
std::vector<std::string> command_to_midistr(const char* commandstr)
{
  char cmd[64], arg[64];
  if (sscanf(commandstr, "%s %s", cmd, arg) != 2)
  {
    fprintf(stderr, "Invalid command : %s\n", commandstr);
    return {};
  }

  auto [begin, end] = command_to_midi_map.equal_range(cmd);
  if (begin == end) // key not found
  {
    fprintf(stderr, "Unbound command : %s\n", cmd);
    return {};
  }

  std::vector<std::string> result;
  for (auto itr = begin; itr != end; ++itr)
  {
    auto& [_, binding] = *itr;
    uint16_t key = binding->key(); // get key from command
    uint8_t value = binding->str_to_midival(arg);// convert args to value

    char tmp[512];
    sprintf(tmp, "%02x %02x %02x", key >> 8, key & 0xFF, value);
    result.emplace_back(tmp);
  }
  
  return result;
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
      auto result = midistr_to_command(buffer);
      for (auto& cmd : result)
      {
        fprintf(stdout, "%s\n", cmd.c_str());
      }
    }
  }
  else // 0 != cpid : Parent : stdin -> toAPC
  {
    char buffer[512];
    FILE* apc40 = fopen(path_to_apc, "w");
    if (!apc40)
    {
      perror("fopen to apc");
      exit(EXIT_FAILURE);
    }
    while (is_running && fgets(buffer, 512, stdin))
    {
      auto result = command_to_midistr(buffer);
      for (auto& msg : result)
      {
        fprintf(apc40, "%s\n", msg.c_str());
        fprintf(stderr, "To apc : %s\n", msg.c_str());
      }
      fflush(apc40);
    }
    kill(cpid, SIGTERM);
  }

  return 0;
}