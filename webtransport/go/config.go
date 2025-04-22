package main

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
)

type Config map[string]interface{}

func NewConfig() (Config, error) {
	cwd, err := os.Getwd()
	if err != nil {
		return nil, err
	}
	absPath := filepath.Join(cwd, "./eqemu_config.json")

	data, err := os.ReadFile(absPath)
	if err != nil {
		return nil, err
	}

	var config Config
	if err := json.Unmarshal(data, &config); err != nil {
		return nil, err
	}

	return config, nil
}

func (c Config) Get(keyPath string, defaultValue interface{}) interface{} {
	keys := strings.Split(keyPath, ".")
	current := c

	for i, key := range keys {
		if i == len(keys)-1 {
			if value, ok := current[key]; ok {
				return value
			}
			return defaultValue
		}

		if next, ok := current[key].(map[string]interface{}); ok {
			current = next
		} else {
			return defaultValue
		}
	}

	return defaultValue
}

func (c Config) GetString(keyPath string, defaultValue string) string {
	if value, ok := c.Get(keyPath, defaultValue).(string); ok {
		return value
	}
	return defaultValue
}

func (c Config) GetInt(keyPath string, defaultValue int) int {
	// JSON numbers are float64 by default
	if value, ok := c.Get(keyPath, float64(defaultValue)).(float64); ok {
		return int(value)
	}
	return defaultValue
}

func (c Config) GetBool(keyPath string, defaultValue bool) bool {
	if value, ok := c.Get(keyPath, defaultValue).(bool); ok {
		return value
	}
	return defaultValue
}
