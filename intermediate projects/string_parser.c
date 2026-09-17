#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_ENTRIES 32
#define MAX_KEY_LEN 32
#define MAX_VAL_LEN 64

typedef struct {
  char key[MAX_KEY_LEN];
  char value[MAX_VAL_LEN];
} config_entry_t;

typedef struct {
  config_entry_t entries[MAX_ENTRIES];
  size_t count;
} config_t;

void config_init(config_t *cfg) { cfg->count = 0; }

// Parses one line like "wifi_ssid=MyNetwork" and adds it as an entry.
// Ignores blank lines and lines starting with '#'.
bool config_parse_line(config_t *cfg, const char *line) {
  if (strlen(line) < 2) {
    return false;
  }
  char *brk_ptr = strchr(line, '=');
  memset(cfg->entries[cfg->count].key, '\0', MAX_KEY_LEN);
  memset(cfg->entries[cfg->count].value, '\0', MAX_VAL_LEN);
  if (line[0] != '#' && brk_ptr != NULL && cfg->count < MAX_ENTRIES) {
    size_t j = 0;
    for (uint8_t i = 0; i < strlen(line); i++) {
      if (&line[i] < brk_ptr && i < MAX_KEY_LEN - 1) {
        cfg->entries[cfg->count].key[i] = line[i];
      }
      if (&line[i] > brk_ptr && j < MAX_VAL_LEN - 1) {
        // printf("pushing value %c\n", line[i]);
        cfg->entries[cfg->count].value[j] = line[i];
        // printf("pushed value %c\n", cfg->entries[cfg->count].value[j]);
        j++;
      }
    }
    cfg->count++;
    return true;
  }
  return false;
}

// Returns the value for a given key, or NULL if not found.
const char *config_get(config_t *cfg, const char *key) {
  if (cfg->count == 0 || strlen(key) > MAX_KEY_LEN) {
    return NULL;
  }
  for (uint8_t i = 0; i < cfg->count; i++) {
    if (strcmp(key, cfg->entries[i].key) == 0) {
      return cfg->entries[i].value;
    }
  }
  return NULL;
}

int main() {
  config_t new_config;
  config_init(&new_config);
  char str[5][20] = {"id=dak07", "#comment1", "#test comment",
                     "pass=domestic123", "loc=delhi"};
  char find[] = "loc";
  for (uint8_t i = 0; i < (sizeof(str) / sizeof(str[0])); i++) {
    (void)config_parse_line(&new_config, &str[i][0]);
  }
  printf("Entry count: %lu\n", new_config.count);
  printf("Current config entries: \n");
  printf("Key | Value\n");
  for (uint8_t i = 0; i < new_config.count; i++) {
    printf("%s | %s\n", new_config.entries[i].key, new_config.entries[i].value);
  }
  printf("Looking for %s .....\n", find);
  const char *result = config_get(&new_config, find);
  if (result != NULL) {
    printf("Value found: %s\n", result);
  } else {
    printf("Invalid key\n");
  }
  return 0;
}
