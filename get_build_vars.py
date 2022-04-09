#!/usr/bin/env python3
import os
import sys
import yaml

secrets_path = os.path.expanduser("~/.secrets.yaml")

with open(secrets_path) as file:
    secrets_yaml = yaml.load(file, Loader=yaml.FullLoader)

esp_secrets = secrets_yaml['esp']

build_vars = list(map(lambda x: f"{x}={esp_secrets[x]}", sys.argv[1:]))

print(" -D ".join([""] + build_vars))
